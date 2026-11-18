"""Time FrameLab's real output loop with preloaded original recording pixels."""
import argparse
import csv
import json
import pathlib
import shutil
import statistics
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def run(root, mult, legacy, quality, seconds, repeat, vsync=False):
    label = f'{mult}x-{quality}-{"old-wait" if legacy else "independent-pump"}-r{repeat}'
    out = root / label
    if out.exists():
        raise RuntimeError(f'Output already exists: {out}')
    out.mkdir(parents=True)
    logdir = ROOT / 'app/logs'
    before = set(logdir.glob('*'))
    args = [str(ROOT / 'app/FrameLab-next.exe'), '--replay-recording',
            str(ROOT / 'app/recordings/analysis-15s-84744531'), '--replay-from', '9.5',
            '--replay-length', '3', '--fg', f'--{mult}x', f'--{quality}', '--no-vsync',
            '--seconds', str(seconds), '--verify-pacing']
    if legacy:
        args.append('--legacy-output-wait')
    if vsync:
        args.remove('--no-vsync')
    print('Replaying:', label, flush=True)
    p = subprocess.run(args, cwd=ROOT / 'app', timeout=seconds + 75,
                       creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if p.returncode:
        raise RuntimeError(f'App returned {p.returncode}')
    files = set(logdir.glob('*')) - before
    for f in files:
        shutil.copy2(f, out / f.name)
    telemetry = [f for f in files if f.suffix == '.csv' and not f.stem.endswith('-pacing')]
    if len(telemetry) != 1:
        raise RuntimeError(f'Unexpected telemetry files: {telemetry}')
    rows = list(csv.DictReader(telemetry[0].open()))
    rows = [r for r in rows if float(r['elapsed_s']) >= 5]
    if len(rows) < 5:
        raise RuntimeError('Insufficient replay telemetry; app may have been minimized')
    metrics = {key: statistics.mean(float(r[key]) for r in rows) for key in rows[0]}
    report = {'name': label, 'multiplier': mult, 'quality': quality, 'legacy_output_wait': legacy,
              'args': args[1:], 'samples_after_warmup': len(rows), 'means': metrics,
              'limitations': 'Original 1440p R pixels from take 84744531, 9.5–12.5 s, repeated with recorded arrival spacing. Five-second startup excluded. No recording/export. GPU counters are whole-device. Latency is callback to Present, not physical monitor or HDMI latency. FPS counts submitted/OS-observed IDs, not unique pixels.'}
    (out / 'summary.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'  submitted {metrics["content_submissions_fps"]:.1f}, observed {metrics["os_observed_content_fps"]:.1f}, '
          f'latency {metrics["callback_to_present_mean_ms"]:.1f} ms, GPU {metrics["gpu_total_percent"]:.1f}%', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=pathlib.Path)
    parser.add_argument('--seconds', type=int, default=20)
    parser.add_argument('--repeats', type=int, default=1)
    parser.add_argument('--quality', choices=['low', 'medium', 'high'], default='medium')
    parser.add_argument('--multipliers', nargs='+', type=int, choices=[2, 3, 4], default=[3, 4])
    parser.add_argument('--vsync', action='store_true')
    args = parser.parse_args()
    for repeat in range(args.repeats):
        for mult in args.multipliers:
            for legacy in ((True, False) if repeat % 2 == 0 else (False, True)):
                run(args.out.resolve(), mult, legacy, args.quality, args.seconds, repeat, args.vsync)

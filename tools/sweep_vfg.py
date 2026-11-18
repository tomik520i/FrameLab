"""Repeatable, headless VFG experiments using saved original pixels (no screen capture)."""
import argparse
import csv
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ['pair_id', 'offset_a', 'offset_b', 'truth_1', 'truth_2', 'truth_3',
          'A_id', 'B_id', 'source_a_s', 'source_b_s']


def write_pairs(path, rows):
    with path.open('w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(HEADER)
        writer.writerows(rows)


def prepare(output):
    output.mkdir(parents=True, exist_ok=True)
    for mult in (3, 4):
        size = 1920 * 1080 * 4
        rows = []
        for n in range(0, 240 - mult, mult):
            truth = [(n + i) * size if i < mult else -1 for i in (1, 2, 3)]
            rows.append([n, n * size, (n + mult) * size, *truth,
                         n, n + mult, n / 60, (n + mult) / 60])
        write_pairs(output / f'csgo-{mult}x.csv', rows)
        write_pairs(output / f'csgo-{mult}x-smoke.csv', rows[:8])
    recording = ROOT / 'app/recordings/analysis-15s-84744531'
    frames = list(csv.DictReader((recording / 'frames.csv').open()))
    raw = {int(r['id']): r for r in frames if r['kind'] == 'R'}
    pairs = sorted({(int(r['source_A']), int(r['source_B'])) for r in frames
                    if r['kind'] == 'G' and int(r['source_A']) in raw and int(r['source_B']) in raw})
    chosen = set(pairs[::max(1, len(pairs) // 15)])
    chosen.update([(22768, 22771), (22628, 22632), (22873, 22876)])
    rows = [[a, int(raw[a]['byte_offset']), int(raw[b]['byte_offset']), -1, -1, -1,
             a, b, raw[a]['time_s'], raw[b]['time_s']] for a, b in sorted(chosen)]
    write_pairs(output / 'ps5-pairs.csv', rows)
    write_pairs(output / 'ps5-pairs-smoke.csv', rows[:8])
    # The three-second sequence uses exactly the original recorder's processed pairs.
    sequence = [(a, b) for a, b in pairs if 9.5 <= float(raw[a]['time_s']) < 12.5]
    write_pairs(output / 'ps5-sequence.csv', [[a, int(raw[a]['byte_offset']), int(raw[b]['byte_offset']),
        -1, -1, -1, a, b, raw[a]['time_s'], raw[b]['time_s']] for a, b in sequence])


def run(output, dataset, mult, quality, cuts, legacy=False, smoke=False, save=False, suffix=''):
    name = f'{dataset}-{mult}x-q{quality}-cut{int(cuts)}-{"legacy" if legacy else "fixed"}{suffix}'
    dest = output / name
    if (dest / 'summary.json').exists():
        print('Already complete:', name, flush=True)
        return
    if dest.exists():
        raise RuntimeError(f'Incomplete output exists: {dest}; preserve it and choose a different output directory')
    exe = ROOT / 'build/Release' / f'FrameLab-benchmark{"-legacy" if legacy else ""}.exe'
    if dataset == 'csgo':
        source = ROOT / 'app/samples/csgo-1080p60.bgra'
        manifest = output / f'csgo-{mult}x{"-smoke" if smoke else ""}.csv'
        w, h = 1920, 1080
    else:
        source = ROOT / 'app/recordings/analysis-15s-84744531/raw.bgra'
        manifest = output / f'ps5-{"sequence" if dataset == "ps5-sequence" else "pairs"}{"-smoke" if smoke else ""}.csv'
        w, h = 2560, 1440
    args = [str(exe), '--input', str(source), '--pairs', str(manifest), '--out', str(dest),
            '--runtime', str(ROOT / 'app/runtime'), '--width', str(w), '--height', str(h),
            '--quality', str(quality), '--multiplier', str(mult), '--cuts', str(int(cuts))]
    if save:
        args.append('--save-all')
    print('Running:', name, flush=True)
    p = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=240,
                       creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    (output / f'{name}.log').write_text(p.stdout + p.stderr, encoding='utf-8')
    if p.returncode:
        raise RuntimeError(f'{name}: {p.returncode}: {p.stdout} {p.stderr}')
    result = json.loads((dest / 'summary.json').read_text())
    print(f'  pair {result["pair_ready_mean_ms"]:.3f} ms; GPU phases {result["phase_gpu_mean_ms"]}; '
          f'PSNR {result["psnr_mean_db"]:.3f}; copies {result["endpoint_copies"]}/{result["outputs"]}', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=pathlib.Path, required=True)
    parser.add_argument('--stage', choices=['prepare', 'smoke', 'sweep', 'sequence', 'repeat', 'quality-clips'], default='smoke')
    args = parser.parse_args()
    output = args.out.resolve()
    prepare(output)
    if args.stage == 'smoke':
        for m in (3, 4):
            for legacy in (True, False):
                run(output, 'csgo', m, 1, True, legacy, smoke=True)
    elif args.stage == 'sweep':
        for dataset in ('csgo', 'ps5'):
            for m in (3, 4):
                run(output, dataset, m, 1, True, legacy=True)
                for q in (0, 1, 2):
                    for cuts in (True, False):
                        run(output, dataset, m, q, cuts)
    elif args.stage == 'sequence':
        for m in (3, 4):
            for legacy in (True, False):
                run(output, 'ps5-sequence', m, 1, True, legacy=legacy, save=True)
    elif args.stage == 'quality-clips':
        for quality in (0, 1, 2):
            run(output, 'ps5-sequence', 4, quality, True, save=True)
        run(output, 'ps5-sequence', 4, 1, False, save=True)
    elif args.stage == 'repeat':
        # Alternating order avoids always favouring the warmed-up second implementation.
        for repeat in range(3):
            for m in (3, 4):
                for legacy in ((False, True) if repeat % 2 == 0 else (True, False)):
                    run(output, 'ps5-sequence', m, 1, True, legacy=legacy, suffix=f'-repeat{repeat}')

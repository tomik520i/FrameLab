"""Local A/B using the same binary/model/input; no screen capture or network."""
import argparse
import csv
import json
import statistics
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("app", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--seconds", type=int, default=24)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    runs = []
    cases = [("old_schedule_1440", ["--nr", "--nr-height", "1440", "--legacy-nr-scheduling"]),
             ("ahead_1440", ["--nr", "--nr-height", "1440"]),
             ("ahead_1080", ["--nr", "--nr-height", "1080"]),
             ("ahead_720", ["--nr", "--nr-height", "720"]),
             ("fg_without_nr", [])]
    for name, options in cases:
        previous = set((args.app / "logs").glob("debug-*"))
        command = [str(args.app / "FrameLab.exe"), "--demo", "--1440", "--fg", "--medium",
                   "--no-vsync", "--seconds", str(args.seconds), "--debug-seconds", str(args.seconds)] + options
        print("START", name, flush=True)
        result = subprocess.run(command, cwd=args.app, timeout=args.seconds + 120)
        if result.returncode:
            raise RuntimeError(f"{name} exited {result.returncode}")
        created = set((args.app / "logs").glob("debug-*")) - previous
        if len(created) != 1:
            raise RuntimeError(f"Expected one diagnostic folder, got {created}")
        folder = created.pop()
        rows = [{k: float(v) for k, v in row.items() if k != "utc"} for row in csv.DictReader((folder / "metrics.csv").open())]
        # Exclude startup/model initialization. This is a synthetic pacing test,
        # not a visual-quality score or proof of scanout FPS.
        rows = [r for r in rows if r["time_s"] >= 8 and (not options or r["nr_frames"] > 30)]
        if len(rows) < 5:
            raise RuntimeError(f"Too few warmed samples in {name}")
        keys = ["input_fps", "changed_fps", "output_submitted_fps", "generated_fps", "os_confirmed_fps",
                "app_latency_mean_ms", "app_latency_p95_ms", "gpu_percent", "gpu_power_w", "vram_used_mb",
                "cpu_system_percent", "nr_worker_ms", "nr_prepare_gpu_ms", "nr_inference_gpu_ms",
                "nr_finish_gpu_ms", "nr_ready_to_take_ms"]
        record = {"name": name, "command": command, "folder": str(folder), "samples": len(rows),
                  "mean": {k: statistics.mean(r[k] for r in rows) for k in keys}}
        frames = [{k: float(v) for k, v in row.items() if k != "utc"} for row in csv.DictReader((folder / "frames.csv").open())]
        intervals = sorted(r["content_interval_ms"] for r in frames if r["time_s"] >= 8 and r["new_content"] and r["content_interval_ms"] > 0)
        record["frame_interval_ms"] = {"median": statistics.median(intervals), "p95": intervals[int((len(intervals)-1)*.95)], "max": max(intervals)}
        runs.append(record)
        (args.output / "results.json").write_text(json.dumps({"note": "1440p synthetic motion, 30 unique input / FG 2x, same GPU and binary, warmup excluded; submitted FPS is not physical monitor FPS", "runs": runs}, indent=2), encoding="utf-8")
        print(name, json.dumps(record["mean"]), flush=True)


if __name__ == "__main__":
    main()

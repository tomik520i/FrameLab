# Full pipeline recordings (preview.15)

Choose **Record 15 s analysis** in the recording tab. The source keeps playing.
Recording first buffers losslessly compressed frames in bounded RAM, then writes
compressed `.flp` archives and creates playable MP4 clips in the background.
The app refuses another take while the previous one is saving/exporting.

The recording folder contains:

- `*_raw-indexed.mp4` and `*_generated-indexed.mp4`: source and generated frames.
- `*_before.mp4`: output before enhancement, including any frame generation.
- `*_after.mp4`: the same frames after enhancement at the selected effect strength.
- `*_final.mp4`: clean output with the actual displayed Czech subtitles burned in.
- `*_before-after-subtitles.mp4`: synchronized three-column comparison.
- `subtitles.cs.srt` and `subtitles.json`: displayed subtitle times, Czech text and English source.
- `clips.json`, `session.json`, `pipeline-settings.json`, `settings-timeline.jsonl`
  and the subtitle/NR settings files: recording IDs, clip IDs, settings and hashes.
- `frames.csv`, `output-frames.jsonl`, `presents.csv` and display confirmations:
  original timing and explicit pixel references for comparison.

The player uses the same output composition code as the recorder. The recorded
image covers the entire source, without the dashboard, divider, OCR outline or
monitor/window cropping. It does not record the desktop or audio. When enhancement
or translated subtitles are off, those stages reuse the preceding pixels; no
effect or translation is invented for the recording. A subtitle arriving late is
recorded with that actual delay, including its hold and disappearance.

MP4s are high-quality **lossy previews**. The compressed FLP archives retain exact
BGRA bytes; `tools/recording_archive.py` with `app/FrameLabPixels.dll` provides
random-access decoding without raw scratch files. The before/after/final previews
share the same output timeline, sampled at 60–240 FPS with frame holds. Raw and
generated indexed previews space their frames evenly: use the CSV for their exact
timing. None of these files proves physical monitor scanout FPS.

Recording readback/compression and later export have a performance cost. The RAM
budget is capped at 16 GiB, keeps 12 GiB free for the system and also respects disk
space. An overloaded recorder stops and reports an incomplete take instead of
growing indefinitely. No uncompressed `.bgra` files are created by the new recorder.
Compressed originals remain available if export fails. `export-progress.json` and
encoder logs describe progress/errors. Existing recordings are not rewritten and
remain compatible with the old exporter.

The desktop **FrameLab** shortcut uses `--resume`, restoring the last source,
FG, enhancer/subtitle enable states, audio/settings files, window and diagnostic
logging state. `runtime-session.txt` is local user state; do not bundle it in a
public release.

## Destination and incomplete takes

Choose a folder in Recording; recording-folder.txt remembers it. The free-space estimate uses that destination. Allow 36 GiB free for a 1440p full-effects take as a practical recommendation, not a fixed requirement. RAM stays bounded.

GPU readback is checked before timing out: a completed copy is accepted even after a CPU stall. Only a still-busy copy older than two seconds stops the take. A 180 ms delayed-poll GPU regression verifies the former false-timeout case. This does not guarantee 15 seconds on every workload.

If an output references missing pixels, the exporter ends synchronized clips at the first missing row, records incomplete status and missing references, and leaves the original timeline unchanged. RAW/generated indexed rates still use the original recording duration. Caption SRT ends at the usable prefix.

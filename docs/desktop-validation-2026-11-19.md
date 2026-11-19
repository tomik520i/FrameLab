# Desktop validation — preview.14, 19 November 2026

## Preview.14 update

Same desktop and enabled Avast as below. Neural OCR is now bundled. All 11 CTest checks and five offline download/integrity tests pass. Fragment-boundary tests cover completed sentences, speaker labels, generation changes, timeout, disabled joining and date cleanup. Settings persistence covers Q5 ID 3 without remapping existing Q8 ID 2.

The private 48.25-second portrait video was replayed through real RapidOCR + Q3 + VFG 2×. Its ending no longer gained the observed invented continuation; the next fragment was translated with the preceding context. Grammar and speaker gender remain model limitations. The downloaded personal clip, frames and dialogue traces are not redistributed. [Recognition comparison and scope](subtitle-ocr.md).

Q5_K_M: publisher download complete, 5,131,409,728 bytes, SHA-256 verified. On the owner’s subsequent request Q5 was activated: the runtime reported Q5_K_M and all 33/33 layers on GPU. This was a basic startup check, not a controlled quality or speed benchmark. Existing 300-text Q3/Q8 measurements below remain the preview.12 baseline.

## Retained preview.13 validation


## System and scope

Ryzen 9 5900X (12 cores / 24 threads), 64 GB RAM, Gigabyte X570 AORUS PRO,
Windows 11 Pro build 26200, RTX 5070 Ti (16 GB), NVIDIA driver 617.14.
The tests used bundled video playback and actual OCR fixtures. They did not
capture the desktop or measure new PS5/capture-card latency. Avast remained enabled.

## Application checks

- Release build and all **10 CTest checks pass**.
- Five offline asset tests pass: interrupted/resumed download, SHA rejection,
  archive traversal rejection, repair retaining the previous runtime, and readiness
  invalidation when model/runtime files change or disappear.
- Native D3D overlay test verifies actual ImGui drag events, independent OCR crop,
  clean fullscreen pixels and disabled editing in fullscreen. A cropped source
  image extending beyond the window retains **all 18 test lines** inside the visible
  viewport, with zero white caption pixels outside. The line-count fixture uses
  solid capital stems; dots/antialiased curves can split one line into two pixel bands.
- Layout tests cover edge fallback, multiline bounds, all placement presets,
  cropped/portrait/4K geometry, settings persistence and migration. Font tests
  separately cover Czech glyphs and high-resolution font atlases.
- Windows English OCR was actually installed through approved Windows servicing.
  The component reports Installed; no restart was required on this machine.
  The app detects readiness. Its explicit install button uses the same built-in
  Add-WindowsCapability command with UAC; a second fresh-OS button-install trial
  was not performed after the component was installed.
- Native production OCR → translation tests pass for **CPU, CUNI Q3 and CUNI Q8**:
  image text “The door is locked.” produces “Dveře jsou zamčené.” They also check
  blank clearing, source reset and disabled state.
- A delayed test worker verifies stale-result rejection while a translation is
  in flight: blank source and reset cannot republish the old translation. This
  test worker is never included in the app's translation runtime.
- Native model lifecycle test loads Q8, cancels a load, reloads and unloads
  asynchronously. GPU workers verify complete GPU offload; silent CPU fallback
  is rejected. The physical-VRAM guard is enabled.
- Actual NR GPU pixel test at 720p passes, including correct input/result pairing,
  a single in-flight job and no duplicate publication.
- The **new application** runs actual OCR and CUNI Q3/Q8 while **VFG 2× Medium and
  NR 65% / 720p** process the bundled 720p OCR video. Diagnostic exports verify
  initialized VFG, completed NR output, loaded model and the recognized/translated
  line. Own-rendered screenshots show independent placement and a two-line clean
  fullscreen caption above the lower edge. These are functional tests, not latency
  benchmarks. No desktop screen capture was used.

## Additional local-video and multi-pass checks

- Clocked Media Foundation playback tested with an 8-second **24 FPS H.264/AAC
  MP4**, a silent MP4 and an explicitly selected Windows audio endpoint. Tests
  verify native timestamps, red-top/blue-bottom pixel orientation, actual signal
  in this process's audio session, pause of the clock and video callbacks, paused
  seeking, resume, EOF, looping, shutdown and missing-file errors. The Windows
  Video Processor MFT handles decoder padding before delivering packed RGB32.
- Actual application own renders verify **local MP4 + audio topology + VFG 2× +
  NR two passes + Windows OCR / CPU translation** together. This is a functional
  check, not a physical audio/video synchronization or long-film benchmark.
- GPU chaining tests pass with **ShortFuse: two passes at 360p and four at 720p**,
  and **original NVIDIA NR 310.8: two passes at 720p**, all on RTX 5070 Ti. Twelve
  submissions include resets and motion disabled. Final results exactly match an
  independent CPU readback / re-upload chain (RGB-byte MAE **0**). Reference
  frames match the submitted input. This proves sequential processing, not
  improved quality. Separate feature/history state is maintained per layer.
- NR settings retain one pass for version-1 files, round-trip version-2 pass
  count/strength, and bound invalid counts. Existing ImGui strength/split/fullscreen
  tests and all ten CTests still pass. Changing passes rebuilds the enhancer.
- Files use their own native playback timing. The existing bundled benchmark
  decoder and its FPS ceiling are unchanged. First audio/video tracks only;
  SDR H.264/AAC is the tested baseline. No DRM, HDR tone mapping or external /
  embedded subtitle-track rendering. Decoder/audio timing shares a clock;
  subsequent FG/NR/display delay is not automatically compensated.

## Controlled coexistence measurement

Both CUNI quantizations completed **300 exact-text requests plus 3×60 paced
requests**, with their own adjacent unloaded controls. All 600 outputs match the
separately reviewed preflight after stripping surrounding whitespace. No empty or
truncated output occurred. All four warmups per model verify full GPU loading.
Application telemetry was current throughout; NVML sampled the whole GPU at 1 Hz.

The controlled benchmark used the unchanged preview.12 executable, verified against
its published SHA-256 `228ba462bd8a96015351391292522d1e56db2ff4a54aa41cd152d291e7fe0916`
(source commit `9b0b3dc54e7ce43f4aaa26a0e37373f46c3fcc54`). Its existing CPU OCR path,
VFG 2× Medium and NR 65% / 720p remained active over the same 1080p30 CS:GO loop.
A separate local CUNI worker processed exact corpus text. This isolates translator
load consistently; it does not measure preview.13 subtitle delay end to end.

| Metric | CUNI Q3_K_M | CUNI Q8_0 |
|---|---:|---:|
| Paced translation median | 212 ms | 360 ms |
| Paced translation p95 | 479 ms | 855 ms |
| Requests above 2 seconds | 0/180 | 0/180 |
| Additional resident GPU memory | 3.81 GiB | 7.62 GiB |
| Mean output change against own controls | −1.95% | −1.96% |
| Acceptable translations, assistant assessment | 271/300 | 271/300 |
| Major translation errors | 2 | 5 |

Q3's median request took about 147 ms / 41% less time. Quality grades are assistant
judgments, not a blinded independent human study. Higher quantization did not
produce a clear quality gain on this corpus. No claim is made about physical
monitor FPS or total subtitle latency; OCR sampling/stabilization are additional.

[Interactive evidence, all pairs, raw data and settings](https://opicebot.cz/framelab/translation/)
· [Summary JSON](performance/2026-11-19-cuni/summary.json)
· [Test systems and scope](test-system.md)

## Distribution

The public ZIP is assembled from the reviewed complete portable base with current
executables, workers, model catalog and EN/CZ guides. English is the default.
GPU weights and the portable GPU inference runtime remain optional verified
publisher downloads. The private handoff, local NR model DLLs and developer GPU
model cache are excluded. Source archives preserve the release's exact Git tree.

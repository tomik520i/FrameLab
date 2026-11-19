# Local Neural Rendering experiment

**Licensing correction, 2026-11-18:** see `NR-LICENSING-STATUS.txt`. ShortFuse is a community modification of NVIDIA's runtime; Veyra provides an integration reference. Redistribution rights for these exact NR DLLs have not been established. Updated launchers leave AI off. The comparison gallery uses previously exported pixels without running NR.

This local FrameLab variant adds a real Feature-18 neural-rendering path based on the
community integration in [Veyra-NRVideo](https://github.com/Likely7/Veyra-NRVideo),
revision `33dfdf71197e3056e250465150eb9541d0bb24ce`. It is an experimental video/capture
post-process, not an official NVIDIA game integration. Preview.12 includes the SDK-free NR adapter and import wizard. External NR model DLLs are not bundled. [Complete installation](enhancer-install.en.md) / [Česky](enhancer-install.cs.md).

## Run

In the preview.12 full bundle, use `1 - Spustit FrameLab.cmd`. It starts the included CS:GO
1080p60 video with enhancement disabled by default. `Ukazka - Spider-Man.cmd` opens a second combat
fixture. Double-clicking the EXE starts with enhancement off; source/device selection
still works normally. Own video: `FrameLab.exe --video "C:\clip.mp4"`.

- F6: enhancer on/off. F7: original/enhanced/split view. F8: freeze the comparison.
  F11: full screen. F1: dashboard.
- AI / DLSS 5 tab: strength 0–100%, divider, working resolution, motion estimation.
- Strength 0% is the exact original draw; strength 100% uses the full neural residual.
- The existing evidence was produced at 720p inference and 65% / 100% strength.
  Lower inference resolution trades detail for speed.
- The runtime defaults to ShortFuse's community-modified 310.9.16 variant. The optional
  original variant is intended for RTX 50 and was not validated on this RTX 4050.
- Startup loads the model in a worker while raw preview continues. Failure disables
  enhancement and shows the error. Source preview does not require an NR runtime.

## Actual pipeline and limits

FrameLab retains its D3D11 capture/FG path. Shared textures and an input fence connect
it to a separate D3D12 device on the same adapter. Our locally built host lives at
`app/nr-host/nvngx.dll`; it is not an NVIDIA driver DLL. Model files are separate and
are not modified by this implementation; the SF file was already modified upstream.
No executable patch, IAT hook or driver
installation is performed. The external runtime ABI and the known initialization
identifier follow Veyra's experimental Feature-18 contract.

Each operation downsamples the source to the selected inference height (rounded
down to multiples of eight and never above source resolution), estimates coarse
motion with GPU block matching, supplies **flat depth**, and runs preset 2. Output
is `clamp(original + enhanced_low_resolution - input_low_resolution)`, then the UI
blends it by the user's strength. This can alter lighting, texture and appearance;
it does not reconstruct engine geometry or provide ground-truth motion/depth.
Artifacts, temporal instability and changed HUD details remain possible. SDR BGRA8
is supported; this is not an HDR pipeline or a generative diffusion video model.

Only one inference is in flight. A/B retains the exact input paired with that result;
it never compares an older enhanced frame with a newer live reference. The live NR
path does no pixel readback. The worker waits for GPU completion; the UI does not.
Queues remain bounded, and slow processing can drop frames and adds latency. There
is no promised FPS increase. Displayed NR milliseconds include CPU submission and
GPU wait, not a GPU timestamp measurement.

History resets on source/settings changes, input gaps, backwards source timestamps,
large sampled scene differences and dropped output bursts. The scene heuristic is
not a perfect cut detector. With motion estimation disabled every frame resets.
NR runs **after optional frame generation**; Original then means the frame entering
the enhancer. Existing RAW/FG analytical recordings do not include NR or strength.

Settings are in `app/nr-settings.txt`; on/off is intentionally not persisted.
Runtime diagnostics are in `app/nr-cache/nr-host.log`. Delete neither model nor cache
while an instance is running. Inference has a 20-second GPU timeout; model loading
and driver internals are outside the UI thread but cannot always be cancelled immediately.

## Samples

`app/samples/action.mp4`: 180 original 1920×1080 frames at 60 FPS, seconds 25–28 of
[Xiph CSGO.y4m](https://media.xiph.org/video/derf/twitch/y4m/CSGO.y4m), encoded H.264
CRF18. It loops for three seconds. The download's complete Y4M frame markers were
validated; the incomplete remainder was not used. No frame interpolation.

`combat.mp4`: three seconds of the original left image from FrameLab's public
`113768562` source-aligned comparison, cropped to 960×540 and restored from 4× slow
motion to normal speed. Already lossy/downscaled; the generated right pane is not
included. The two `.source.json` files preserve sources, transformations and hashes.
Videos are silent test fixtures; game assets retain their owners' rights.

Native Windows Media Foundation decodes MP4, so playback needs no Python or huge
uncompressed BGRA file. The original BGRA sample path remains a fallback.

## Build and validation

Windows x64, MSVC 2022, Windows SDK, CMake and Ninja. Use `tools/setup-deps.ps1 -BuildOnly`. No NGX SDK or static library is needed for the adapter. Runtime model files are optional local imports, not build dependencies. See the installation guide for the pinned external archive and rights limitations.

From an x64 Developer Command Prompt:

```bat
cmake -S . -B build-nr -G Ninja -DCMAKE_BUILD_TYPE=Release -DFRAMELAB_EXPERIMENTAL_NR=ON
cmake --build build-nr --parallel 2
ctest --test-dir build-nr --output-on-failure
build-nr\nr_controls_test.exe
```

Optional `FRAMELAB_APP_DIR` selects another portable output directory.
`FRAMELAB_EXPERIMENTAL_NR=OFF` omits the host; trying to enable NR
without a host shows a load error. Real inference tests are separate from default
CTest and require a runtime with the applicable authorization to use it.
Source sample reconstruction details are in the provenance files; the portable
package already includes the ready-to-play MP4s.

Validated locally on RTX 4050 Laptop 6 GB, driver 610.62: real Feature-18 creation and
evaluation, full-opacity output, exact paired input on 12 changing submissions,
single-in-flight enforcement, repeated-result rejection, reset/motion-off paths,
1440p source at 720p inference, and 7 offscreen UI rendering cases including zero,
half and full strength. All five existing CTest tests passed. Pixel changes establish
that inference ran, not that every change improves image quality. Hardware timing
is a smoke-test observation, not a sustained benchmark or a general RTX guarantee.

Preview.12 validation (2026-11-19): the SDK-free parameter implementation completed 12 real GPU submissions on RTX 4050 Laptop with exact paired inputs, nonzero neural output and full alpha. Import wizard validates both original DLL hashes without executing Veyra. This verifies compatibility of the host, not a license grant or an image-quality benchmark.

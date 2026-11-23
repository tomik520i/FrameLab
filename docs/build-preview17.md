# Build and package preview.17

## SDK-free NR adapter and guided import

The NR adapter now builds from FrameLab source with Windows D3D libraries only. No NGX headers/static library or external NR model DLL is needed to build. `tools/Install-Enhancer.ps1` imports the pinned external archive locally; it never enters release packaging. Use `--setup <built app/FrameLab-Setup.exe> --nr-host <built app/nr-host/nvngx.dll>` with package_release.py.

For running the application, use the complete release ZIP and [installation guide](../START_HERE.md). These instructions are for developers.

## Build the public application

Windows x64; Visual Studio 2022 C++ Build Tools with Windows SDK, CMake, Git and Python **3.11 x64** for dependency preparation. No CUDA compiler is needed.

```powershell
git clone https://github.com/tomik520i/FrameLab.git
cd FrameLab
git checkout v0.2.0-preview.17
.\tools\setup-deps.ps1 -BuildOnly
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DFRAMELAB_EXPERIMENTAL_NR=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Output: `app/FrameLab.exe`. The build-only CI artifact has no models, videos, Python or NVIDIA runtime; it is not the complete end-user package. The manually attached source archive includes the exact ImGui source; a Git checkout or GitHub automatic source archive fetches the pinned ImGui revision in setup-deps.ps1.

## Prepare external components

The application binary and external files have separate build steps. The easiest local development layout is to extract the complete matching release and replace its `app/FrameLab.exe`, `app/FrameLab-Setup.exe` and `app/nr-host/nvngx.dll` with your own build. Keep component licenses, sample metadata and all folders.

To recreate dependencies directly from publishers, use:

```powershell
python tools/prepare_portable_dependencies.py
.\tools\prepare_msvc_dependencies.ps1
python tools/prepare_translation.py
python tools/prepare_ocr.py --output app/ocr
```

The first two prepare `deps/portable` (official NVIDIA VFG wheel, CPython, export libraries and Microsoft runtime); they do not provide DLSS NR. The translation command prepares `app/translation`, downloading only pinned publisher archives/wheels with SHA-256 checks. It converts the OPUS-MT Marian model to CTranslate2 int8. See `translation-components.json` for exact versions, source URLs and original licenses. Python 3.11 x64 plus pip is needed for this **developer step**, not to use the ready-made application.

The exact sample MP4s, `.source.json` files and synthetic OCR fixture are in the full release's `app/samples`; copy that directory into your build. Their transformations and content rights are documented in `sample-provenance.md`. Building from code alone does not synthesize replacement gameplay or silently download videos.

## RIFE and multilingual components

`setup-deps.ps1 -BuildOnly` also prepares the pinned ONNX Runtime / DirectML headers. It does not supply RIFE weights. Use the verified runtime from the full release or follow [RIFE export instructions](RIFE-DIRECTML.md). Run `python tools/prepare_ocr.py --output app/ocr` for all 12 pinned recognition families. `prepare_translation.py` also stages the language catalog and multilingual helper. Large GPU translation weights are optional in-app downloads.

Run `python tools/render_current_guides.py` and `python tools/render_multilingual_report.py` after documentation changes. The homepage is maintained directly in `website/index.html`; the old release homepage generator is intentionally disabled.

## Package a release

Stage the reviewed source tree, build the NR adapter **ON**, and pass the complete component directories explicitly:

Use the current `tools/package_release.py` command with explicit paths, starting from a clean extracted public full release (no imported NR DLLs, GPU models, or GPU runtime):

```powershell
python tools/package_release.py --base D:/clean-public-base --application app/FrameLab.exe --build-cache build/CMakeCache.txt --nr-host app/nr-host/nvngx.dll --setup app/FrameLab-Setup.exe --ocr app/ocr --media app/media/vlc --rife app/runtime/rife --gta-sample app/samples/gta.mp4 --output dist/preview17 --version 0.2.0-preview.17 --commit HEAD
```

Commit reviewed source first; the packager requires the exact clean Git tree. Use a fresh output directory. It refreshes binaries, all helpers, language catalogs, OCR/RIFE/media runtimes, guides and notices. It validates pinned hashes, creates both the full ZIP and exact source ZIP with ImGui, then writes archive checksums/provenance. No upload occurs. The older `package.ps1` workflow does not assemble the complete preview.17 multilingual/RIFE set.

Do not publish local logs, settings, recordings, caches, credentials, external NR model binaries or GPU translation downloads. The public ZIP deliberately includes CPU translation, OCR and RIFE; optional larger models remain explicit downloads with publisher terms.

## Optional experimental NR source

`FRAMELAB_EXPERIMENTAL_NR` defaults ON. The host uses FrameLab parameter storage and Windows D3D libraries; no NGX SDK download or static library is required. It does not provide or license an NR model runtime. No external NR DLL enters either release archive. See [installation](enhancer-install.en.md), [licensing status](NR-LICENSING-STATUS.txt) and [technical notes](experimental-nr.md).

## Media runtime and current validation

Run `python tools/prepare_media.py --output app/media/vlc` for the pinned LibVLC runtime. Its provenance must match docs/media-runtime.json. `package_release.py` requires `--media`, `--ocr` and `--gta-sample` as explicit paths. Supply a verified clean public base without external NR model DLLs, an NR-enabled current executable, the built NR host/setup helper, and the exact clean Git commit. It excludes personal settings, logs, model caches and external NR DLLs. Generate guides with `python tools/render_install_guides.py` first.

The GTA source demo is available in the full release under app/samples/gta.mp4; its hash is docs/sample-gta-source.json. Do not substitute the comparison video for original input. Preparation uses Python 3.11 x64. Neither Python nor a compiler is required by the end user.

Tests include `recording_output_test`, `analysis_recorder_test`, `subtitle_profiles_test`, `subtitle_style_test`, media track fixtures, and the CTest targets. `tests/recording_export.py` validates actual exported caption pixels, SRT and the incomplete-tail case. `tests/translation_eos.py` exercises the OPUS source-EOS handling. `tests/create_media_fixture.py` produces media test input; `tests/local_video_tracks.cpp` checks track selection and local playback. Inspect each tool's CLI for explicit paths.

See [release notes](release-preview17.md) and [published evidence](../examples/2026-11-20/README.md). Q5 is used in the live caption evidence; this is not a universal model-quality ranking. The optional NR runtime remains separate from source/build and may require authorization under its publisher terms.

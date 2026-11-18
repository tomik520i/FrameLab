# Build and package preview.10

For running the application, use the complete release ZIP and [installation guide](../START_HERE.md). These instructions are for developers.

## Build the public application

Windows x64; Visual Studio 2022 C++ Build Tools with Windows SDK, CMake, Git and Python **3.11 x64** for dependency preparation. No CUDA compiler is needed.

```powershell
git clone https://github.com/tomik520i/FrameLab.git
cd FrameLab
git checkout v0.2.0-preview.10
.\tools\setup-deps.ps1 -BuildOnly
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DFRAMELAB_EXPERIMENTAL_NR=OFF
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Output: `app/FrameLab.exe`. The build-only CI artifact has no models, videos, Python or NVIDIA runtime; it is not the complete end-user package. The manually attached source archive includes the exact ImGui source; a Git checkout or GitHub automatic source archive fetches the pinned ImGui revision in setup-deps.ps1.

## Prepare external components

The application binary and external files have separate build steps. The easiest local development layout is to extract the complete matching release and replace only its `app/FrameLab.exe` with your own build. Keep component licenses, sample metadata and all folders.

To recreate dependencies directly from publishers, use:

```powershell
python tools/prepare_portable_dependencies.py
.\tools\prepare_msvc_dependencies.ps1
python tools/prepare_translation.py
```

The first two prepare `deps/portable` (official NVIDIA VFG wheel, CPython, export libraries and Microsoft runtime); they do not provide DLSS NR. The third prepares `app/translation`, downloading only pinned publisher archives/wheels with SHA-256 checks. It converts the OPUS-MT Marian model to CTranslate2 int8. See `translation-components.json` for exact versions, source URLs and original licenses. Python 3.11 x64 plus pip is needed for this **developer step**, not to use the ready-made application.

The exact sample MP4s, `.source.json` files and synthetic OCR fixture are in the full release's `app/samples`; copy that directory into your build. Their transformations and content rights are documented in `sample-provenance.md`. Building from code alone does not synthesize replacement gameplay or silently download videos.

## Package a release

Stage the reviewed source tree, build NR **OFF**, and pass the complete component directories explicitly:

```powershell
.\tools\package.ps1 -Version 0.2.0-preview.10 -Complete -Translation
```

This checks required samples, translation files, NVIDIA/Microsoft hashes and signatures, and rejects NR DLLs. It includes the current guides and launchers. Choose a fresh output directory if files already exist. Source ZIPs must include all modified/new source files, tests, notices, build/setup scripts and the exact ImGui source. Do not publish logs, settings, personal recordings, model caches, credentials or NR binaries.

`tools/package_release.py` can instead construct the full archive from a reviewed existing complete portable directory, a freshly built NR-OFF executable and an exact Git commit. It refreshes documentation and sample notices, removes local logs/settings/caches from the archive, checks required components and produces SHA-256 metadata. This path was used for preview.10 to reuse the already verified full component set. Run `python tools/package_release.py --help` for its explicit paths; nothing is uploaded by this tool.

## Optional experimental NR source

`FRAMELAB_EXPERIMENTAL_NR` defaults OFF. `python tools/setup_nr_experiment.py` prepares pinned public NGX headers/static library only. Building with `-DFRAMELAB_EXPERIMENTAL_NR=ON` compiles the local adapter, but **does not provide or license an NR model runtime**. No NR model binary is attached to the release. See [licensing status](NR-LICENSING-STATUS.txt) and [technical experiment notes](experimental-nr.md). Do not substitute a public SR/RR/FG DLL for NR.

## Validation scope

Seven automated tests pass. Actual Windows English OCR + the bundled EN-CS model was tested, including blank clearing, reset and disable. A moving synthetic OCR clip ran with NVIDIA VFG 2×/3× and Optical Flow 4× on RTX 4050 Laptop. A full ZIP was extracted into a fresh directory with spaces, launched without system Python on PATH, and displayed Czech translation with VFG 3×. This is not a dialogue-quality benchmark or a guarantee for every GPU/capture card. No new NR inference was performed after the licensing audit; high NR resolutions and a physical capture card were not retested on the laptop.

# Complete portable bundle: components and provenance

The **complete Windows x64 bundle** contains the FrameLab application, an optional NVIDIA VFG component and an isolated clip-export environment. Drivers are not bundled. Extract everything and use `Start FrameLab - NVIDIA.cmd` for NVIDIA VFG or `Start FrameLab - Optical Flow.cmd` for the alternative backend. Both enable generation; first-run FG defaults remain Medium / 2×; the complete preview.12 package starts in Czech. Existing saved preferences still apply.

## NVIDIA component

The five unmodified DLLs come directly from NVIDIA's public package:

- [Official package index](https://pypi.nvidia.com/nvidia-vfx/).
- [nvidia_vfx-0.2.0.0-cp311-cp311-win_amd64.whl](https://pypi.nvidia.com/nvidia-vfx/nvidia_vfx-0.2.0.0-cp311-cp311-win_amd64.whl), SHA-256 `5aaf6a42bc6b6dbbf52fcb714194c994a6893cbbf7ada38bc2165a1f83e4a6fc`.
- Runtime: NVIDIA VFX SDK 1.3.0. Files: `cudart64_12.dll`, `NVCVImage.dll`, `nvngxruntime.dll`, `NVVideoEffects.dll`, `nvVFXVideoFrameGeneration.dll`.
- Exact DLL hashes, sizes and publisher origin are in `app/runtime/manifest.json`. Packaging checks every hash and valid NVIDIA Authenticode signature.
- [NVIDIA VFG reference](https://docs.nvidia.com/maxine/vfx/latest/Filters/VideoFrameGeneration.html): Windows x86-64 supports **Ada and Blackwell architecture GPUs**, including RTX 40/50. The runtime is not tied to an individual RTX 5070 Ti. RTX 5070 Ti and RTX 4050 Laptop have been tested. It is not a general RTX 20/30 VFG runtime.

The original wheel's license files are included under `licenses/nvidia-vfg`. NVIDIA files remain proprietary under their own terms and are **not relicensed under FrameLab's GPL**. The bundled *Product-Specific Terms for NVIDIA AI Products* (2025-05-05), grant 1.1.3, permits sublicensing/distribution as part of a Customer Product, subject to its conditions. The corresponding agreement defines Customer Product as including value-added functionality and Compatible Application as not adversely affecting other software components. FrameLab provides capture, processing, presentation, audio routing and diagnostics. NVIDIA binaries are unchanged, separately identified and removable; the Optical Flow route remains available without them. The optional add-on is intended for use with FrameLab, not presented as an independent NVIDIA SDK product.

Preserve NVIDIA licenses/notices, applicable use restrictions and proprietary status when passing this bundle on. NVIDIA does not sponsor or endorse FrameLab. No credentials, NVIDIA account, SDK installer, Python interpreter or model download is required to run the NVIDIA component from the complete bundle; the compatible installed NVIDIA driver is still required.

## Portable export environment

The complete package also carries unmodified, Microsoft-signed `msvcp140.dll`, `vcruntime140.dll` and `vcruntime140_1.dll` in `app/runtime`, beside the NVIDIA DLLs. These are the x64 Visual C++ 2022 release redistributables needed by those DLLs, taken from the installed Visual Studio `VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT` directory. The restricted DLL loader searches that runtime directory; no system-wide Visual C++ installer is needed. `licenses/microsoft-vc-manifest.json` records exact versions and hashes. Microsoft components retain their own terms, not the application's GPL. See the [Microsoft redistribution list](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution) and [deployment documentation](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170). The embedded Python distribution separately includes its own Microsoft runtime DLLs.

- **CPython 3.11.9, Windows x64 embedded**: [official ZIP](https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-amd64.zip), PSF license in `tools/python/LICENSE.txt`; [corresponding source](https://www.python.org/ftp/python/3.11.9/Python-3.11.9.tar.xz).
- **Pillow 13.0.0**: official CPython 3.11 Windows x64 wheel, SHA-256 `d7710eb676e9bb68965cb045be0d333b4c0d7f9dfbaa7fbf121385b1588683d3`; [source and wheel](https://pypi.org/project/pillow/13.0.0/#files). Its license and bundled-library notices are retained in `deps/python/pillow-13.0.0.dist-info`.
- **imageio-ffmpeg 0.6.0**: official Windows wheel, SHA-256 `02fa47c83703c37df6bfe4896aab339013f62bf02c5ebf2dce6da56af04ffc0a`; [source and wheel](https://pypi.org/project/imageio-ffmpeg/0.6.0/#files). Package license and notices remain in its `dist-info` directory. Its unmodified **FFmpeg 7.1** executable is used as a separate command-line tool. [FFmpeg source](https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz), [upstream build/source instructions](https://github.com/imageio/imageio-ffmpeg/tree/v0.6.0/binaries), [FFmpeg legal information](https://ffmpeg.org/legal.html). The executable's build/license output is retained in the bundle's `licenses` directory.

The app selects `tools/python/python.exe` when present and otherwise falls back to `python` on PATH, preserving older minimal-package behavior. The embedded interpreter does not install into Windows or alter PATH. That interpreter is used for diagnostic clip export. A separate isolated interpreter under app/translation/python serves offline OCR translation. Captured audio is still not recorded.

## Source and other components

FrameLab's corresponding source ZIP accompanies every binary version. The application source remains GPL-3.0; see `LICENSE` and `THIRD_PARTY_NOTICES.md` for application dependencies. Downloaded runtime and interpreter binaries are release assets, not stored in Git history. Per-component publisher URLs and archive hashes are retained in `licenses/components.json`. No personal preferences, local sessions, credentials or recorded raw pixels are included.

## Preview.10 offline OCR translation and samples

The full preview.12 archive also contains Windows-independent model/interpreter files under `app/translation`: Helsinki-NLP OPUS-MT en-cs opus-2019-12-18, converted to CTranslate2 int8 (CC BY 4.0); CTranslate2 4.9.0 (MIT), SentencePiece 0.2.3 (Apache-2.0), NumPy 2.3.5 (BSD and bundled notices), PyYAML 6.0.3 (MIT), and CPython 3.11.9 (PSF). Original notices, model card, attribution, conversion description and publisher hashes are preserved. Windows.Media.Ocr remains an installed Windows feature; no Windows component is redistributed. See [translation provenance](translation-components.json) and [implementation](local-subtitles.md). No STT or network translation is used.

All three sample MP4s are included in `app/samples`. See [sample provenance and content rights](sample-provenance.md). Gameplay is not relicensed as GPL.

DLSS NR DLLs and the NR host are excluded from the public executable/package. [NR licensing audit](NR-LICENSING-STATUS.txt) records why a valid NVIDIA signature or Veyra GPL notice is not an NR redistribution grant. Preserving notices is not a blanket certification of all third-party uses or GPL/proprietary combination issues.


Preview.12 also includes the GPL FrameLab NR adapter and the hash-checking PowerShell import wizard. The adapter contains no NVIDIA NGX static library. External NR model DLLs are not in this package: [guided setup and exact external source](enhancer-install.en.md). External hosting and user import do not establish usage/redistribution rights.

# Component provenance

- Optional English subtitle entity filter: [dslim/distilbert-NER](https://huggingface.co/dslim/distilbert-NER), Apache-2.0; quantized ONNX conversion from [onnx-community/distilbert-NER-ONNX](https://huggingface.co/onnx-community/distilbert-NER-ONNX/tree/3a19fe9404a4469d91aa3d551558a97f68872f67). Model revision, file hashes, Apache license and attribution are bundled in `app/translation/filter`. Hugging Face tokenizers 0.22.2 (Apache-2.0) and its license are bundled in `filter/deps`; inference uses the existing MIT-licensed ONNX Runtime on CPU. This component recognizes names; it is not a translation model. `tools/prepare_entity_filter.py` pins and verifies the model and Windows tokenizer wheel.

Portable local-file player (2026-11-20): unmodified VideoLAN VLC **3.0.24**
Windows x64 libraries and plugins, downloaded from the official VideoLAN archive.
The LibVLC API library is LGPL-2.1-or-later; VLC/plugins also contain GPL and
other separately licensed components. Original `COPYING.txt`, `AUTHORS.txt`,
`README.txt` and source information accompany `app/media/vlc`. Exact archive
URL, SHA-256 and corresponding source URL are in `docs/media-runtime.json`.
FrameLab dynamically loads these shared libraries and does not modify them.
See [local video playback](docs/local-video.md). Distribution must retain the
upstream notices and corresponding-source access for the included build.

Preview.14 optional/bundled OCR: [RapidOCR](https://github.com/RapidAI/RapidOCR)
1.4.4, Apache-2.0, with unmodified PP-OCRv4 detection and PP-OCRv5 English
recognition ONNX weights from RapidAI / [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
(Apache-2.0). Exact wheel/model URLs and SHA-256 are in `docs/ocr-components.json`.
Original dependency licenses remain in `app/ocr/deps`; model attribution and
provenance accompany `app/ocr`. ONNX Runtime MIT; OpenCV Apache-2.0; NumPy/Pillow
BSD-style; Shapely BSD; PyYAML/pyclipper/six MIT; tqdm MPL-2.0/MIT; flatbuffers
Apache-2.0; protobuf/colorama BSD; packaging Apache-2.0/BSD. Python is shared with
the bundled translation runtime. No OCR model downloads occur during playback.

Local NR licensing correction (2026-11-18): the SF file was already modified by
ShortFuse upstream; copying it unchanged from Veyra does not make it an unmodified
NVIDIA original. Redistribution rights for both included NR files are not
established. NVIDIA's forum response of 2026-11-09 says the DLSS 5 SDK is not
publicly available and has no public evaluation/preview license. The public SDK's
conditional redistribution grant is not permission for arbitrary NR binaries.
See [the exact-file audit](docs/NR-LICENSING-STATUS.txt). This local experiment is
not cleared for public NR runtime distribution. Preview.11 bundles the GPL NR adapter and import wizard, but no external NR model DLL. The adapter no longer links NVIDIA NGX static code or requires SDK headers; FrameLab owns its parameter storage implementation. The external runtime contract is unchanged. This removes bundled SDK implementation code; it does not establish rights to the separately imported runtime.

- Local NR experiment: Feature-18 parameter contract and initialization sequence adapted from Veyra-NRVideo, commit `33dfdf71197e3056e250465150eb9541d0bb24ce`, GPL-3.0. The D3D11/D3D12 bridge, asynchronous wrapper, coarse motion estimator and FrameLab controls are local implementation. See `docs/experimental-nr.md`. The separate NR model DLLs come unchanged from Veyra v2.1.9 NVIDIA portable, archive SHA-256 `4225ecb11a8e96c2826b4b0b798feb811d7ae237f3edf0b627f3b6895c79f362`; the SF compatibility build is a community runtime, not an official NVIDIA-supported integration. Original licenses and per-file hashes accompany the local package. Earlier local builds used public NGX headers/static core from NVIDIA/DLSS revision `9981c6350dc58c0ce21f024fd65e31e851c31689`. Preview.11 uses independent ABI parameter storage (`NrParameters.hpp`) and no NVIDIA static library. Functional ABI reference is that revision's public `include/nvsdk_ngx_params.h`; no SDK implementation is copied into the new adapter. The locally compiled `nr-host/nvngx.dll` is FrameLab code, not a replacement system driver. No Veyra executable is launched.

- Space Grotesk and IBM Plex Mono, SIL Open Font License 1.1. The same unmodified TTF files used by the website are embedded into the application. Original notices: `website/assets/fonts/SpaceGrotesk-OFL.txt` and `website/assets/fonts/IBMPlexMono-OFL.txt`; both are included under `docs` in portable packages.

- LZ4 library v1.10.0, https://github.com/lz4/lz4/releases/tag/v1.10.0, BSD-2-Clause. Vendored `lib/lz4.c`, `lib/lz4.h` and `lib/LICENSE` under `third_party/lz4`. Used for lossless recording RAM compression, not frame generation. The matching notice is distributed as `docs/LZ4-LICENSE.txt` in the portable app.
- CaptureKVM, https://github.com/PaulFreund/CaptureKVM, commit `dc6dec0b566bcb139c4ea5881f75dfeeae9fad35`, MIT. `DirectShowCapture` and `DeviceEnumeration` copied and adapted. The copied license is `third_party/CaptureKVM-LICENSE.txt`. Local fixes include UTF-8 conversion buffer sizing and immediate stop.
- Dear ImGui, https://github.com/ocornut/imgui, v1.91.9b / `f5befd2d29e66809cd1110a152e375a7f1981f06`, MIT. License in `third_party/imgui/LICENSE.txt`.
- NVIDIA VFX SDK samples, https://github.com/NVIDIA-Maxine/VFX-SDK-Samples, commit `52011f89c1741d06b40ea312af1f20be8be9ec62`, MIT. API integration reference; original license in the upstream repository LICENSE.
- Veyra, https://github.com/Likely7/Veyra-NRVideo, inspected commit `bf596e11112ab350e66d91dfb9c7ea742c254fb4`, GPL-3.0. VfgBackend used as an integration reference for public C API signatures and opaque image descriptors. FrameLab source is distributed under GPL-3.0 to preserve compatibility; see LICENSE. No Veyra executable is invoked and no compatibility patches are applied.
- NVIDIA VFG runtime 1.3.0: complete preview.8 packages use the **official NVIDIA-hosted** `nvidia_vfx-0.2.0.0-cp311-cp311-win_amd64.whl`, SHA-256 `5aaf6a42bc6b6dbbf52fcb714194c994a6893cbbf7ada38bc2165a1f83e4a6fc`. All five DLLs match `docs/nvidia-runtime-manifest.json` and have valid NVIDIA Authenticode signatures. Original wheel licenses/notices are included under `licenses/nvidia-vfg` in complete releases. These binaries retain NVIDIA's separate terms, not GPL; see [distribution and component notes](docs/runtime-distribution.md). Earlier development used identical DLLs extracted from Veyra 2.1.9; the publisher wheel is now verified directly.
- CUDA headers: NVIDIA PyPI package `nvidia-cuda-runtime-cu12` 12.9.79, Windows x64. Original wheel is stored as `downloads/cuda-runtime.zip`; license metadata remains under `deps`.

New application code: FrameLab contributors, 2026. GPL-3.0. Pinned upstream sources are identified above. NVIDIA binaries are included only as separate components of complete packages/runtime add-ons, with original notices; they are not stored in Git history or included in the application source license.

Complete preview.8 packages also include CPython 3.11.9 embedded (PSF), Pillow 13.0.0 and imageio-ffmpeg 0.6.0 with its separate FFmpeg 7.1 executable. Their original licenses and notices are preserved. Source links, publisher hashes and build information are documented in [runtime-distribution.md](docs/runtime-distribution.md). Website fonts Space Grotesk and IBM Plex Mono use the SIL Open Font License; notices accompany the self-hosted font files.

The D3D11 Optical Flow backend is new FrameLab GPL-3.0 code (GPU block matching and warping); it does not call NVIDIA Optical Flow SDK.


Local subtitle translator (2026-11-19): Windows.Media.Ocr is provided by Windows, not redistributed. English-to-Czech weights: Helsinki-NLP / OPUS-MT, original https://object.pouta.csc.fi/OPUS-MT-models/en-cs/opus-2019-12-18.zip; CC BY 4.0 as stated by LICENSE in that exact archive (not the Apache label on a different Hugging Face package). Converted to CTranslate2 4.9.0 int8; no retraining. Attribution, original license, source and hashes accompany app/translation/model and provenance.json. CTranslate2 MIT, SentencePiece 0.2.3 Apache-2.0, NumPy 2.3.5 BSD and bundled-library notices, PyYAML 6.0.3 MIT, CPython 3.11.9 PSF. Original package notices accompany each runtime. Application OCR/IPC/overlay code is FrameLab GPL-3.0. No speech recognition or network translation API is used.

Optional CUNI subtitle downloads (preview.13): CUNI-MH is the CUNI WMT team's
English-to-Czech fine-tuning of Mistral 7B. The archived original model card states
MIT licensing. Q3_K_M, Q5_K_M and Q8_0 GGUF files are published by Richard Erkhov in
`RichardErkhov/wmt24-cuni_-_CUNI-MH-gguf`, pinned to revision
`ebc7f2c321087d423cbed4079b80e63d89e4ac8c`. The app downloads the original publisher
files and the official Ollama Windows portable 0.40.16 runtime (MIT project;
included third-party component notices remain applicable). These weights and
runtime are **not included** in the FrameLab release ZIP. Exact URLs, sizes and
SHA-256 values are in [the catalog](docs/subtitle-models.json). The
[desktop study](https://opicebot.cz/framelab/translation/) retains source cards and
provenance. GPU inference is local and does not use a hosted translation service.
RIFE DirectML backend (2026-11-22): RIFE 4.22 lite inference definitions
and pretrained checkpoint from HolyWu/vs-rife, MIT, with source notices in
third_party/rife422. Export graph optimizations adapted from SmoovPlay, MIT;
full attribution and both licenses accompany the application. The export uses
FP16 CNN operations and FP32 warps, without retraining. Checkpoint digest and
export hashes are in runtime/rife/model-provenance.json. ONNX Runtime DirectML
1.24.4 is MIT; its native distribution includes third-party notices. DirectML
1.15.4 uses the accompanying Microsoft DirectML license, not the ONNX Runtime
MIT license. Full runtime notices are in runtime/rife/licenses. The source
setup script downloads pinned official NuGet archives and checks SHA-256.
These RIFE/runtime terms do not authorize redistribution of separate NR DLLs.
# Multilingual extension (2026-11-23)

TranslateGemma weights are governed by the **Gemma Terms of Use**, not this
project's software license. See [full terms](docs/licenses/Gemma-LICENSE.txt)
and [required notice](docs/licenses/Gemma-Notice.txt). Upstream Google models
and community GGUF quantizers are pinned in `docs/subtitle-models.json`.
The supplied GGUF files are not modified by FrameLab. The multilingual PP-OCRv5
ONNX models from RapidAI / PaddleOCR remain Apache-2.0; hashes and sources are
in `docs/ocr-components.json`. Arabic decoding and horizontal reading-order
handling are adapted in FrameLab; PaddleOCR's Apache-2.0 notice is retained.
DirectWrite, Direct2D and system font fallback are Windows components.

# Component provenance

- LZ4 library v1.10.0, https://github.com/lz4/lz4/releases/tag/v1.10.0, BSD-2-Clause. Vendored `lib/lz4.c`, `lib/lz4.h` and `lib/LICENSE` under `third_party/lz4`. Used for lossless recording RAM compression, not frame generation. The matching notice is distributed as `docs/LZ4-LICENSE.txt` in the portable app.
- CaptureKVM, https://github.com/PaulFreund/CaptureKVM, commit `dc6dec0b566bcb139c4ea5881f75dfeeae9fad35`, MIT. `DirectShowCapture` and `DeviceEnumeration` copied and adapted. The copied license is `third_party/CaptureKVM-LICENSE.txt`. Local fixes include UTF-8 conversion buffer sizing and immediate stop.
- Dear ImGui, https://github.com/ocornut/imgui, v1.91.9b / `f5befd2d29e66809cd1110a152e375a7f1981f06`, MIT. License in `third_party/imgui/LICENSE.txt`.
- NVIDIA VFX SDK samples, https://github.com/NVIDIA-Maxine/VFX-SDK-Samples, commit `52011f89c1741d06b40ea312af1f20be8be9ec62`, MIT. API integration reference; original license in the upstream repository LICENSE.
- Veyra, https://github.com/Likely7/Veyra-NRVideo, inspected commit `bf596e11112ab350e66d91dfb9c7ea742c254fb4`, GPL-3.0. VfgBackend used as an integration reference for public C API signatures and opaque image descriptors. FrameLab source is distributed under GPL-3.0 to preserve compatibility; see LICENSE. No Veyra executable is invoked and no compatibility patches are applied.
- NVIDIA VFG runtime 1.3.0, sourced from the public Veyra 2.1.9 NVIDIA portable archive. Archive SHA-256 `4225ecb11a8e96c2826b4b0b798feb811d7ae237f3edf0b627f3b6895c79f362` matched its release checksum. The five extracted DLLs individually match the bundled runtime manifest and all have valid NVIDIA Authenticode signatures. Exact hashes and upstream origin wheel are in `app/runtime/manifest.json`. NVIDIA licenses and third-party notices are preserved under `third_party/nvidia-vfg`. These binaries remain under NVIDIA's terms, not GPL. This local use does not establish public redistribution rights.
- CUDA headers: NVIDIA PyPI package `nvidia-cuda-runtime-cu12` 12.9.79, Windows x64. Original wheel is stored as `downloads/cuda-runtime.zip`; license metadata remains under `deps`.

New application code: FrameLab contributors, 2026. GPL-3.0. Pinned upstream sources are identified above. Proprietary runtime files are excluded from public packages.

The D3D11 Optical Flow backend is new FrameLab GPL-3.0 code (GPU block matching and warping); it does not call NVIDIA Optical Flow SDK.

# FrameLab NR enhancer: complete setup

The preview.11 full ZIP includes FrameLab, frame generation, both game demos, the offline Czech translation model, export tools and the NR adapter. No compiler, Python or SDK installation is required. NR model/runtime files are a separate external download.

**Community experiment, not an official NVIDIA installer.** As checked on 19 November 2026, NVIDIA has not made the NR SDK publicly available. The external archive below is supplied by Veyra. ShortFuse is a modified NVIDIA runtime. Usage and redistribution rights for these exact files have not been established here; linking, importing and matching checksums do not grant those rights. Use the import only with the applicable authorization. [NVIDIA statement](https://forums.developer.nvidia.com/t/official-dlss-5-neural-rendering-plugin-access-for-unreal-engine-5-8-2/382731/11) · [Veyra component provenance and terms](https://github.com/Likely7/Veyra-NRVideo/blob/33dfdf71197e3056e250465150eb9541d0bb24ce/THIRD_PARTY_NOTICES.md).

## Download → extract → import

1. **[Download FrameLab preview.11 full Windows x64 ZIP](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.11/FrameLab-0.2.0-preview.11-windows-x64-full.zip)**. Extract all into a new writable folder. Allow about 4 GB for both downloads, extraction and import. Run **1 - Spustit FrameLab.cmd** to try the included demos. [General installation and Windows English OCR](https://opicebot.cz/framelab/guide/en.html).
2. **[External download: Veyra-2.1.9-NVIDIA-win64-portable.zip — 731 MB](https://github.com/Likely7/Veyra-NRVideo/releases/download/v2.1.9/Veyra-2.1.9-NVIDIA-win64-portable.zip)**. [Author's release page](https://github.com/Likely7/Veyra-NRVideo/releases/tag/v2.1.9). Select this exact NVIDIA portable archive, not AMD, Source code or the Virtual Camera variant. Keep it zipped; Veyra does not need to run.
3. Close FrameLab. Run **4 - Doplnit enhancer.cmd**. Button 1 opens the external download; button 2 selects the downloaded ZIP and imports it. The same wizard is accessible from **AI / NR → Set up / check enhancer**. The current wizard labels are Czech; the numbered steps are the same.
4. Wait for **Soubory jsou pripravene** (files ready). The importer checks the archive hash and both DLL hashes, copies only the required runtimes and notices, and records provenance. It changes no driver, Windows folder or service, and requires no administrator privileges.
5. Relaunch FrameLab → **AI / NR → Enable enhancer (F6)**. Start at **720p**, **65% strength**, **Split comparison**. F7 changes view, F8 freezes the paired frame, F6 disables the effect. Runtime initialization determines whether the selected GPU/profile actually works.

## Requirements and troubleshooting

- The default **ShortFuse NR 310.9.16** profile was tested on RTX 4050 Laptop. This does not validate RTX 20/30/50 cards. The **NVIDIA NR 310.8 original** profile targets RTX 50 and was not tested on that hardware in this release.
- No AI / NR tab: preview.10 was built without the adapter. Upgrade to the complete preview.11 first; copying DLLs into preview.10 cannot enable it.
- Wrong checksum: use the exact archive above; finish or repeat the download. Unknown versions are rejected.
- Access denied or file in use: close FrameLab and retry in a writable folder.
- Administrator blocks the wizard: respect that policy. With administrator authorization, use manual import below instead of changing system policy.
- Model initialization fails: check NVIDIA drivers, profile and GPU memory; try a 360p/540p limit. The error is shown in AI / NR and `app/nr-cache/nr-host.log`.
- External download removed: an existing verified archive may be imported with applicable authorization. Without a compatible runtime, NR is unavailable; the other FrameLab features still work. The [official NVIDIA DLSS download](https://developer.nvidia.com/rtx/dlss) currently supplies other DLSS technologies, not a substitute NR file.

Pipeline: input → selected frame generation → NR → Czech subtitle overlay → display. NR Original means the frame entering NR, potentially already generated. Enhancement can add latency, drop frames and alter HUD/details. [Real before/after examples](https://opicebot.cz/framelab/#enhancer).

Manual import: copy the external archive's `runtime/experimental/nr-ampere/nvngx_dlssnr.dll` to `FrameLab/app/runtime/nr-sf/nvngx_dlssnr.dll` and `runtime/experimental/nr-original/nvngx_dlssnr.dll` to `FrameLab/app/runtime/nr-original/nvngx_dlssnr.dll`; preserve its NVIDIA notices under `FrameLab/app/licenses/nr-external`. Do not replace FrameLab's own `app/nr-host/nvngx.dll`.

External archive size: 731012476 bytes. SHA-256: `4225ecb11a8e96c2826b4b0b798feb811d7ae237f3edf0b627f3b6895c79f362`. [Exact DLL identities and licensing audit](NR-LICENSING-STATUS.txt). External hosting is not a licensing workaround.

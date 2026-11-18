# Release preparation

1. Run build, CTest, core and both backend verification commands in README. Exercise device enumeration, 4K, language switching, audio and fullscreen on actual hardware; report untested configurations explicitly.
2. Run `tools/package.ps1` after building and staging the reviewed public files. The default minimal ZIP excludes NVIDIA runtime; `-Complete` includes the publisher-verified optional NVIDIA component and portable clip exporter after `tools/prepare_portable_dependencies.py`. Both exclude personal pixel dumps/preferences and accompany the binary with a source ZIP from the exact Git index. The executable uses the static MSVC runtime.
3. Source ZIP must accompany the binary under GPL-3.0. Preserve all notices and provide the exact source used to build. Dependencies are reconstructed by the pinned setup script. Do not include local logs, absolute user paths, downloaded reference projects or credentials.
4. The public repository is [tomik520i/FrameLab](https://github.com/tomik520i/FrameLab). Push reviewed source, then upload portable/source ZIPs and compact examples to a release. No automatic publishing is performed by the packaging script.
5. `tools/prepare_release_example.py TAKE_DIRECTORY` copies only compressed comparison/output previews and their metadata into `release-assets/ID`, with a new manifest, sizes and SHA-256. The original lossless files stay local. Review game footage and any displayed account information before public release; source GPL does not license third-party game assets.

GitHub requires each release asset to be below 2 GiB: https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases . The current full raw MKV exceeds that limit; use the compact previews. Do not put multi-GB raw captures in ordinary Git history.

New benchmarks should label backend, build revision, source resolution/rate, quality, multiplier, VSync, hardware, recording overhead, source/generated IDs, and whether there is held-out ground truth. Compare identical input segments and include failure examples, not just best-looking frames.

## Release: 0.2.0-preview.8

Run `python tools/prepare_portable_dependencies.py`, then `tools/package.ps1 -Version 0.2.0-preview.8 -ApplicationPath app/FrameLab-next.exe -OutputDirectory dist/preview8-final -Complete`. The full bundle contains only the five pinned NVIDIA DLLs with valid publisher signatures, their original licenses, CPython embedded, Pillow and FFmpeg. Test the extracted bundle with no system Python on PATH, real NVIDIA and Optical Flow checks, and a synthetic recording export. Preserve matching source/provenance. The runtime add-on merges `app/runtime` into an existing FrameLab installation and keeps licenses; it is not a generic SDK mirror.

Build the website with `tools/build_website.py` and the reviewed download catalog. Verify every file against its manifest on the server, then expose the `/framelab` directory. Check country routing, manual language precedence, desktop/mobile layouts, videos and byte-range downloads. Keep deployment credentials and raw recordings outside Git. The server requires PHP for the local country lookup; geolocation failure defaults to English.

Use [preview.8 release notes](release-notes-0.2.0-preview.8.md), upload full/source/add-on ZIPs and hashes, verify GitHub asset digests and the Windows CI result, and publish the prerelease. Set the repository homepage to https://opicebot.cz/framelab/ after verifying the live page. Older comparison release URLs remain valid.

## Earlier release: 0.2.0-preview.7

Use [preview.7 release notes](release-notes-0.2.0-preview.7.md). Attach the portable/source ZIPs, both `FrameLab-112947656-comparison-example.zip` and `FrameLab-113768562-comparison-example.zip`, their standalone source-aligned 3× MP4s, `FrameLab-vfg-tuning-2026-11-18.zip`, `4x-quality-3seconds-4x-slow.mp4`, package provenance JSON and `SHA256SUMS-preview.7.txt`. The two new 3× captures are **1080p / VSync off** and **1440p combat / VSync on**, not a controlled comparison. Previous 2×/4× examples are different **1440p** runs. Preserve their historical release URLs. The README 3× GIF goes below the existing 2× GIF and keeps its own metadata/frame map.

Use `tools/package.ps1 -Version 0.2.0-preview.7 -OutputDirectory dist/preview7-final` to preserve an earlier local candidate. The provenance JSON records the staged source tree and package/application hashes. Confirm the eventual release commit has that tree.

Build/test before staging the exact source. Create the release as a draft, upload assets, verify server-reported SHA-256 digests/sizes against local files, check the Windows CI build, then publish the prerelease. Do not include proprietary runtime DLLs, raw recordings or personal session logs.

## Earlier release: 0.2.0-preview.6

Attach these assets from `dist`:

- `FrameLab-0.2.0-preview.6-windows-x64.zip` — ready-made application, launchers and guides.
- `FrameLab-0.2.0-preview.6-source.zip` — corresponding source from the reviewed Git index.
- `FrameLab-82183953-comparison-example.zip` — four checked movies, full-resolution images/crops, settings and manifest; open `index.html` after extraction.
- `FrameLab-84744531-comparison-example.zip` — four checked 4× movies, all three interpolation phases, visible artifacts and separate live GPU measurements.

Also attach each example's standalone `source-aligned_3s_4x-slow.mp4` from `release-assets` for direct playback/download. Use [release notes](release-notes-0.2.0-preview.6.md). The example ZIPs are approximately 176 MB and 213 MB and belong in Releases, not ordinary Git history. Verify hashes against `dist/SHA256SUMS-preview.6.txt`. The public screenshot retains the user's Czech UI while the app's first-run default remains English. Generated examples use different gameplay paths and must not be presented as a controlled same-input 2×/4× test.

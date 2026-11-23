# FrameLab website

English/Czech site published at **https://opicebot.cz/framelab/**. Presentation is plain HTML/CSS/JavaScript. A small PHP endpoint performs a local IP-country lookup: CZ/SK select Czech, other/unknown locations select English. An explicit saved language choice wins. No visitor IP is sent to a third-party service; no database, analytics or third-party embeds are used. Fonts are self-hosted under OFL. The Opicebot installation supplies Font Awesome 6.4.2 under `/fonts/fontawesome-free-6.4.2-web/`.

Country ranges are attributed to [IPdeny](https://www.ipdeny.com/) and refreshed by `python tools/update_country_data.py`. Both IPv4 and IPv6 are covered. IP country is approximate and follows a VPN's exit address. By default the endpoint uses Apache's `REMOTE_ADDR`. Behind a verified reverse proxy, a server-only `country-data/runtime-config.json` may set `trusted_proxies` to exact peer IPs; forwarded chains are parsed right-to-left and client-supplied country headers are ignored. The file is excluded from Git and denied over HTTP. An optional `local_network_countries` object maps a verified private LAN CIDR to `CZ` or `SK` for hairpin connections where no public client address survives. Never trust arbitrary forwarded headers or map public networks using this override. Missing data or request failure safely defaults to English. `locale.php` sends `Cache-Control: private, no-store` to prevent one visitor's country being cached for another.

The live website can evolve independently of a packaged application release; application source ZIPs preserve their exact build tree. Use current `main` for the latest website source.

The complete stopped 20 November 2026 debug audit is under `runs/2026-11-20-3h/`. It describes a **local preview.16** build, not the public preview.15 download. It includes all 1,747 translation jobs and explicit, subjective single-AI review labels for all 1,584 unique pairs, mapped to all 1,747 jobs (zero unreviewed). The original frozen random sample of 300 is retained separately. It does not claim measured OCR accuracy. Before a full website build, populate ignored `release-assets/debug-run-1795196244267574/` with `raw-debug-1795196244267574.zip` and `analysis-full1747.zip` from the published report's `data/` directory. Their hashes and sizes are in `website/runs/2026-11-20-3h/data/download-sizes.json`; `build_website.py` checks them. These exact debug files were explicitly authorized for publication by the owner; this does not authorize publishing other debug sessions or preferences. Analysis scripts and methodology are supplied beside the data. Never substitute the historical 20-minute graph or original comparison media with this later run without updating their labels.

Build with `python tools/build_website.py --downloads PATH_TO_REVIEWED_ASSET_CATALOG`. Output defaults to ignored `dist/framelab-web-preview17`. The catalog lists explicit paths and SHA-256 hashes; raw recordings, credentials and personal preferences are never included. Recorded movies are copied without re-encoding; WebP posters and screenshot previews are resized display derivatives, not new interpolated evidence. PNG/GIF originals remain downloadable.

The historical Spider-Man section retains the original 2×, 3×, combat and 4× clips, two opt-in GIFs and the combat phase strip. Before building, populate ignored `release-assets/spiderman-web/` with the exact files listed in `website/spiderman-media.json`. They are available at `https://opicebot.cz/framelab/media/<file>`; the build verifies their SHA-256 hashes. These recordings used different builds and settings and are labelled accordingly. Do not regenerate them as if they were measurements of the latest release.

Publish the completed directory as `/var/www/html/framelab` on the existing authorized host. Validate against `site-manifest.json` before exposing it, preserve a previous deployment outside the public root, and verify public HTTP responses and video byte ranges. No modification to the parent Opicebot application, database or virtual-host configuration is required. New versions should retain old download filenames until references are updated.

Test keyboard tabs, EN/CZ persistence, video source switching, GIF disclosure panels, image dialogs, downloads, reduced-motion behavior and narrow/desktop layouts. Headless browser screenshots should capture this site only; do not capture the user's desktop.

The full prompt study is `runs/2026-11-20-3h/prompt-test.html`, covering all 3,768 test responses. Also retain `release-assets/debug-run-1795196244267574/prompt-test-evidence.zip`; its hash is in the report data `prompt-evidence-sha256.txt`. The candidate did not generalize and is not the production prompt. Prepared preview.16 guides do not mean the public executable has already been updated.


## Current guide generation (preview.17)

Run `python tools/render_current_guides.py` before packaging. This also renders the separate illustrated walkthrough using `tools/visual_guide_preview17.py`. Historical entry points delegate to this generator and cannot replace the walkthrough with the old text template. Home Guide links resolve to `guide/visual-en.html` or `guide/visual.html`; the detailed text pages remain available. Real capture provenance: `docs/visual-guide-preview16-provenance.json`. Offline HTML uses the same generated pages and assets.


## Preview.17 maintenance and deployment

The current homepage is authoritative HTML in `website/index.html`, with `release.css`, scoped additions in `preview17.css`, and `release.js`. Do not invoke the obsolete release homepage template. Retain the historical Spider-Man/GTA sections, telemetry, enhancer comparisons, subtitle clips and the November 20 full audit. Current measurements are separate at `runs/2026-11-22-rife/` and `runs/2026-11-23-multilang/`; the latter is reproducibly rendered from frozen public JSON/PNG evidence by `tools/render_multilingual_report.py`.

New guide captures are in `docs/images/preview17/`: RIFE and GPU blocking from the local November 22 build, multilingual controls and Japanese/German captions from November 23. Captures show a local test with optional models already downloaded; the guide explicitly explains that public packages require those downloads. Existing installation screenshots retain their historical labels. `render_current_guides.py` builds all 14 text pages plus the two illustrated walkthroughs; `package_guides.py` copies these same pages/assets offline.

Incremental deployment is supported when historical large release assets are already on the server. Review the live file hashes against the baseline, stage only the changed/new public files, verify hashes and all new downloads, back up replaced files outside the web root, and publish `index.html` last. Merge `site-manifest.json` and `downloads/catalog.json`; preserve existing downloads, historical media and server-only `country-data/runtime-config.json`. A new application release must be mirrored and verified before changing its download links. Never replace the whole web root with the source-only `website/` folder: large historical media are intentionally absent from Git.

## November 23 API comparison and remote-window guide

Hosting observation: the public Apache response currently injects the host's existing `/provoz/tracker.js?v=2` script into HTML. This is outside the FrameLab source template. The earlier no-analytics statement describes the original source implementation, not a verified absence of host-wide analytics today. Deployment verification records that exact insertion separately; all static data files are checked byte-for-byte.

`tools/render_remote_guide.py` produces the Czech/English Remote Play, subtitle-only and BYOK API guides, including four real captures of the local test window. Console connectivity was not physically tested. These guides explicitly describe a local experiment, not a replacement for the public preview.17 download.

`runs/2026-11-23-api500/` contains the 500-entry comparison, every translation and rating, latency, token-cost ledger, GPL dataset attribution and source-only audit archive. Rebuild with `render_api500_report.py` and `finalize_api500_audit.py` using the frozen local evidence; never publish a partial preview. The AI judge is not independent human validation. Large complete JSON is downloadable; the interactive viewer fetches smaller chunks. Keep all historical media and download catalog entries unchanged when adding this report.


## Preview.18 publication

Current package links point to preview.18. The historical public preview.17 hero capture remains explicitly labelled; it is not represented as a preview.18 capture. Remote Play/API/window functions are now included in the release. The paired compute guides and new walkthrough steps cover CUDA/Vulkan/CPU. Existing image comparison media and benchmark identities remain unchanged.

`runs/2026-11-23-direct-review/` publishes all 2,000 individually assigned Codex review scores, blinded mapping, notes, formula, source hashes and CSV. No paid API requests were made for this second review. It is single-AI subjective assessment, not human validation. The older paid-judge page and frozen source audit remain historical evidence; do not regenerate their frozen code from current sources. `render_direct_review.py` renders from the saved ratings and original 500-entry outputs without inference.

`runs/2026-11-23-compute/` retains the separate 32-translation backend compatibility/timing study and native OCR integration evidence. Do not combine its times with the 500-entry quality ranking or claim it measures simultaneous FG/NR.

# FrameLab preview.11 — NR setup without compiling or copying DLLs manually

**[Download complete FrameLab](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.11/FrameLab-0.2.0-preview.11-windows-x64-full.zip)** · **[Česky: kompletní instalace enhanceru](https://opicebot.cz/framelab/guide/enhancer.html)** · **[English: complete NR setup](https://opicebot.cz/framelab/guide/enhancer-en.html)**

## Česky: tři kroky

1. Rozbal celý **FrameLab preview.11 full ZIP** do nové složky. Obsahuje aplikaci, VFG, obě herní ukázky, lokální titulky EN→CZ s modelem, exportér a nyní také adaptér NR a průvodce.
2. Pro experimentální NR použij **4 - Doplnit enhancer.cmd**. První tlačítko otevře přesný externí download **Veyra 2.1.9 NVIDIA portable (731 MB)**. Stažený ZIP nerozbaluj; druhým tlačítkem ho vyber. Průvodce ověří SHA-256 archivu i obou DLL, doplní je do správných složek a zachová oznámení. Před importem zavři FrameLab.
3. Spusť FrameLab → **AI / NR → Zapnout enhancer**. Začni 720p, silou 65 % a děleným porovnáním. F6 zapnutí, F7 režim porovnání, F8 zmrazení stejného snímku.

Nemusíš instalovat Python, SDK ani kompilátor. Klikací návody **ZACNI TADY.html** a **NAVOD ENHANCER.html** jsou také přímo v balíku. Preview.10 neměla podporu NR v binárce; pouhé přidání DLL do staré verze nestačí.

## English

Extract the full ZIP, run **4 - Doplnit enhancer.cmd**, then use the two numbered buttons to open the exact external download and import the downloaded ZIP. Close FrameLab before importing. Restart → **AI / NR → Enable enhancer**. The adapter is already built and the importer verifies the archive and individual components. Existing bundled samples, frame generation, offline Czech translation and export tools are retained. Clickable offline guides are included.

## External runtime and rights

The public ZIP includes FrameLab's GPL adapter and installer, **not external NR model DLLs**. The adapter no longer links NVIDIA's proprietary NGX static library. The external archive is provided by Veyra, not an official NVIDIA NR distribution; ShortFuse is a community-modified NVIDIA runtime. Usage and redistribution rights for those exact files are not established here. Linking, importing and hash verification do not grant a license. Use with applicable authorization. [Exact source and terms](https://opicebot.cz/framelab/guide/enhancer.html) · [NVIDIA's public-access statement](https://forums.developer.nvidia.com/t/official-dlss-5-neural-rendering-plugin-access-for-unreal-engine-5-8-2/382731/11).

## Validation

Eight CTest checks pass; A/B drawing checks cover off, zero/full/half strength, split and missing runtime. The SDK-free host completed 12 changing GPU submissions using the imported ShortFuse runtime on RTX 4050 Laptop, with exact input pairing and nonzero neural output. This is compatibility evidence, not a quality benchmark or proof of rights. Installer import, repeat import, hash rejection, paths with spaces and native wizard/file-dialog display were checked. RTX 20/30/50 and physical capture-card behavior were not retested. Enhancement can add latency and artifacts.

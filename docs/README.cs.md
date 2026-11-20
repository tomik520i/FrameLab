# FrameLab — česky

Obraz ze střihovky nebo videa → NVIDIA GPU → monitor. Generování snímků, volitelný enhancer a lokální české titulky z anglického obrazu.

**[Stáhnout preview.15](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.15/FrameLab-0.2.0-preview.15-windows-x64-full.zip) · [Web a ukázky](https://opicebot.cz/framelab/?lang=cs) · [English](../README.md)**

[Připravené změny preview.16 (EN)](release-preview16.md) · [Úplné hodnocení 1 747 překladů](https://opicebot.cz/framelab/runs/2026-11-20-3h/?lang=cs)

![Aktuální aplikace](images/preview15/app.png)

![Originál a 2× FG, čtyřikrát zpomaleně](images/preview15/gta-2x-slow.gif)

## Co umí

- FG 2× / 3× / 4× přes NVIDIA VFG nebo samostatný Optical Flow. Začněte na 2×; vyšší násobiče mohou dělat výrazné artefakty.
- Volitelný NR enhancer: porovnání stejného snímku před/po, síla, limit rozlišení a 1–4 průchody.
- České titulky: neuronové OCR, CPU překlad a volitelné CUNI Q3/Q5/Q8. Označte slovo, oblast čtení a uložte pojmenovaný profil.
- Vlastní MP4/MKV, výběr zvuku a titulkové stopy, samostatné FPS a rozlišení střihovky.
- Nahrávání vstupu, FG, před/po enhancu a výsledku s titulky; klipy, SRT, ID a nastavení JSON.
- Debug až do vypnutí: FPS, CPU/GPU, fronty, OCR, překlad a změny nastavení. Fullscreen, audio výstupy a EN/CZ.

## Spuštění

Rozbalte celý ZIP a spusťte **Start FrameLab.cmd** nebo **1 - Spustit FrameLab.cmd**. Nová instalace pustí GTA ukázku, další spuštění obnoví relaci. V Capture vyberte kartu nebo vlastní video, začněte s FG 2×. Celá obrazovka: F11.

**[Kompletní český návod](../INSTALL.cs.md) · [Obrázkový návod](https://opicebot.cz/framelab/guide/)**

V balíčku jsou VFG runtime, OCR, CPU překladač, kodeky a export; není nutný Python ani SDK. GPU model se stáhne přímo v aplikaci. Externí NR DLL nejsou přibalené ani zrcadlené, protože oprávnění k jejich distribuci nebylo doloženo. [Průvodce importem a odkazy](enhancer-install.cs.md).

## Výsledky bez přikrášlení

[Nová GTA sada](../examples/2026-11-20/README.md) obsahuje parametry, ID snímků, nativní výřezy, videa, SRT a očištěný debug. U 2× jsme zkontrolovali **437 párů A / generovaný / B**; žádný generovaný snímek nebyl kopií kraje. To dokazuje rozdílné pixely, nikoli bezchybný pohyb.

Záznam má **29,33 změněných vstupů/s**, **29,20 generovaných/s** a **52,07 různých snímků/s odeslaných aplikací**. Není to důkaz stabilních 60 FPS na panelu. Čítače neměří fyzický monitor ani celou odezvu od ovladače.

![Detail před/po enhancu](images/preview15/gta-enhancer-crop.png)

Dva enhancer záznamy používají 1440p, jeden průchod, 100 %, odhad pohybu zapnutý. Druhý má jen **12,28 s** úplných dat. Tři titulkové ukázky mají **6,61 / 3,70 / 3,46 s**; chyby překladů zůstaly původní. Nová verze opravuje příliš krátký timeout čtení GPU. Délku dál omezuje RAM a volný disk.

Sestava: RTX 5070 Ti 16 GB, Ryzen 9 5900X, 64 GB RAM, Windows 11 Pro, AVerMedia GC573, monitor 1440p/165 Hz, ovladač 617.14. Ve smíšené dvacetiminutové relaci: medián OCR **333 ms**, příjem obrazu → český titulek **958 ms**, NR worker **5,83 ms**. Časy se překrývají; relace zahrnuje nastavování i export.

Při stejném opakovaném vstupu s FG 2× + NR vyšlo **53,81 FPS v popředí, 52,93 za maximalizovaným oknem a 54,82 fullscreen**. Výrazný propad se nezopakoval. Titulky byly při izolovaném testu vypnuté. Výchozí systémové časování se nemění.

## Podpora

**[Podpořit přes Opicebot](https://opicebot.cz/?public=prdelniopice&donation2=prdelniopice)** — pouze česká stránka, PayPal v CZK.

Zdrojový kód GPL-3.0; knihovny a herní záběry mají vlastní podmínky. Komunitní projekt bez spojení s NVIDIA a vydavateli her.

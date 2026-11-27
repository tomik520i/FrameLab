# FrameLab 0.2.0-preview.20

[English](#english) · [Česky](#česky) · [Download](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.20)

## English

This release publishes the local subtitle fixes used after preview.19. It does not replace the translation models or promise better language quality from the example prompt.

- **OpenAI recovery:** a saved reviewed model and Windows-stored key can start without refreshing the model list. Use **Subtitles → Start OpenAI translation** or **Restart OpenAI translation**. An explicit restart clears the current translation session/history.
- **Bounded waiting:** OpenAI and custom servers expose **Maximum wait per subtitle (seconds)** and **Tokens per response**; fresh defaults are **2 seconds / 1000 tokens**. A shared deadline covers all speaker blocks. Timed-out work is discarded and the latest queued caption is tried; normal timeouts retain the worker and dialogue history. The configured limit bounds waiting, not guaranteed translation speed or display latency. Token limits are output limits, not the model's total context window.
- **Recoverable failures:** temporary connection/server errors and unusable individual answers skip the caption. No immediate automatic retry of the same failed caption. Authentication, configuration and HTTP 429 rate/quota failures stop translation until corrected and restarted. A separate process guard handles a stuck worker. Memory selection keeps its previous evidence when an update fails.
- **Custom prompt editor:** **Subtitles → Custom AI server / LAN → Edit translation prompt…**. Enable the editor, write instructions and select plain text or a single `<translation>…</translation>` block. Use `{source_language}` and `{target_language}`; OCR text and enabled dialogue history are attached automatically. **Save and apply** reconnects an active translator and clears its session history; Cancel leaves the running settings alone. The editor affects the custom-server provider only. Its example is a starting point, not a validated improvement. The memory selector still has its own prompt.
- **Two-speaker captions:** keep separate speaker blocks while joining soft-wrapped lines within a block. A nearby smaller neutral-grey reply can pass the style filter when geometry matches an accepted caption. OCR can still miss faint text or misidentify a speaker; this is not a universal detector.

### Verification

Release checks: 76 Python tests, 18 CTest tests, and the native prompt-editor migration/layout test passed. In a localhost fault-injection run through the actual C++ → Python queue, the next caption request began 2.016 seconds after the intentionally slow request; obsolete queued captions were skipped and history survived. This is a controlled recovery test, not an internet latency or translation-quality benchmark. [Reproduction](../tests/SUBTITLE_RECOVERY.md).

### Install or update

Download the full Windows x64 ZIP, extract **all** files to a new writable folder, and run **Start FrameLab.cmd**. English is the fresh-install UI default; Czech remains available. CPU translation, OCR, media codecs and existing demos are included. GPU translation models and the optional external NR runtime retain their separate setup; no new credentials or personal model files are bundled.

To preserve settings, close FrameLab first, keep the previous folder as a backup, and copy your `app/*-settings.txt`, `app/runtime-session.txt`, `app/subtitle-profiles.txt`, `app/language.txt` and `app/recording-folder.txt` into the new `app` folder if present. Copy only configuration from a trusted installation; do not replace the new executable or Python workers with old files. Locally downloaded models/runtime can be copied separately. Keys remain in Windows Credential Manager for the same Windows account. A genuinely new installation remains offline until the user enables an API provider.

[Custom server and prompt guide](https://opicebot.cz/framelab/guide/custom-ai-en.html#prompt) · [OpenAI start/recovery guide](https://opicebot.cz/framelab/guide/remote-subtitles-en.html#openai) · [Validation record](https://opicebot.cz/framelab/downloads/FrameLab-preview20-validation.json)

## Česky

Vydání zveřejňuje lokální opravy titulků po preview.19. Nemění překladové modely a neslibuje vyšší kvalitu překladu díky ukázkovému promptu.

- **Obnova OpenAI:** uložený podporovaný model a klíč stačí ke spuštění bez opětovného načítání seznamu modelů. V Titulkách použij **Spustit OpenAI překlad / Restartovat OpenAI překlad**. Ruční restart vymaže aktuální relaci a historii překladače.
- **Čekání a tokeny:** u OpenAI i vlastního serveru nastavíš **Nejdelší čekání na titulek (sekundy)** a **Tokenů na odpověď**. Čistá instalace začíná na **2 sekundách / 1000 tokenech**. Jeden časový limit platí pro všechny repliky titulku. Po timeoutu se přeskočí zastaralá fronta a pokračuje nejnovějším titulkem; běžný timeout nemaže historii ani neshazuje worker. Limit není zárukou rychlosti překladu ani odezvy zobrazení. Výstupní tokeny nejsou velikost celého kontextu modelu.
- **Chyby spojení:** dočasná síťová chyba, chyba serveru nebo nepoužitelná odpověď přeskočí titulek. Tentýž chybný titulek se okamžitě automaticky neopakuje. Chyby klíče, nastavení a HTTP 429 (limit/kvóta) vyžadují opravu a nové spuštění. Při selhání aktualizace paměti zůstanou poslední platné podklady.
- **Vlastní prompt:** **Titulky → Custom AI server / LAN → Upravit překladový prompt…**. Zapni vlastní instrukce a vyber prostý text nebo jeden blok `<translation>…</translation>`. Značky `{source_language}` a `{target_language}` se doplní; aktuální OCR a zapnutá historie se připojí samy. **Uložit a použít** připojí aktivní překladač znovu a vymaže jeho historii; Zrušit zachová původní stav. Editor platí jen pro vlastní server, ne pro vestavěné OpenAI nebo lokální modely. Ukázka není ověřeným zlepšením kvality. Výběr paměti má vlastní prompt.
- **Dva mluvčí:** jednotlivé repliky zůstávají oddělené, zalomení jednoho mluvčího se spojuje. Blízká menší šedá odpověď může projít filtrem stylu podle umístění vůči přijatému titulku. Slabý text může OCR stále minout.

### Instalace a aktualizace

Rozbal celý nový ZIP do nové zapisovatelné složky a spusť **Start FrameLab.cmd**. Výchozí jazyk čisté instalace je angličtina; češtinu lze přepnout. CPU překladač, OCR, kodeky a dosavadní ukázky jsou součástí. GPU modely a volitelný externí NR runtime se nastavují samostatně jako dříve.

Při zachování nastavení nejdřív aplikaci zavři a původní složku ponech jako zálohu. Přenes existující `app/*-settings.txt`, `app/runtime-session.txt`, `app/subtitle-profiles.txt`, `app/language.txt` a `app/recording-folder.txt` do nového `app`. Nepřepisuj nový program ani Python skripty starými soubory. Stažené modely/runtime lze přenést zvlášť. Klíče zůstávají ve Správci pověření stejného účtu Windows. Čistá instalace používá lokální režim, dokud uživatel API sám nezapne.

[Vlastní server a prompt](https://opicebot.cz/framelab/guide/custom-ai.html#prompt) · [Spuštění a obnova OpenAI](https://opicebot.cz/framelab/guide/remote-subtitles.html#openai) · [Záznam ověření](https://opicebot.cz/framelab/downloads/FrameLab-preview20-validation.json)

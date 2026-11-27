# FrameLab preview.20 — česká instalace

[English](START_HERE.md) · [Web a ukázky](https://opicebot.cz/framelab/?lang=cs)


[Obrazový návod: aktuální screenshoty, očíslované kroky a zvětšování](https://opicebot.cz/framelab/guide/visual.html). Stejný návod je v ZIPu jako **OBRAZOVY NAVOD.html**.

## Nové v preview.20

Preview.20: obnova překladu, oddělené repliky mluvčích a volitelný editor vlastního promptu. OpenAI lze znovu spustit s uloženým modelem a klíčem; pomalý titulek nezablokuje frontu. [Změny a přenos nastavení](docs/release-preview20.md).

## Nové v preview.19: vlastní AI server a volitelná paměť rozhovoru

- **Titulky → Způsob překladu → Custom AI server / LAN:** připoj OpenAI-compatible nebo Ollama server, načti/vyber model a nastav jeho podporované parametry. Volitelný klíč ukládá Správce pověření Windows zvlášť pro danou adresu. [Návod k serveru](https://opicebot.cz/framelab/guide/custom-ai.html); offline: **VLASTNI AI SERVER.html**.
- **Použít poslední repliky rozhovoru** přidá nedávné původní titulky. **Uchovat důležité repliky** nechá vybraný model průběžně vybírat původní věty k delšímu uchování. Obě možnosti jsou dobrovolné a výchozí stav je vypnuto. CUNI druhou možnost nepodporuje; překladovým specialistům může více vyhovovat paměť vypnutá. [Nastavení a omezení](https://opicebot.cz/framelab/guide/memory.html); offline: **PAMET ROZHOVORU.html**.
- V Titulkách lze prohlédnout použitý kontext a vymazat jej při změně hry nebo tématu. Testovaný výběr pomáhá přes odbočky, ale po změně tématu může držet starý význam. Není to záruka lepšího překladu. Paměť tvoří tentýž vybraný model; API tím může zvýšit cenu a místní model nároky na GPU/RAM.
- [25 českých + 50 vícejazyčných situací](https://opicebot.cz/framelab/runs/2026-11-24-context75/?lang=cs): 300 skutečných běhů aplikace s OCR, VFG a NR, 900 původních snímků, prodlevy i chyby. Měřena byla Luna a oddělená serverová Gemma; nejde o měření překladových vah běžících vedle FG na notebooku.

## Nové v preview.18: okno hry, samostatné titulky a Vulkan

- **Vstup → Okno aplikace:** obnov seznam, vyber běžící hru nebo Remote Play a spusť zachytávání. Hra musí zůstat otevřená; začni režimem okna nebo bezrámečkového okna. Chráněný obraz a některé hry zachytit nejdou. Obrazovou GPU vybereš samostatně; při změně se aplikace restartuje.
- **Zobrazení → Jen titulky přes zdrojové okno:** průhledný překryv sleduje vybrané okno, kliknutí propouští do hry. OCR oblast nastav předem a polohu českého textu posuň nezávisle. Tento režim nepřidává FG ani NR do samotné hry. [Obrazový postup Remote Play, překryvu a API](docs/remote-subtitles-api.cs.md).
- **Titulky → Výpočet překladu:** Automaticky, CUDA, Vulkan nebo CPU; vyber i konkrétní GPU. CUNI a Gemma používají stejné modely. Pro nedostatek VRAM lze povolit GPU + RAM. Na tomto notebooku je nejlepší CUDA; Intel UHD / Vulkan funguje, ale je pomalý. AMD nebylo fyzicky otestováno. [Návod a časy](docs/local-translation-compute.cs.md).
- **Volitelné OpenAI API:** zvol online způsob překladu, vlož vlastní klíč a načti modely. Klíč ukládá Windows Credential Manager; do API odchází text OCR. Výchozí nabídka míří na levné rychlé modely, podporované parametry řídí profily. Bez klíče fungují místní překladače. Placené hodnocení kvality se v aplikaci nespouští.
- Přibalený místní filtr omezuje jména, SPZ a nápisy; nejednoznačnou větu neumí vždy odlišit od titulku. Zúžení OCR oblasti zůstává důležité. Aktivní přehrávání/zachytávání brání uspání displeje; nejde o trvalou změnu nastavení Windows.

[Vlastní hodnocení všech 2 000 překladů, pořadí modelů a konkrétní chyby](https://opicebot.cz/framelab/runs/2026-11-23-direct-review/). Sada 500 replik měřila textový překlad bez FG, enhanceru a OCR; není to měření herních FPS.

## Nové v preview.17: jiná GPU a další jazyky

V **Frame gen** zvol **RIFE 4.22 lite / DirectML** nebo **D3D11 Optical Flow**. Tyto cesty nejsou omezené na RTX 40/50. RIFE vyžaduje vhodnou hardwarovou DirectX 12 GPU a ovladač. Na tomto Intel UHD je při 720p RIFE příliš pomalé; Optical Flow dosáhl přibližně 59 potvrzených prezentací/s. AMD a starší GeForce zatím nejsou fyzicky otestované. NVIDIA VFG a NR mají vlastní užší omezení. [Tabulka GPU](docs/GPU-BACKENDS.md).

V **Titulky** vyber jazyk obrazu a cíl. Všech 12 OCR sad je v ZIPu. **OPUS-MT / CPU** funguje hned pro EN → CS. Pro jiný pár zvol **TranslateGemma 4B Q4_K_M**, klikni **Stáhnout / pokračovat a ověřit**, počkej na dokončení a zapni **Překládat titulky**. Model má 2,49 GB, první GPU runtime dalších 1,62 GB; po stažení se nic nepřekládá v cloudu. Q8 a 12B jsou volitelné větší varianty. Nejde o přepis zvuku. [Podrobný návod](docs/MULTILINGUAL.md).

Celý veřejný ZIP přibaluje RIFE modely a runtime, VFG runtime, 12 OCR sad, CPU překladač, dva herní klipy a syntetickou titulkovou ukázku. Velké GPU překladové modely a runtime stáhne tlačítko; externí NR DLL nejsou v ZIPu ani na zrcadle. Staré screenshoty nezměněných kroků jsou označené, nové jazyky ukazuje doplněný obrazový návod.

## Stáhnout a spustit

1. [Stáhněte celý ZIP pro Windows x64](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.20/FrameLab-0.2.0-preview.20-windows-x64-full.zip) a **rozbalte vše**, například do D:/FrameLab. Nespouštějte aplikaci uvnitř ZIPu.
2. Spusťte **Start FrameLab.cmd** nebo **1 - Spustit FrameLab.cmd**. Nová instalace pustí GTA ukázku, další spuštění obnoví relaci. Python, SDK ani systémový VLC nepotřebujete; složky balíčku musí zůstat pohromadě.
3. Začněte s **Frame gen → 2×**. **F11** zobrazí čistý výstup, Escape/F11 vrátí ovládání. Tlačítkem **EN / CZ** přepnete anglické rozhraní do češtiny.

VFG cílí na RTX 40/50. Spouštěč **3 - Optical Flow.cmd** zvolí samostatnou alternativu; podporu a výkon je nutné ověřit na konkrétní kartě. VFG runtime, neuronové OCR, CPU překladač, kodeky a export jsou přibalené. GPU modely jsou volitelné stažení; enhancer vyžaduje externí import. Antivirus může první spuštění zdržet; ochranu nevypínejte, zkontrolujte případné chyby.

## Střihovka

V **Capture / Vstup** zvolte zdroj, **Rozlišení** a **FPS vstupu** zvlášť. Použijí se tlačítkem **Spustit vstup**. Automatika nechá volbu ovladači. 4K i jiné poměry stran závisejí na nabízených režimech karty.

**Přijaté snímky** ukazují skutečné rozměry a rychlost dodávání obrazu. API nehlásí vstupní časování HDMI. Šedesát snímků střihovky může opakovat obrazy 30FPS hry. Odstranění opakovaných snímků tomu pomáhá; odhad obsahu není čítač herního enginu.

Černý obraz: zkontrolujte správnou kartu, kabel, povolený zdroj a stav aplikace. Zavřete jiné programy využívající kartu. **Test 30 FPS** je umělý test, nikoli PS5.

## Vlastní video, MKV a stopy

**Capture → Open video… / Otevřít video…** otevře soubor. K dispozici je pauza, posun, rychlost a opakování. Přibalený **LibVLC 3.0.24** přehrává MP4/MKV. Po načtení stop vyberte zvuk a titulky, případně vložené titulky vypněte. [Podrobnosti](docs/local-video.md).

Titulková stopa se vykreslí do vstupu před FG/NR. OCR překládá viditelný obraz, nikoli přímo textovou stopu; řeč nepřepisuje. Chráněné streamovací soubory nejsou podporované. Dekódování a kopírování mohou zatížit CPU.

## FG a enhancer

Začněte na **2× / Medium**. Kontrolujte hrany, HUD a rychlý pohyb; 3×/4× mohou dělat silné artefakty. Násobič nezaručuje stejný nárůst skutečně zobrazených snímků.

V **AI / NR** je volitelný enhancer. [Přesný postup importu](docs/enhancer-install.cs.md). Začněte na **720p a jednom průchodu**. Při nízkých FPS snižte limit rozlišení/průchody nebo vypněte odhad pohybu. Síla efektu pouze míchá výsledek, nesnižuje práci neuronové sítě. F6 zapíná NR, F7 mění porovnání, F8 zmrazí pár. Originál u NR může už obsahovat FG. Pořadí: vstup → FG → NR → český titulek → displej.

## České titulky a uložené profily

V **Subtitles / Titulky** zvolte **RapidOCR**, **OPUS-MT / CPU** a zapněte překlad. RapidOCR nepotřebuje Windows OCR jazyk. Umělá ukázka EN → CZ v Capture pomůže ověřit nastavení.

Když je vidět anglický titulek, klikněte na **Vybrat slovo titulku + oblast čtení…**. Náhled se zmrazí. Obtáhněte **jedno slovo**, potom celou oblast pro titulky včetně více řádků a potvrďte. Učí se barva a přibližná výška písmen, nikoli konkrétní slovo. Pokud lze, držte SPZ a HUD mimo oblast čtení.

Profil pojmenujte a uložte. Vzhled, oblast i výběr se pamatují; pro další hru lze načíst jiný profil. Filtr vzhledu omezuje nesouvisející text, není to stoprocentní detekce titulků.

Pro GPU překlad vyberte **CUNI Q3/Q5/Q8** a **Stáhnout / pokračovat a ověřit**. Aplikace stáhne model i přenosný runtime, ověří hashe a ukáže průběh. Q5 je v publikovaných GTA ukázkách; vyšší přesnost nemusí vždy znamenat lepší překlad. [Velikosti a časování](docs/subtitle-model-guide.cs.md).

České řádky se centrují, jejich pozici lze přesouvat nezávisle od OCR. Text zůstává uvnitř obrazu. Minimální doba zobrazení a ponechání při čekání dávají čas číst opožděné titulky, zatímco OCR pokračuje. Rychlé dialogy a chyby překladů zůstávají omezením.

## Displej a zvuk

**Display / Zobrazení** vybírá monitor a Fit/Fill/1:1. Fit zachová obraz s pruhy; Fill i 1:1 větší než okno mohou ořezávat. F11 skryje ovládání, porovnání a OCR editaci; český titulek zůstane ve výstupu.

**Audio / Zvuk** nabízí výstup, hlasitost a ztlumení. Video a střihovka mají vlastní zvukové ovládání. Analytické klipy zvuk neobsahují. Odeslané/DXGI FPS neměří fyzický panel ani celou odezvu od ovladače.

## Nahrávání před/po a titulků

V **Recording / Nahrávání** vyberte složku a **Nahrát 15 s pro analýzu**. Cesta se uloží. Pro 1440p se všemi efekty doporučujeme alespoň **36 GiB volného místa**; spotřeba záleží na materiálu.

Snímky se bezeztrátově komprimují do omezené RAM, poté ukládají a exportují. Počkejte na dokončení. Nedostatek paměti/disku nebo pomalé čtení GPU může ukončit záznam dřív; částečný záznam je označený a vyexportuje se úplný úsek.

Výstupy: RAW/generované přehledy, synchronizované před/po/finál, trojité porovnání, **subtitles.cs.srt**, texty, časování a nastavení s ID. Finál obsahuje skutečný titulek včetně zpoždění. Plocha ani zvuk se nenahrávají. [Formát a limity](docs/pipeline-recording.md).

## Debug a měření

Záložku najdete šipkami v řadě nastavení. Přečtěte upozornění a zapněte log; **F9** jej přepíná. Ukládá časová razítka, CPU/GPU, FPS, fronty, texty OCR/překladů a nastavení **až do vypnutí**. Před sdílením zkontrolujte soukromé texty.

Nově se ukládá zaměření/fullscreen, velikost okna, čekání na prezentaci a prodlevy smyčky. [Význam měření](docs/debug-measurement.md).

## Aktualizace

Aplikaci zavřete a zálohujte starou složku/nastavení. Nový ZIP rozbalte zvlášť. Přeneste osobní nastavení/profily a případně ověřené modely/runtime; staré skripty ani program nekopírujte přes nové. Veřejný balíček neobsahuje soukromé logy, záznamy ani nastavení autora.

## Preview.16: obnova titulků a malé statistiky

V **Debug a měření** zapni malé fullscreen statistiky, případně stiskni **F10**. Vpravo nahoře uvidíš softwarově potvrzené/odeslané FPS a průměr/P95 odezvy uvnitř aplikace. Chybí zpoždění střihovky a monitoru; nejde o úplnou odezvu od ovladače. Statistika se nevypaluje do analytického záznamu.

Běžné spuštění obnoví uloženou relaci včetně stavu logování. Pokud jej při příštím startu nechceš zapnout, před zavřením ho zastav. Při pádu překladače aplikace zkusí nejvýše tři obnovy po 2, 5 a 15 sekundách a znovu odešle i nezměněný OCR text. Vypnutí titulků opakování zruší. GPU model zůstává načtený, dokud jsou titulky zapnuté; vypnutím se uvolní.

[Úplný tříhodinový audit](https://opicebot.cz/framelab/runs/2026-11-20-3h/?lang=cs) má posouzených všech 1 747 překladů a obsahuje původní logy i chyby. Experimentální testy promptů samy nemění prompt aplikace.


## Volitelný úsporný enhancer Eco

V **AI / NR** zapni **Úsporně: enhancer před frame genem (experiment)**. NR pak počítá zdrojové obrazy před interpolací, takže se zapnutým FG může provádět méně výpočtů. Zachová rozlišení a sílu efektu, ale může přidat latenci, VRAM a jiné artefakty. Výchozí stav je vypnutý. Nižší AI limit a méně průchodů šetří výkon v obou režimech; nižší síla sama výpočet nešetří. [Naměřené výsledky a omezení](docs/NR_ECO.md).

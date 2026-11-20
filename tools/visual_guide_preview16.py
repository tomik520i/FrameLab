"""Observed preview.16 controls and bilingual walkthrough copy.

Images are real Windows captures. Coordinates are in the original screenshot;
the renderer crops the view and draws annotations without changing UI pixels.
"""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
DOWNLOAD = 'https://opicebot.cz/framelab/downloads/FrameLab-0.2.0-preview.16-windows-x64-full.zip'

def content(legacy):
    shots = {k: legacy[k] for k in ('extract', 'destination', 'launch', 'wizard', 'select', 'ready')}
    for key, shot in list(shots.items()):
        filename, size, crop, marks, caption = shot
        shots[key] = (filename, size, crop, marks, (
            'Starší skutečný snímek nezměněného instalačního kroku. ' + caption[0],
            'Historical real screenshot of an unchanged installation step. ' + caption[1]))
    def shot(key, filename, crop, marks, cs, en):
        size = (2560, 1440 if key == 'fullscreen' else 1392)
        shots[key] = ('preview16/' + filename + '.png', size, crop, marks, (cs, en))
    def mark(cs, en, cx, cy, x, y, w, h):
        return ((cs, en), cx, cy, x, y, w, h)
    shot('overview','01-overview',None,[
        mark('Obraz a porovnání','Preview and comparisons',130,760,285,150,1510,835),
        mark('Nastavení a měření','Settings and telemetry',1980,320,2085,24,465,1335)],
        'Preview.16 ovládaná ve Windows. V náhledu běží přiložené video; čítače nejsou benchmark.',
        'Preview.16 operated in Windows with the bundled clip playing. Counters are not a benchmark.')
    shot('capture','03-capture',(2010,265,540,790),[
        mark('Zdroj','Source',2040,359,2100,344,438,33),
        mark('Rozlišení a FPS zvlášť','Separate resolution and FPS',2040,475,2100,437,438,119),
        mark('Spustit vstup','Start capture',2040,617,2100,601,112,32)],
        'AVerMedia GC573 je vybraná v seznamu. Zde se ale přehrává 720p video; 1440p je volba pro příští spuštění karty.',
        'AVerMedia GC573 is selected, but the playing source here is a 720p video. 1440p is the next capture request.')
    shot('local','10-local-video',(2010,660,540,620),[
        mark('Vybrat soubor','Choose a file',2040,700,2100,683,108,33),
        mark('Přehrávání a smyčka','Playback and loop',2040,899,2100,877,315,79)],
        'Vlastní MP4 přes LibVLC. Tato krátká ukázka nemá zvukovou ani titulkovou stopu; nabídky stop se zobrazují jen u souborů, které je obsahují.',
        'Local MP4 through LibVLC. This short clip has no audio or subtitle tracks; track selectors appear only when the file contains them.')
    shot('fg','11-frame-gen',(2010,265,540,485),[
        mark('Backend','Backend',2040,360,2100,344,438,34),
        mark('Zapnout a zvolit kvalitu','Enable and choose quality',2040,399,2100,382,288,33),
        mark('Násobič','Multiplier',2040,466,2100,447,438,40)],
        'NVIDIA VFG 2×, v tomto snímku kvalita Low. Pro začátek vyzkoušej 2× / Medium.',
        'NVIDIA VFG 2×, with Low selected in this capture. Try 2× / Medium as a starting point.')
    shot('nr','12-enhancer-eco',(2010,310,540,1065),[
        mark('Zapnutí a Eco','Enable and Eco',2040,460,2100,427,438,70),
        mark('Počet průchodů','Number of passes',2040,639,2100,621,438,34),
        mark('Limit zpracování','Processing cap',2040,1110,2100,1093,438,34)],
        'Eco zapnuté pro pořízení návodu. Vstup je 1280 × 720; limit 1440p obraz nezvětšuje. Původní nastavení bylo po focení obnoveno.',
        'Eco enabled for this walkthrough. The input is 1280 × 720; the 1440p cap does not upscale it. Original settings were restored afterwards.')
    shot('models','07-subtitles-running',(2010,555,540,520),[
        mark('OCR je přibalené','OCR is bundled',2040,592,2100,573,438,34),
        mark('Vybrat překladač','Select a translator',2040,697,2100,680,438,34),
        mark('Stav a zapnutí','Status and enable',2040,953,2100,922,438,84)],
        'Q5 už je zde stažené. U chybějícího modelu se objeví Download / resume and verify; žádné ruční příkazy nejsou potřeba.',
        'Q5 is already downloaded here. A missing model exposes Download / resume and verify; no manual commands are needed.')
    shot('word-button','07-subtitles-running',(2010,270,540,285),[
        mark('Vybrat slovo a oblast','Select word and area',2040,404,2100,388,263,33),
        mark('Znovu otevřít inspektor','Reopen the inspector',2040,483,2100,468,226,33)],
        'Tlačítka jsou hned nahoře v Titulcích. Výběr používá zmrazený zdrojový snímek.',
        'Buttons are at the top of Subtitles. Selection uses a frozen source frame.')
    shot('word','04-ocr-word',(675,255,1210,905),[
        mark('Jedno čitelné slovo','One legible word',1440,719,1063,704,73,30),
        mark('Oblast pro všechny řádky','Area for all subtitle lines',1450,755,836,709,410,44),
        mark('Změnit oblast čtení','Adjust reading area',1450,373,811,354,151,34)],
        'Na umělém testovacím videu jsem označil „locked.“. Modrá je vzorek slova, žlutá čtecí oblast, zelená přijatý text.',
        'I selected “locked.” in the artificial test video. Blue marks the word, yellow the reading area, green accepted text.')
    shot('inspector','05-ocr-inspector',(675,255,1210,905),[
        mark('Barva a výška písmen','Colour and letter height',1480,607,698,587,600,80),
        mark('Odmítnutý text mimo oblast','Rejected text outside the area',1480,840,698,824,600,106),
        mark('Přijatý dialog','Accepted dialogue',1480,1081,698,1044,600,99)],
        'Naměřeno přibližně 30 px, RGB 253/254/255. Časomíra mimo oblast byla odmítnuta; věta byla přijata.',
        'Measured about 30 px and RGB 253/254/255. The timer outside the region was rejected; the sentence was accepted.')
    shot('profiles','06-save-profile',(2010,1070,540,255),[
        mark('Výběr uložených profilů','Saved profile selector',2040,1116,2100,1099,438,33),
        mark('Název a uložení','Name and save',2040,1210,2150,1152,305,130)],
        'Ukázkový profil se jmenuje Demo - white subtitles. Uložení vlastní kopie nepřepíše předchozí profil.',
        'The example profile is called Demo - white subtitles. Save as creates a copy without overwriting the previous profile.')
    shot('timing','08-subtitles-timing',(2010,690,540,350),[
        mark('Spojování vět','Sentence joining',2040,730,2100,713,310,33),
        mark('Minimum a čekání','Minimum duration and waiting',2040,850,2100,810,438,121)],
        'Čas na čtení začíná až zobrazením českého titulku. Následující text se může překládat souběžně.',
        'Reading time begins when the Czech caption appears. The following text can translate concurrently.')
    shot('placement','09-subtitles-placement',(2010,915,540,472),[
        mark('Velikost textu','Text size',2040,945,2100,930,438,33),
        mark('Umístění a zarovnání','Position and anchor',2040,1075,2100,991,438,290),
        mark('Přetáhnout překryv myší','Drag the overlay',2040,1305,2100,1287,438,33)],
        'Český překryv má vlastní polohu. Oblast OCR se nastavuje zvlášť; není to pozice výsledného titulku.',
        'Czech overlay placement is independent of the OCR reading area.')
    shot('display','13-display',(2010,267,540,455),[
        mark('Měřítko a monitor','Scaling and monitor',2040,362,2100,344,312,74),
        mark('Celá obrazovka','Fullscreen',2040,517,2100,502,122,33)],
        'Fit zachová celý obraz. Monitor je v tomto průchodu nastavený ve Windows na 2560 × 1440 / 165 Hz.',
        'Fit preserves the whole image. Windows reports 2560 × 1440 / 165 Hz for the selected monitor in this walkthrough.')
    shot('audio','14-audio',(2010,267,540,290),[
        mark('Výstupní zařízení','Output device',2040,360,2100,344,438,33),
        mark('Hlasitost a ztlumení','Volume and mute',2040,450,2100,432,438,73)],
        'Ovládání zvuku souboru. Zvolená ukázka nemá audio; její ticho není chyba vybraného výstupu.',
        'File audio controls. The selected sample is silent; this is not a failure of the output device.')
    shot('recording','15-recording',(2010,267,540,325),[
        mark('Nahrát 15 sekund','Record 15 seconds',2040,335,2100,319,155,33),
        mark('Cíl a volné místo','Destination and free space',2040,473,2100,444,438,99)],
        'Nahrává se výstup FrameLabu a jeho mezistupně, ne pracovní plocha. Tímto focením se nový velký záznam nespouštěl.',
        'Records FrameLab output and intermediate stages, not the desktop. Taking these screenshots did not start another large recording.')
    shot('stats','16-stats',(2010,267,540,755),[
        mark('Potvrzená ID od Windows','Windows-confirmed IDs',2040,428,2100,393,438,46),
        mark('Latence aplikace','Application latency',2040,681,2100,647,438,143),
        mark('Celá GPU','Whole GPU',2040,843,2100,829,438,25)],
        'Graf obsahuje přepínání a opakované načítání krátkého klipu. Jde o průvodce čítači, ne měření stabilního výkonu.',
        'The graph includes switching and repeated loading of the short clip. This explains counters; it is not a steady-state benchmark.')
    shot('debug','17-debug',(2010,267,540,955),[
        mark('Malé statistiky ve fullscreenu','Small fullscreen statistics',2040,360,2100,345,310,33),
        mark('Zahrnout OCR a překlady','Include OCR and translations',2040,554,2100,537,438,33),
        mark('Start / Stop / F9','Start / Stop / F9',2040,651,2100,635,198,33)],
        'Logování je zde vypnuté. Until stopped zapisuje až do zastavení; zapnuté logování se při běžném příštím startu obnoví.',
        'Logging is off here. Until stopped keeps writing until stopped; an active logging session resumes on a normal next launch.')
    shot('fullscreen','18-fullscreen-hud',None,[
        mark('Volitelný přehled F10','Optional F10 statistics',2200,140,2371,3,182,46)],
        'Skutečný F11 výstup bez panelů a OCR rámečku. APP je odezva uvnitř aplikace, nikoli ovladač → fyzický monitor.',
        'Actual F11 output without panels or the OCR outline. APP measures application delay, not controller-to-physical-display latency.')

    steps=[]
    def step(key, cs, en, bodycs, bodyen, expectedcs, expecteden, keys):
        steps.append((key,(cs,en),(bodycs,bodyen),(expectedcs,expecteden),keys))
    step('download','Stáhni a rozbal celý ZIP','Download and extract the full ZIP',
        f'<p><a class="guide-cta" href="{DOWNLOAD}">Stáhnout preview.16 ↓</a></p><p>Ve Windows 10/11 x64 klikni na stažený ZIP pravým tlačítkem → <strong>Extrahovat vše</strong>. Vyber krátkou zapisovatelnou cestu, třeba <code>D:/FrameLab</code>, a počkej na dokončení. Nespouštěj program uvnitř ZIPu.</p><p>Potřebuješ ovladač NVIDIA GPU; pro živý obraz také ovladač střihovky. Python, kompilátor ani SDK neinstaluješ. Při chybě 0x80010135 zvol kratší cestu.</p>',
        f'<p><a class="guide-cta" href="{DOWNLOAD}">Download preview.16 ↓</a></p><p>On Windows 10/11 x64, right-click the ZIP → <strong>Extract all</strong>. Choose a short writable path such as <code>D:/FrameLab</code> and wait for completion. Do not run inside the ZIP.</p><p>Install your NVIDIA GPU driver, and your capture-card driver for live input. No Python, compiler or SDK setup is needed. For error 0x80010135, use a shorter path.</p>',
        'Celá složka s app, deps, docs, tools, licenses a spouštěči.','A complete folder with app, deps, docs, tools, licenses and launchers.',['extract','destination'])
    step('launch','Spusť aplikaci a ukázku','Launch the app and a demo',
        '<p>Spusť <code>Start FrameLab.cmd</code> nebo <code>1 - Spustit FrameLab.cmd</code>. Čerstvá instalace pustí GTA ukázku; další starty obnoví relaci. Přepínač <strong>EN / CZ</strong> je vpravo nahoře.</p><p>V kartě <strong>Capture / Vstup</strong> můžeš kdykoliv pustit GTA 720p / 30 FPS, CS:GO 1080p nebo test EN → CZ. K těmto ukázkám nepotřebuješ střihovku. Na snímcích tohoto návodu běží skutečný klip, případně označené umělé OCR video.</p>',
        '<p>Run <code>Start FrameLab.cmd</code> or <code>1 - Spustit FrameLab.cmd</code>. A clean install starts the GTA demo; subsequent launches restore the session. <strong>EN / CZ</strong> is at the top right.</p><p>Under <strong>Capture</strong>, choose GTA 720p / 30 FPS, CS:GO 1080p or the EN → Czech test at any time. No capture card is needed. These screenshots show a playing clip or the explicitly labelled artificial OCR fixture.</p>',
        'Obraz vlevo, nastavení vpravo. Další karty odkryjí malé šipky vedle názvů karet.','Preview on the left, settings on the right. Small arrows beside the tabs reveal more tabs.',['overview'])
    step('capture','Připoj střihovku','Connect a capture card',
        '<p>Zdroj HDMI → vstup střihovky → USB/PCIe do PC. Zavři jiné aplikace používající kartu. V <strong>Capture</strong> vyber zařízení, rozlišení a FPS zvlášť a stiskni <strong>Start capture</strong>. 4K i jiné poměry stran závisí na režimech ovladače.</p><p><strong>Delivered frames</strong> ukazuje rozměry skutečně přijímaných snímků. HDMI timing toto API nehlásí. 60 FPS přenos může obsahovat opakované snímky 30 FPS hry. <strong>Test 30 FPS</strong> je umělý zdroj. Při No Signal ověř kabel, zdroj a obsazení karty.</p>',
        '<p>HDMI source → capture-card input → USB/PCIe to the PC. Close other apps holding the card. In <strong>Capture</strong>, select device, resolution and FPS separately, then <strong>Start capture</strong>. 4K and other aspect ratios depend on driver modes.</p><p><strong>Delivered frames</strong> reports received frame dimensions. The API does not report HDMI timing. A 60 FPS capture can repeat a 30 FPS game image. <strong>Test 30 FPS</strong> is synthetic. For No Signal, check the cable, source and device ownership.</p>',
        'Živý vstup nahradí ukázku. HDR není implementované.','Live input replaces the demo. HDR is not implemented.',['capture'])
    step('local-video','Otevři vlastní video nebo MKV','Open your own video or MKV',
        '<p><strong>Capture → Open video…</strong> otevře výběr souboru. Přehrávač nabízí pauzu, posun a smyčku. Kodeky LibVLC jsou v kompletním balíku; systémové VLC není potřeba.</p><p>Obsahuje-li soubor více audií či titulky, po načtení vyber příslušnou stopu. Vypni vložené titulky, pokud je nechceš v obrazu. OCR čte viditelný text; nepřekládá přímo textovou stopu ani řeč. Chráněné streamingové soubory nejsou podporované.</p>',
        '<p><strong>Capture → Open video…</strong> opens a file picker. Use pause, seek and loop. The full package includes LibVLC codecs; no system VLC installation is required.</p><p>If the file contains multiple audio/subtitle tracks, select them after loading. Disable embedded subtitles if unwanted. OCR reads visible text; it does not translate text tracks directly or transcribe speech. Protected streaming files are unsupported.</p>',
        'Název souboru a jeho nativní rozměry. Chybějící nabídka stop může znamenat, že je soubor nemá.','The file name and native dimensions appear. Missing track selectors can mean the file has no such tracks.',['local'])
    step('fg','Začni na frame genu 2×','Start with 2× frame generation',
        '<p><strong>Frame gen → NVIDIA VFG (RTX 40/50) → Frame generation</strong>. Začni <strong>2× / Medium</strong>. Optical Flow je samostatný alternativní backend pro další GPU; jeho kvalita a rychlost jsou jiné.</p><p>Odstraňování opakovaných vstupů pomáhá u 30 FPS hry přenášené v 60 FPS. Sleduj hrany, HUD a prudký pohyb. 3×/4× mohou mít výrazné artefakty. Násobič nezaručuje odpovídající fyzická FPS monitoru.</p>',
        '<p><strong>Frame gen → NVIDIA VFG (RTX 40/50) → Frame generation</strong>. Start at <strong>2× / Medium</strong>. Optical Flow is a separate fallback for other GPUs, with different quality and speed.</p><p>Remove repeated input frames helps with a 30 FPS game delivered at 60 FPS. Inspect edges, HUD and fast motion. 3×/4× can cause severe artifacts. The multiplier does not guarantee physical monitor FPS.</p>',
        'Stav aktivního backendu a nenulový čítač generovaných snímků při pohybu.','An active backend status and generated-frame counter during motion.',['fg'])
    step('wizard','Volitelně doplň enhancer','Optionally install the enhancer',
        '<p>FG a OCR fungují bez tohoto kroku. NR je komunitní experiment s externími soubory. Zavři FrameLab a spusť <code>4 - Doplnit enhancer.cmd</code>. Průvodce odkáže na přesný balík a zkontroluje import.</p><p>Stáhni <strong>Veyra-2.1.9-NVIDIA-win64-portable.zip</strong>, nech jej zabalený a vyber druhým tlačítkem průvodce. Nevybírej AMD, Virtual Camera ani Source code. <a href="enhancer.html">Přesné odkazy, podmínky a opravy chyb →</a></p>',
        '<p>FG and OCR work without this step. NR is a community experiment using external files. Close FrameLab and run <code>4 - Doplnit enhancer.cmd</code>. The wizard links to the exact package and verifies the import.</p><p>Download <strong>Veyra-2.1.9-NVIDIA-win64-portable.zip</strong>, keep it zipped and select it with the second wizard button. Do not choose AMD, Virtual Camera or Source code. <a href="enhancer-en.html">Exact links, terms and troubleshooting →</a></p>',
        'Průvodce potvrdí připravené soubory. Pak aplikaci znovu spusť.','The wizard confirms files are ready. Restart the app afterwards.',['wizard','ready'])
    step('nr','Enhancer, porovnání a úsporné Eco','Enhancer, comparison and optional Eco',
        '<p>V <strong>AI / NR</strong> zapni enhancer. Pro začátek zkus <strong>720p / 1 pass</strong>. F6 efekt vypíná, F7 přepíná porovnání, F8 zmrazí stejný pár. Síla mění výsledné prolnutí; nižší síla sama nešetří výpočet AI.</p><p><strong>Eco: enhance before frame gen</strong> přesune NR před FG. Při 30 → 60 FPS může NR počítat přibližně 30 místo 60 obrazů za sekundu. Může přidat latenci, spotřebuje více VRAM a změní artefakty. Je volitelné a výchozí stav je vypnutý.</p><p>Snížení AI limitu nebo počtu průchodů šetří výkon v obou režimech. Původní rozměry výstupu zůstávají. <a href="https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.16/docs/NR_ECO.md">Naměřené výsledky a omezení Eco →</a></p>',
        '<p>Enable the enhancer in <strong>AI / NR</strong>. Start with <strong>720p / 1 pass</strong>. F6 toggles the effect, F7 switches comparison, F8 freezes a matching pair. Strength changes the final blend; reducing it alone does not reduce AI computation.</p><p><strong>Eco: enhance before frame gen</strong> moves NR before FG. At 30 → 60 FPS, NR can process about 30 rather than 60 images per second. It can add latency, uses extra VRAM and changes artifacts. It is optional and off by default.</p><p>Lower AI caps or fewer passes reduce work in both modes. Output dimensions are preserved. <a href="https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.16/docs/NR_ECO.md">Measured Eco results and limits →</a></p>',
        'Stav AI a počet vyhodnocení za sekundu. Změna obrazu nemusí být zlepšení; porovnej stejnou scénu.','AI status and evaluations per second. A change is not necessarily an improvement; compare the same scene.',['nr'])
    step('models','Zapni OCR a zvol překladač','Enable OCR and choose a translator',
        '<p>V <strong>Subtitles</strong> ponech <strong>RapidOCR / PP-OCRv5 English</strong>. Je přibalené, Windows OCR nestahuješ. Přibalený <strong>OPUS-MT / CPU</strong> umožní první test bez dalšího downloadu.</p><p>Pro GPU vyber <strong>CUNI Q3, Q5 nebo Q8</strong> a klikni na <strong>Download / resume and verify</strong>, pokud model chybí. Aplikace stáhne model i potřebný přenosný runtime a ověří soubory. Potom zapni <strong>Translate subtitles to Czech</strong>. Q5 model má asi 5,13 GB; první runtime další 1,62 GB. Nech místo i pro rozbalení. <a href="subtitles.html">Velikosti všech modelů a postup →</a></p>',
        '<p>Under <strong>Subtitles</strong>, keep <strong>RapidOCR / PP-OCRv5 English</strong>. It is bundled; no Windows OCR download is needed. The included <strong>OPUS-MT / CPU</strong> supports a first test without another download.</p><p>For GPU translation, select <strong>CUNI Q3, Q5 or Q8</strong> and click <strong>Download / resume and verify</strong> if missing. The app downloads and verifies the model and portable runtime. Then enable <strong>Translate subtitles to Czech</strong>. Q5 is about 5.13 GB; the first runtime adds 1.62 GB. Leave extraction space too. <a href="subtitles-en.html">All model sizes and steps →</a></p>',
        'Stav připraveného modelu; po přečtení angličtiny se zobrazí čeština. Vyšší kvantizace nezaručuje lepší větu.','A ready-model status; Czech appears after English is read. Higher precision does not guarantee a better sentence.',['models'])
    step('word','Označ jedno slovo a čtecí oblast','Mark one word and the reading area',
        '<p>Počkej na dobře čitelný anglický titulek. Klikni <strong>Select subtitle word + reading area…</strong>. V náhledu myší obtáhni <strong>jedno celé slovo</strong>.</p><p>Pak zvol <strong>Adjust reading area</strong> a vymez pás, kam se vejdou i delší titulky na více řádcích. Vynech minimapu, SPZ a chat. Slovo určuje přibližnou barvu a výšku písmen, není hledaným slovem ani názvem fontu. Potvrď <strong>Use this style</strong>.</p>',
        '<p>Wait for a legible English caption. Click <strong>Select subtitle word + reading area…</strong>. Drag around <strong>one complete word</strong> in the preview.</p><p>Then choose <strong>Adjust reading area</strong> and mark a band large enough for longer, multiline captions. Exclude minimaps, plates and chat. The word supplies approximate colour and letter height; it is not a required phrase or font name. Confirm <strong>Use this style</strong>.</p>',
        'Modrý vzorek a větší žlutá oblast. Přehrávání na pozadí běží dál.','A blue sample and a larger yellow region. Background playback continues.',['word-button','word'])
    step('inspector','Ověř, co OCR skutečně přijalo','Inspect what OCR actually accepted',
        '<p><strong>OCR inspector / saved sample…</strong> znovu otevře uložený vzorek. Rozbal <strong>Recognized text / measured values / reasons</strong>.</p><p>Vidíš přečtený text, barvu, výšku a důvod odmítnutí. Zeleně jsou přijaté řádky; oranžově například text mimo čtecí oblast. Když mizí správné titulky, vyber čistší slovo nebo uprav tolerance v <strong>Appearance details</strong>. Stejně vypadající nápis uvnitř oblasti stále může projít.</p>',
        '<p><strong>OCR inspector / saved sample…</strong> reopens the saved sample. Expand <strong>Recognized text / measured values / reasons</strong>.</p><p>Inspect text, colour, height and rejection reasons. Accepted lines are green; text outside the region is marked orange. If valid captions disappear, select a cleaner word or adjust tolerances under <strong>Appearance details</strong>. Unrelated text with the same style inside the region can still pass.</p>',
        'Na této umělé ukázce byla přijata věta a odmítnuta časomíra mimo oblast.','In this artificial example, the sentence was accepted and the timer outside the region rejected.',['inspector'])
    step('profiles','Ulož profil pro hru nebo video','Save a profile for a game or video',
        '<p>V <strong>Subtitle appearance</strong> klikni <strong>Save as…</strong>, zadej název a stiskni <strong>Save profile</strong>. Příště jej vybereš ze seznamu. <strong>Save changes</strong> aktualizuje vybraný profil; Save as vytvoří nový.</p><p>Ukládá se styl i čtecí oblast. Výběr přežije restart aplikace. Pro jiné rozlišení nebo výrazně jiný styl titulků profil zkontroluj.</p>',
        '<p>Under <strong>Subtitle appearance</strong>, choose <strong>Save as…</strong>, enter a name and click <strong>Save profile</strong>. Select it from the list next time. <strong>Save changes</strong> updates the selected profile; Save as creates a new one.</p><p>Style and reading region are saved. The selection survives restarts. Check the profile after changing source resolution or subtitle style.</p>',
        'V rozbalovacím seznamu je tvůj název.','Your name appears in the profile selector.',['profiles'])
    step('timing','Dej překladu čas a nastav polohu','Allow reading time and place the overlay',
        '<p><strong>Minimum on screen</strong> počítá čas až od zobrazení češtiny. <strong>Keep while waiting</strong> podrží předchozí titulek během dalšího překladu. <strong>Join split sentences</strong> zkouší spojit pokračující větu; vypni jej, pokud spojuje nesouvisející dialog.</p><p>Níže nastav velikost, polohu a ukotvení českého textu nebo povol <strong>Drag Czech overlay in preview</strong> a přesuň jej myší. Víceřádkový text je vystředěný a omezený viditelným obrazem. Zelený OCR rámeček označuje čtení originálu, ne polohu češtiny.</p>',
        '<p><strong>Minimum on screen</strong> starts when Czech appears. <strong>Keep while waiting</strong> retains the last caption while another translates. <strong>Join split sentences</strong> attempts to join continuations; disable it if unrelated dialogue is joined.</p><p>Below, adjust size, placement and anchors, or enable <strong>Drag Czech overlay in preview</strong>. Multiline text is centered and clamped to the visible image. The green OCR rectangle marks source reading, not Czech placement.</p>',
        'Překlad zůstává čitelný; zpoždění překladu ale není odstraněné.','The translation remains readable; translation delay is not eliminated.',['timing','placement'])
    step('display','Vyber monitor a měřítko','Choose monitor and scaling',
        '<p><strong>Display → Fit</strong> zachová celý obraz, Fill a příliš velké 1:1 mohou oříznout okraje. Vyber monitor. Rozlišení a obnovovací frekvenci monitoru nastavuješ ve Windows, ne zde.</p><p>Výsledný obraz jde na výstup GPU tohoto PC. Passthrough střihovky úpravy FrameLabu neobsahuje. F11 zobrazí čistý hlavní obraz, Escape/F11 vrátí ovládání.</p>',
        '<p><strong>Display → Fit</strong> preserves the whole image. Fill and oversized 1:1 can crop edges. Choose the output monitor. Set its resolution and refresh rate in Windows, not here.</p><p>The processed image goes through this PC’s GPU output. Capture-card passthrough does not contain FrameLab effects. F11 shows clean output; Escape/F11 returns to controls.</p>',
        'Správný monitor a zachovaný poměr stran.','The intended monitor and aspect ratio.',['display'])
    step('audio','Vyber zvukový výstup','Choose audio output',
        '<p>Šipkou vpravo vedle karet najdi <strong>Audio</strong>. Vyber výstup Windows, hlasitost a případně ztlumení. Zvuk videa a capture mají vlastní ovládání.</p><p>Ne každý klip obsahuje zvuk. U filmu s více audii vyber jazykovou stopu v přehrávači. FG a NR přidávají zpoždění obrazu; čítač APP není měření synchronizace A/V.</p>',
        '<p>Use the right tab arrow to reach <strong>Audio</strong>. Select the Windows output, volume and mute. File and capture audio have their own controls.</p><p>Not every clip contains sound. For a movie with multiple audio tracks, choose its language track in the player. FG and NR add video delay; the APP counter does not measure A/V synchronization.</p>',
        'Zvuk jde do zvoleného zařízení, pokud zdroj obsahuje audio.','Sound reaches the chosen device when the source contains audio.',['audio'])
    step('recording','Nahraj 15 sekund pro porovnání','Record 15 seconds for comparison',
        '<p>V <strong>Recording → Choose folder…</strong> vyber disk s místem. Pro 1440p se všemi stupni počítej prakticky alespoň s 36 GiB volného místa. Klikni <strong>Record 15 s analysis</strong> a počkej i na následný export klipů.</p><p>Uloží se RAW, generované snímky, párované před/po enhanceru, finální obraz s českými titulky, SRT, časování a JSON s nastavením a ID. Nahrávají se zapnuté efekty, klipy jsou bez zvuku. Nedokončené záznamy jsou označené. <a href="recording.html">Formát a limity →</a></p>',
        '<p>In <strong>Recording → Choose folder…</strong>, select a drive with space. For 1440p with all stages, allow at least 36 GiB free as a practical recommendation. Click <strong>Record 15 s analysis</strong> and wait for clip export too.</p><p>Exports include RAW, generated frames, matching before/after enhancement, final image with Czech captions, SRT, timing and settings/ID JSON. Only enabled effects are recorded; clips are silent. Incomplete takes are labelled. <a href="recording-en.html">Format and limits →</a></p>',
        'Dokončený export ve Clips folder. Během záznamu neměň pipeline.','Completed exports in Clips folder. Do not change the pipeline during a take.',['recording'])
    step('stats','Čti FPS a odezvu správně','Read FPS and latency correctly',
        '<p><strong>Stats</strong> rozlišuje vstupní snímky, odhad měnícího se obsahu, odeslané obrazy a ID potvrzená Windows. Pomlčky znamenají nedostupné měření, ne nulu. Potvrzená ID mohou být dolní odhad.</p><p>APP / callback → Present měří čas uvnitř aplikace. Nezahrnuje střihovku, fyzický monitor ani ovladač. Vytížení GPU je pro celou kartu, tedy i ostatní programy. Krátký screenshot neprokazuje dlouhodobě stabilní FPS.</p>',
        '<p><strong>Stats</strong> separates incoming frames, estimated changing content, submitted images and Windows-confirmed IDs. Dashes mean unavailable, not zero. Confirmed IDs can be a lower bound.</p><p>APP / callback → Present measures time inside the app. It excludes the capture card, physical display and controller. GPU usage covers the whole card, including other apps. One screenshot does not establish sustained FPS.</p>',
        'Grafy a jasně označené rozsahy měření.','Graphs with explicitly scoped measurements.',['stats'])
    step('debug','Zapni debug a textové logování','Enable debug and text logging',
        '<p>Šipkou vedle karet otevři <strong>Debug &amp; measurement</strong>. Zvol dobu, případně <strong>Frame-by-frame timing</strong> a <strong>Include OCR and translated text</strong>. Klikni <strong>Start debug recording</strong> nebo F9.</p><p><strong>Until stopped</strong> zapisuje do složky až do Stop/F9 nebo ukončení programu. Pokud aplikaci zavřeš se zapnutým logováním, běžný další start jej obnoví. Log obsahuje časová razítka, nastavení, CPU/GPU, FPS, fronty, OCR a překlady podle voleb. Obrázky ani zvuk nenahrává a nic automaticky neposílá. Před sdílením zkontroluj soukromý text.</p>',
        '<p>Use the tab arrow to open <strong>Debug &amp; measurement</strong>. Choose duration, optionally <strong>Frame-by-frame timing</strong> and <strong>Include OCR and translated text</strong>. Click <strong>Start debug recording</strong> or F9.</p><p><strong>Until stopped</strong> writes until Stop/F9 or exit. If you close with logging active, a normal next launch restores it. Logs contain timestamps, settings, CPU/GPU, FPS, queues, OCR and translations according to selected options. No images or audio are recorded and nothing uploads automatically. Check private text before sharing.</p>',
        'Roste čas a velikost logu. Open debug folder otevře soubory; Stop ukončí zápis.','Elapsed time and log size increase. Open debug folder opens files; Stop ends writing.',['debug'])
    step('fullscreen','Malé statistiky při hraní','Small statistics while playing',
        '<p>V Debug zapni <strong>Fullscreen FPS + latency</strong> nebo stiskni F10. Potom F11 otevře čistý výstup. Vpravo nahoře zůstane nenápadné FPS, APP a P95.</p><p>F10 statistiku skryje. Ovládání, porovnání i čtecí rámeček OCR se ve fullscreenu skryjí; přeložené titulky zůstávají. Statistika se nevypaluje do analytických klipů.</p>',
        '<p>In Debug, enable <strong>Fullscreen FPS + latency</strong> or press F10. Then F11 opens clean output. Small FPS, APP and P95 readouts remain in the upper-right corner.</p><p>F10 hides the statistics. Controls, comparisons and the OCR reading outline hide in fullscreen; translated captions remain. Statistics are not burned into analysis clips.</p>',
        'Čistý obraz a volitelný malý přehled; Escape vrátí nastavení.','Clean output with an optional small readout; Escape returns to settings.',['fullscreen'])
    return shots, steps

"""Illustrate real screenshots with separate SVG annotations; never regenerate UI pixels."""
from pathlib import Path
import html

ROOT = Path(__file__).resolve().parents[1]
DOWNLOAD = 'https://opicebot.cz/framelab/downloads/FrameLab-0.2.0-preview.12-windows-x64-full.zip'
VEYRA = 'https://github.com/Likely7/Veyra-NRVideo/releases/download/v2.1.9/Veyra-2.1.9-NVIDIA-win64-portable.zip'

# Each marker is (label cs/en, circle x/y, target rectangle x/y/width/height).
# Coordinates refer to the observed screenshots, not inferred UI positions.
SHOTS = {
 'download': ('01-download.jpg', (639,600), None, [
   (('Kompletní balík','Complete package'),550,468,24,500,460,74)],
   ('Tlačítko na webu při původním průchodu preview.11. Aktuální download je preview.12; postup rozbalení je stejný.', 'Website button from the original preview.11 walkthrough. The current download is preview.12; extraction steps are unchanged.')),
 'extract': ('02-extract-menu.png',(922,674),(165,90,600,550),[
   (('Pravé tlačítko na ZIP','Right-click the ZIP'),235,270,176,168,150,28),
   (('Extrahovat vše','Extract all'),235,358,335,343,313,32)],
   ('Skutečná nabídka Průzkumníka Windows 11. Rozbaluj FrameLab, ne ZIP Veyry.', 'Actual Windows 11 Explorer menu. Extract FrameLab, not the Veyra ZIP.')),
 'destination': ('03-extract-confirm.png',(566,474),None,[
   (('Krátká cílová cesta','Short destination path'),70,280,38,146,400,29),
   (('Potvrdit rozbalení','Confirm extraction'),430,360,387,440,92,25)],
   ('Použij vlastní novou složku na Ploše. Jméno uživatele v cestě bude jiné.', 'Use your own new Desktop folder. Your Windows username will differ.')),
 'launch': ('04-launchers.png',(922,674),(165,90,750,560),[
   (('První spuštění','First launch'),800,352,177,345,260,28),
   (('Průvodce enhancerem','Enhancer setup wizard'),800,446,177,432,260,29)],
   ('Soubory po dokončeném rozbalení. Složky app, deps, tools a licenses musí zůstat pohromadě.', 'Files after complete extraction. Keep app, deps, tools and licenses together.')),
 'first': ('05-first-launch.png',(1171,714),None,[
   (('Ukázka běží','Demo is playing'),180,100,15,166,756,429),
   (('NR zatím chybí','NR not installed yet'),697,340,800,305,354,82)],
   ('CS:GO se spustí bez capture karty. Nedostupný NR v této fázi je očekávaný stav.', 'CS:GO plays without a capture card. Unavailable NR is expected at this stage.')),
 'samples': ('06-input-samples.png',(1171,714),None,[
   (('Obě herní ukázky','Both gameplay demos'),694,568,800,557,205,31),
   (('Test českého překladu','Czech translation test'),694,624,800,610,153,27)],
   ('Ukázky se přehrávají z přibalených souborů. Pole Zdroj nahoře je pro živé zařízení.', 'Demos use bundled files. The Source field at the top is for a live device.')),
 'fg': ('07-frame-gen.png',(1171,714),None,[
   (('NVIDIA VFG pro RTX 40/50','NVIDIA VFG for RTX 40/50'),702,300,800,287,354,28),
   (('Zapnout generování','Enable generation'),702,346,800,318,240,28),
   (('Začni na 2×','Start with 2×'),704,390,800,369,115,34)],
   ('NVIDIA VFG active potvrzuje spuštěný backend. Zobrazená FPS jsou okamžitý stav, ne příslib výkonu.', 'NVIDIA VFG active confirms the backend is running. These FPS counters are a momentary state, not a performance promise.')),
 'wizard': ('08-enhancer-wizard.png',(920,747),None,[
   (('Přesný externí download','Exact external download'),830,349,30,384,859,54),
   (('Vybrat stažený ZIP','Select the downloaded ZIP'),830,698,30,517,859,54)],
   ('Průvodce je přibalený. Nejprve ukonči FrameLab. Podmínky externího balíku zůstávají samostatné.', 'The wizard is bundled. Close FrameLab first. The external package retains its separate terms.')),
 'select': ('09-select-zip.png',(942,591),(194,0,748,591),[
   (('Přesný název archivu','Exact archive name'),640,330,218,220,700,30),
   (('Otevřít spustí import','Open starts the import'),730,450,683,535,110,34)],
   ('Veyra-2.1.9-NVIDIA-win64-portable.zip — nerozbalovat; ne AMD, Source code ani Virtual Camera.', 'Veyra-2.1.9-NVIDIA-win64-portable.zip — keep zipped; not AMD, Source code or Virtual Camera.')),
 'ready': ('10-import-ready.png',(920,747),None,[
   (('Import dokončen','Import complete'),830,694,25,595,869,64)],
   ('Toto potvrzení bylo zachyceno po skutečném importu přes grafický průvodce do čerstvě rozbalené aplikace.', 'This confirmation was captured after a real graphical-wizard import into a freshly extracted application.')),
 'nr': ('28-preview12-comparison-settings.png',(1171,714),None,[
   (('Zapnout enhancer','Enable enhancer'),699,350,800,337,235,31),
   (('Síla efektu','Effect strength'),699,404,800,390,355,29),
   (('Porovnání pouze v okně','Window comparison only'),699,487,800,460,355,59)],
   ('Preview.12 s NVIDIA VFG + NR. Čáru můžeš táhnout v náhledu, F8 zmrazí pár. F11 porovnání automaticky skryje.', 'Preview.12 with NVIDIA VFG + NR. Drag the divider in the preview; F8 freezes a pair. F11 automatically hides comparisons.')),
 'ocr': ('12-subtitles.png',(1171,714),None,[
   (('Překlad zapnout','Enable translation'),704,333,800,320,265,28),
   (('Zobrazit oblast OCR','Show OCR region'),704,407,800,393,266,28),
   (('Vymezit pruh titulků','Select subtitle strip'),704,519,800,475,284,126)],
   ('Skutečný výstup lokálního OCR a modelu: The door is locked. → Dveře jsou zamčené. Barevné pruhy jsou označená umělá testovací ukázka.', 'Real local OCR/model output: The door is locked. → Dveře jsou zamčené. Color bars are the synthetic test fixture.')),
 'display': ('13-display.png',(1171,714),None,[
   (('Měřítko obrazu','Image scaling'),701,299,800,288,354,26),
   (('Výstupní monitor','Output monitor'),701,337,800,319,354,27),
   (('Celá obrazovka','Fullscreen'),701,425,800,414,129,27)],
   ('Zpracovaný obraz je na monitoru PC ve FrameLabu. HDMI passthrough capture karty upravený obraz nepřenáší.', 'Processed output appears in FrameLab on the PC monitor. Capture-card HDMI passthrough does not carry these changes.')),
 'capture': ('06-input-samples.png',(1171,714),None,[
   (('Vyber svou capture kartu','Select your capture card'),698,300,800,287,354,26),
   (('Režimy od ovladače','Driver-reported modes'),698,412,800,375,354,82),
   (('Spustit živý vstup','Start live capture'),698,504,800,491,93,29)],
   ('Toto je umístění ovládání, nikoli test capture karty. Na notebooku je v seznamu kamera ACER; u sebe vyber svou střihovku.', 'This shows control locations, not a capture-card test. This laptop lists an ACER camera; select your own capture card.')),
 'longpath': ('trouble-long-path.png',(449,357),None,[
   (('Příliš dlouhá cesta','Path too long'),370,170,22,99,387,31)],
   ('Skutečná chyba při extrakci do hluboce zanořené složky. Rozbalení do krátké složky na Ploše následně prošlo.', 'Actual extraction failure in a deeply nested folder. Extraction to a short Desktop folder subsequently succeeded.')),
}

STEPS = [
 ('download', ('Stáhni kompletní FrameLab','Download complete FrameLab'),
  (f'<p>Otevři web FrameLabu a klikni na <strong>Stáhnout kompletní FrameLab</strong>. Použij balík <code>windows-x64-full.zip</code>.</p><p><a class="guide-cta" href="{DOWNLOAD}">Stáhnout preview.12 ↓</a></p><p>Windows 10/11 x64, NVIDIA GPU a aktuální ovladač. Pro oba archivy, rozbalení a import si nech přibližně 4 GB místa. NVIDIA VFG potřebuje RTX 40/50.</p>',
   f'<p>Open the FrameLab website and choose <strong>Download complete FrameLab</strong>. Use the <code>windows-x64-full.zip</code> package.</p><p><a class="guide-cta" href="{DOWNLOAD}">Download preview.12 ↓</a></p><p>Windows 10/11 x64, NVIDIA GPU and an up-to-date driver. Allow about 4 GB for both archives, extraction and import. NVIDIA VFG requires RTX 40/50.</p>'),
  ('Ve složce stažených souborů máš celý FrameLab ZIP. GitHub Source code není spustitelná aplikace.', 'The complete FrameLab ZIP is in your downloads folder. GitHub Source code is not a ready-to-run application.'), ['download']),
 ('extract', ('Pravé tlačítko → Extrahovat vše','Right-click → Extract all'),
  ('<p>Ve Windows klikni pravým tlačítkem na stažený <strong>FrameLab ZIP</strong> a vyber <strong>Extrahovat vše…</strong>.</p><p>Program nespouštěj uvnitř archivu. Otevření ZIPu dvojklikem není rozbalení.</p>', '<p>In Windows, right-click the downloaded <strong>FrameLab ZIP</strong> and choose <strong>Extract all…</strong>.</p><p>Do not run the program inside the archive. Double-clicking a ZIP does not extract it.</p>'),
  ('Otevře se okno pro výběr cílové složky.', 'The destination-folder dialog opens.'), ['extract']),
 ('destination', ('Vyber krátkou cestu a počkej','Choose a short path and wait'),
  ('<p>Přes <strong>Procházet…</strong> vyber novou složku na své Ploše, například <code>FrameLab</code>. Potvrď <strong>Extrahovat</strong> a počkej, až rozbalování úplně skončí.</p><p>Nevol dlouhou cestu zanořenou v dalších složkách. Chyba <code>0x80010135</code> znamená příliš dlouhou cestu: zruš rozbalování a zopakuj je do kratšího umístění.</p>', '<p>Use <strong>Browse…</strong> to select a new Desktop folder, for example <code>FrameLab</code>. Confirm <strong>Extract</strong> and wait for extraction to finish completely.</p><p>Avoid a deeply nested path. Error <code>0x80010135</code> means the path is too long: cancel extraction and repeat it in a shorter location.</p>'),
  ('Uvnitř je složka FrameLab s app, deps, docs, tools, licenses a spouštěči. Nic nevynechávej.', 'The extracted FrameLab folder contains app, deps, docs, tools, licenses and launchers. Keep everything.'), ['destination']),
 ('launch', ('Spusť první ukázku','Launch the first demo'),
  ('<p>Otevři rozbalenou složku FrameLab a dvojklikem spusť <code>1 - Spustit FrameLab.cmd</code>.</p><p>Python, pip, SDK ani kompilátor neinstaluješ. Potřebné prostředí a model českého překladu jsou už v balíku. Aplikace není podepsaná; není potřeba vypínat ochrany Windows.</p>', '<p>Open the extracted FrameLab folder and double-click <code>1 - Spustit FrameLab.cmd</code>.</p><p>No Python, pip, SDK or compiler installation is required. The runtime and Czech translation model are bundled. The application is unsigned; disabling Windows protections is not required.</p>'),
  ('Hraje krátké CS:GO ve smyčce. Hlásí-li AI / NR chybějící soubory, pokračuj krokem 7.', 'A short CS:GO clip loops. If AI / NR reports missing files, continue with step 7.'), ['launch','first']),
 ('samples', ('Vyzkoušej obě hry','Try both games'),
  ('<p>V kartě <strong>Vstup</strong> dole najdeš <strong>CS:GO 1080p</strong> a <strong>Spider-Man 540p</strong>. Kliknutím přepneš přibalené video.</p><p>Pro tuto zkoušku nepotřebuješ capture kartu ani další stahování. Tlačítko <strong>Vyzkoušet titulky EN &gt; CZ</strong> použij později pro kontrolu překladu.</p>', '<p>At the bottom of <strong>Capture / Vstup</strong>, use <strong>CS:GO 1080p</strong> or <strong>Spider-Man 540p</strong> to switch bundled videos.</p><p>No capture card or further download is needed. The <strong>Vyzkoušet titulky EN &gt; CZ</strong> button runs the translation test used below.</p>'),
  ('Obě videa se přehrávají přímo ze složky aplikace.', 'Both videos play directly from the application folder.'), ['samples']),
 ('fg', ('Zapni frame generation','Enable frame generation'),
  ('<p>Otevři <strong>Frame gen</strong>, zvol <strong>NVIDIA VFG (RTX 40/50)</strong> a zaškrtni <strong>Generování snímků</strong>. Začni kvalitou <strong>Střední</strong> a násobičem <strong>2×</strong>.</p><p>Alternativou je spouštěč <code>2 - NVIDIA RTX 40-50.cmd</code>. Optical Flow je jiný backend, ne další efekt navíc. 3× a 4× mohou přidávat duchy a latenci.</p>', '<p>Open <strong>Frame gen</strong>, choose <strong>NVIDIA VFG (RTX 40/50)</strong> and enable <strong>Frame generation</strong>. Start with <strong>Medium / Střední</strong> and <strong>2×</strong>.</p><p>Alternatively use <code>2 - NVIDIA RTX 40-50.cmd</code>. Optical Flow is an alternative backend, not a second stacked effect. 3× and 4× may increase ghosting and latency.</p>'),
  ('Dole je stav NVIDIA VFG active. Počítadlo generovaných snímků se mění.', 'The status says NVIDIA VFG active. The generated-frame counter updates.'), ['fg']),
 ('wizard', ('Stáhni externí soubory enhanceru','Download external enhancer files'),
  (f'<p>Ukonči FrameLab. V jeho složce otevři <code>4 - Doplnit enhancer.cmd</code>. První tlačítko průvodce otevře přesný externí archiv.</p><p><a href="{VEYRA}">Veyra 2.1.9 NVIDIA portable — 731 MB ↓</a></p><p><strong>Tento ZIP nerozbaluj.</strong> Veyru nemusíš spouštět. Jde o komunitní experiment; práva k použití a redistribuci těchto NR DLL zde nejsou potvrzena. Odkaz ani import licenci neuděluje. <a href="enhancer.html">Původ a podmínky</a>.</p>', f'<p>Close FrameLab. Open <code>4 - Doplnit enhancer.cmd</code> in its folder. The first wizard button opens the exact external archive.</p><p><a href="{VEYRA}">Veyra 2.1.9 NVIDIA portable — 731 MB ↓</a></p><p><strong>Keep this ZIP compressed.</strong> You do not need to run Veyra. This is a community experiment; rights to use and redistribute these NR DLLs have not been established here. Linking/importing does not grant a license. <a href="enhancer-en.html">Source and terms</a>.</p>'),
  ('Máte přesně Veyra-2.1.9-NVIDIA-win64-portable.zip. Verze AMD, Virtual Camera ani Source code nevyhovují.', 'You have exactly Veyra-2.1.9-NVIDIA-win64-portable.zip. AMD, Virtual Camera and Source code variants are not accepted.'), ['wizard']),
 ('import', ('Vyber ZIP a dokonči import','Select the ZIP and finish import'),
  ('<p>Ve stejném průvodci klikni na <strong>2 — Vybrat stažený ZIP a doplnit soubory</strong>. Najdi stažený archiv, označ jej a klikni <strong>Otevřít</strong>.</p><p>Počkej na kontrolu a kopírování. Není potřeba ručně hledat složky DLL ani instalovat ovladač. Chyba kontrolního součtu znamená neúplný nebo jiný archiv.</p>', '<p>In the same wizard, click <strong>2 — Vybrat stažený ZIP a doplnit soubory</strong>. Locate the downloaded archive, select it and click <strong>Open / Otevřít</strong>.</p><p>Wait for verification and copying. No manual DLL placement or driver installation is needed. A checksum error means the archive is incomplete or different.</p>'),
  ('Objeví se Soubory jsou připravené. Potom průvodce zavři a znovu spusť FrameLab.', 'The wizard says Soubory jsou připravené (Files are ready). Close it and restart FrameLab.'), ['select','ready']),
 ('nr', ('Zapni NR a porovnej stejný snímek','Enable NR and compare the same frame'),
  ('<p>Otevři <strong>AI / NR → Zapnout enhancer</strong>. Začni silou <strong>65 %</strong>, <strong>Děleným porovnáním</strong> a v části <strong>Kvalita a výkon</strong> limitem <strong>720p</strong>. První načtení modelu může chvíli trvat.</p><p><strong>F8</strong> zmrazí stejný pár, hranici posuneš tažením čáry v náhledu nebo posuvníkem <strong>Dělicí čára</strong>. <strong>F7</strong> přepíná pohledy, <strong>F6</strong> zapíná/vypíná efekt. Na testované RTX 4050 byl použit výchozí ShortFuse NR; původní NVIDIA profil je určen pro RTX 50 a zde není otestovaný.</p><p>Vstup → vybraný frame gen → NR → český překryv → monitor. Původní obraz v NR porovnání může už obsahovat frame gen. Při nízkém výkonu sniž AI limit na 540p/360p.</p>', '<p>Open <strong>AI / NR → Enable enhancer</strong>. Start with <strong>65%</strong>, <strong>Split comparison</strong> and the <strong>720p</strong> cap in Quality and performance. Initial model loading may take a while.</p><p><strong>F8</strong> freezes the matching pair; drag the divider in the preview or use its slider. <strong>F7</strong> switches views and <strong>F6</strong> toggles the effect. The tested RTX 4050 used the default ShortFuse NR profile. The original NVIDIA profile targets RTX 50 and was not tested here.</p><p>Input → selected frame generation → NR → Czech overlay → monitor. The NR original may already be a generated frame. Reduce the AI cap to 540p/360p if performance is low.</p>'),
  ('Vidíš označení PŮVODNÍ a UPRAVENÝ, dělicí čáru a aktivní zaškrtávací políčko. Změna nemusí být vždy zlepšení.', 'You see ORIGINAL and ENHANCED labels, a divider and the enabled checkbox. A visible change is not always an improvement.'), ['nr']),
 ('ocr', ('Zapni lokální české titulky','Enable local Czech subtitles'),
  ('<p>Ve <strong>Vstupu</strong> spusť <strong>Vyzkoušet titulky EN &gt; CZ</strong>. Pak otevři <strong>Titulky</strong> a zkontroluj <strong>Překládat titulky do češtiny</strong>.</p><p>Pro vlastní hru zapni <strong>Zobrazit oblast čtení</strong> a hranicemi vyber pouze pruh s anglickými titulky. Vynech HUD a chat. Pod oblastí nastavíš velikost a polohu českého textu.</p><p>Pokud chybí anglické OCR, použij nabídnuté jazykové nastavení Windows a přidej <strong>English (United States)</strong> s potřebnými jazykovými funkcemi. Potom aplikaci restartuj. Model je přibalený; překlad běží offline. Zvuk se nepřepisuje.</p>', '<p>In <strong>Capture / Vstup</strong>, run <strong>Vyzkoušet titulky EN &gt; CZ</strong>. Open <strong>Subtitles / Titulky</strong> and check <strong>Translate subtitles to Czech</strong>.</p><p>For your own game, enable the OCR-region outline and select only the English subtitle strip, excluding HUD and chat. Adjust Czech text size and position below the region controls.</p><p>If English OCR is missing, open the Windows language settings offered by the app and add <strong>English (United States)</strong> with the required language features, then restart. The translation model is bundled and runs offline. Audio is not transcribed.</p>'),
  ('V testu se pod anglickým textem zobrazí Dveře jsou zamčené. Překlad má krátké zpoždění a může se mýlit.', 'The test shows Dveře jsou zamčené below the English sentence. Translation has a short delay and can be inaccurate.'), ['ocr']),
 ('capture', ('Připoj skutečnou hru přes capture kartu','Connect a real game through capture'),
  ('<p>Zapoj <strong>konzoli / druhé PC → HDMI vstup capture karty → USB/PCIe do PC s FrameLabem</strong>. Zavři jiné aplikace, které kartu používají.</p><p>V kartě <strong>Vstup</strong> vyber svou kartu, rozlišení a FPS nabízené jejím ovladačem. Začni automatickým režimem nebo podporovaným 1080p. Klikni <strong>Spustit vstup</strong>. V kartě <strong>Zvuk</strong> nastav výstup a hlasitost.</p><p>Na tomto notebooku nebyla připojena capture karta; snímek ukazuje umístění ovládání s dostupnou kamerou ACER. Živý HDMI vstup ani zvuk nejsou tímto průchodem ověřené. HDR není implementované.</p>', '<p>Connect <strong>console / second PC → capture-card HDMI input → USB/PCIe to the FrameLab PC</strong>. Close other applications using the card.</p><p>In <strong>Capture / Vstup</strong>, choose your card and driver-reported resolution/FPS. Begin with automatic or supported 1080p mode. Click <strong>Start capture / Spustit vstup</strong>. Select output and volume in <strong>Audio / Zvuk</strong>.</p><p>No capture card was attached to this laptop; the screenshot shows control locations with its ACER camera listed. Live HDMI capture and audio were not verified in this walkthrough. HDR is not implemented.</p>'),
  ('V náhledu běží tvůj zdroj. Při černém obrazu ověř signál, kabel, ovladač a obsazení karty jinou aplikací.', 'Your source appears in the preview. For a black image check the signal, cable, driver and whether another app is using the card.'), ['capture']),
 ('display', ('Vyber monitor a otevři celou obrazovku','Choose a monitor and go fullscreen'),
  ('<p>V kartě <strong>Zobrazení</strong> nastav měřítko obrazu a monitor. <strong>F11</strong> otevře hlavní obraz přes celou obrazovku; <strong>Escape</strong> se vrátí. Porovnání, ovladače i rámeček OCR se skryjí automaticky. Kurzor zmizí po 2 sekundách nečinnosti. <a href="fullscreen.html">Podrobný obrazový návod fullscreen →</a>.</p><p>Upravený obraz sleduj ve FrameLabu. Chceš-li ho na TV, připoj TV k výstupu grafiky tohoto PC. HDMI passthrough ze střihovky efekty FrameLabu neobsahuje.</p><p>Rozlišení a obnovovací frekvenci samotného monitoru nastavuješ ve Windows. FrameLab je tímto přepínačem nemění.</p>', '<p>In <strong>Display / Zobrazení</strong>, choose image scaling and monitor. <strong>F11</strong> opens the main image fullscreen; <strong>Escape</strong> returns. Comparison views, controls and the OCR outline hide automatically. The cursor hides after 2 idle seconds. <a href="fullscreen-en.html">Detailed illustrated fullscreen guide →</a>.</p><p>Watch the processed output in FrameLab. To use a TV, connect it to this PC’s GPU output. Capture-card HDMI passthrough does not include FrameLab effects.</p><p>Set the actual monitor resolution and refresh rate in Windows; FrameLab does not change them with this selector.</p>'),
  ('Hraješ přes zpracovaný výstup aplikace. Pro návrat k nastavení stiskni Escape.', 'You are viewing the processed application output. Press Escape to return to the controls.'), ['display']),
]

def figure(key, lang='cs', suffix=''):
    idx = int(lang == 'en')
    filename, (width,height), crop, marks, caption = SHOTS[key]
    view = crop or (0,0,width,height)
    marker = 'arrow-' + key + suffix
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{" ".join(map(str,view))}" role="img" aria-label="{html.escape(caption[idx],quote=True)}"><defs><marker id="{marker}" markerWidth="9" markerHeight="9" refX="7" refY="3" orient="auto"><path d="M0,0 L0,6 L8,3 Z" fill="#b8ff51"/></marker></defs><image href="assets/{filename}" width="{width}" height="{height}"/>'
    legends = []
    for n,(label,cx,cy,x,y,w,h) in enumerate(marks,1):
        # Pick the nearest rectangle edge, so arrows stop before the actual label.
        tx = max(x,min(cx,x+w)); ty = max(y,min(cy,y+h))
        svg += f'<g aria-hidden="true"><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="3" fill="none" stroke="#101511" stroke-width="6"/><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="3" fill="none" stroke="#b8ff51" stroke-width="3"/><line x1="{cx}" y1="{cy}" x2="{tx}" y2="{ty}" stroke="#101511" stroke-width="7"/><line x1="{cx}" y1="{cy}" x2="{tx}" y2="{ty}" stroke="#b8ff51" stroke-width="3" marker-end="url(#{marker})"/><circle cx="{cx}" cy="{cy}" r="19" fill="#b8ff51" stroke="#101511" stroke-width="2"/><text x="{cx}" y="{cy+7}" text-anchor="middle" font-family="system-ui,sans-serif" font-size="22" font-weight="800" fill="#101511">{n}</text></g>'
        legends.append(f'<li><b>{n}</b> {html.escape(label[idx])}</li>')
    svg += '</svg>'
    zoom = ('Kliknutím zvětšit ↗','Click to enlarge ↗')[idx]
    return f'<figure class="tutorial-figure" id="shot-{key}{suffix}"><button type="button" class="shot-button" aria-label="{zoom}">{svg}<span class="zoom-hint">{zoom}</span></button><ul class="callout-legend">{"".join(legends)}</ul><figcaption>{html.escape(caption[idx])}</figcaption></figure>'

def lightbox(lang='cs'):
    label = 'Close / Escape' if lang == 'en' else 'Zavřít / Escape'
    return f'<dialog class="guide-lightbox" aria-label="Screenshot detail"><button type="button" class="lightbox-close" autofocus>{label}</button><div class="lightbox-body"></div></dialog><script src="assets/visual.js" defer></script>'

def guide_banner(lang='cs'):
    if lang=='en': return '<p class="notice"><a class="guide-cta" href="visual-en.html">Illustrated walkthrough: real Windows screenshots →</a><br>Download, extraction, frame generation, NR import and Czech subtitles. Click any screenshot to enlarge it. <a href="fullscreen-en.html">Clean fullscreen →</a></p>'
    return '<p class="notice"><a class="guide-cta" href="visual.html">Obrazový návod: skutečný průchod Windows krok za krokem →</a><br>Stažení, rozbalení, frame gen, import NR a české titulky. Každý obrázek jde zvětšit. <a href="fullscreen.html">Čistý fullscreen →</a></p>'

def main():
    for lang in ('cs','en'):
        idx = int(lang=='en'); name = 'visual-en.html' if idx else 'visual.html'
        title = ('Od stažení až po první český titulek.','From download to your first Czech subtitle.')[idx]
        subtitle = ('Obrazový návod pro FrameLab preview.12. Skutečné screenshoty Windows 11, pořízené 19. 11. 2026 při ovládání aplikace. Čísla a šipky jsou samostatná grafická vrstva; obsah rozhraní není přegenerovaný.', 'Illustrated guide for FrameLab preview.12. Real Windows 11 screenshots captured on 19 November 2026 while operating the app. Numbers and arrows are a separate layer; the interface pixels are not regenerated.')[idx]
        textlink = 'en.html' if idx else 'index.html'
        toc = ''.join(f'<a href="#{s[0]}">{i:02d} · {html.escape(s[1][idx])}</a>' for i,s in enumerate(STEPS,1))
        sections=[]
        for i,(key,heading,body,expected,shots) in enumerate(STEPS,1):
            sections.append(f'<section class="step" id="{key}"><div class="step-copy"><span class="step-index">{i:02d} / 12</span><h2>{heading[idx]}</h2>{body[idx]}<p class="expected"><strong>{("Co máš vidět:","Expected result:")[idx]}</strong> {expected[idx]}</p></div><div class="shots">'+''.join(figure(s,lang,'-visual') for s in shots)+'</div></section>')
        notice = ('Preview.12 přidává čistý fullscreen, tažení dělicí čáry a tyto návody i offline. Instalační kroky byly nafocené v preview.11; nový fullscreen v preview.12. Frame gen, model překladu a obě herní ukázky jsou přibalené. Externí NR soubory a jejich oprávnění zůstávají samostatné.', 'Preview.12 adds clean fullscreen, a draggable divider and these offline guides. Installation steps were captured in preview.11; the new fullscreen in preview.12. Frame generation, the translation model and both game demos are bundled. External NR files and their permissions remain separate.')[idx]
        footer = ('Ověřeno: rozbalení Průzkumníkem do krátké cesty, oba klipy, NVIDIA VFG 2×, import přes grafický průvodce a NR + lokální OCR s českým překladem na RTX 4050 Laptop. Fyzická capture karta nebyla připojená. Herní obraz má práva původních vlastníků; viz licence a původ ukázek v balíku.', 'Verified: Explorer extraction to a short path, both clips, NVIDIA VFG 2×, graphical-wizard import and NR + local OCR/Czech translation on RTX 4050 Laptop. No physical capture card was attached. Game imagery retains its owners’ rights; see bundled licenses and sample provenance.')[idx]
        page=f'<!doctype html><html lang="{lang}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab — {title}</title><link rel="stylesheet" href="assets/visual.css"></head><body><main><nav><a href="../#download">← FrameLab</a><a href="visual.html">Česky</a><a href="visual-en.html">English</a><a href="{textlink}">{("Textový návod","Text guide")[idx]}</a></nav><header><p class="eyebrow">FRAME/LAB · WINDOWS · PREVIEW.12</p><h1>{title}</h1><p class="intro">{subtitle}</p><div class="notice">{notice}</div></header><nav class="toc" aria-label="Steps">{toc}</nav>{"".join(sections)}<p class="footer-note">{footer}</p></main>{lightbox(lang)}</body></html>'
        (ROOT/'website/guide'/name).write_text(page,encoding='utf-8')

if __name__=='__main__': main()

# FrameLab preview.12 — čistý fullscreen a obrazové návody

**[Kompletní Windows x64 ZIP](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.12/FrameLab-0.2.0-preview.12-windows-x64-full.zip)** · **[Obrazový návod](https://opicebot.cz/framelab/guide/visual.html)** · **[Fullscreen krok za krokem](https://opicebot.cz/framelab/guide/fullscreen.html)**

- **F11 automaticky vypne zobrazení všech porovnání.** Ukáže jediný výsledný obraz s nastaveným framegenem, enhancerem a českými titulky. Skryje dělicí čáru, RAW/FG/RAW náhledy, panely, popisky a pomocný rámeček OCR. Zmrazené porovnání se při přechodu rozběhne.
- **Esc / F11 vrátí nastavení.** Původně vybraný režim porovnání zůstane dostupný v okně. F1 v režimu celé obrazovky nevyvolává panely.
- **Ostřejší české titulky:** samostatné rastry písma podle výstupní velikosti místo zvětšování malého fontu rozhraní. České znaky a rozlišení fontů pro 1080p/4K jsou testované.
- **Kurzor se nad fullscreenem skryje po 2 sekundách nečinnosti** a pohybem se zobrazí. Nastavení kurzoru ostatních aplikací se nemění.
- **Dělicí čáru NR v náhledu lze táhnout myší.** Síla efektu i režim porovnání se ukládají.
- **Skutečné screenshoty Windows s čísly a šipkami vedle textu**, zvětšení obrázků, česká a anglická verze, kompletní cesta od stažení přes rozbalení po import enhanceru. Samostatný návod monitoru, měřítka a fullscreenu.

Všechny obrázky a osm HTML návodů jsou i přímo v ZIPu: otevři **ZACNI TADY.html**, **OBRAZOVY NAVOD.html** nebo **FULLSCREEN.html**. Potřebné obrázky ani styly se z internetu nestahují. Balík stále obsahuje obě herní ukázky, NVIDIA VFG, překladový model s Pythonem, umělý OCR test, exportér, průvodce NR a licence. Novou verzi rozbal do nové krátké složky na Ploše.

Externí NR modelové DLL nejsou redistribuované. Přibalený průvodce nabízí přesný komunitní download a kontrolovaný import; odkaz ani import neudělují licenci. [Podmínky a celý postup](https://opicebot.cz/framelab/guide/enhancer.html).

Ověřeno na Windows 11 / RTX 4050 Laptop: NVIDIA VFG + NR + lokální české titulky, automatické skrytí porovnání a OCR rámečku, návrat přes F11 i Esc, pokračování zmrazeného obrazu a hlášení Windows o skrytém kurzoru. GPU test čte skutečné výstupní pixely a kontroluje sílu efektu i nepřítomnost dělicí čáry ve fullscreenu; devět CTest testů prošlo. Fyzická capture karta a druhý monitor v tomto průchodu připojené nebyly.

## English

F11 now always shows one final processed image. Preview comparisons, controls, labels and the OCR outline disappear automatically; a frozen comparison resumes. Esc/F11 returns to settings with the chosen window comparison preserved. The cursor hides after two idle seconds over fullscreen and returns on movement. The NR divider can be dragged in the window preview.

Czech subtitles now use dedicated high-resolution font atlases rather than enlarging the small UI font. The full package includes illustrated CZ/EN installation, enhancer and fullscreen guides with original screenshots and separate annotations, available online and offline. **[English walkthrough](https://opicebot.cz/framelab/guide/visual-en.html)** · **[English fullscreen guide](https://opicebot.cz/framelab/guide/fullscreen-en.html)**. External NR runtime/model DLLs remain a separate download with separate, unconfirmed permissions; they are not included in this release.

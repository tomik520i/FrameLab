# FrameLab se všemi funkcemi: instalace enhanceru

**Dva ZIPy a jeden průvodce.** Nic nekompiluješ, neinstaluješ Python ani SDK a nehledáš složky pro DLL. Hlavní FrameLab už obsahuje frame generation, dvě herní ukázky, český překlad včetně modelu, exportér a podporu NR. Externí NR soubory doplní průvodce.

**NR je komunitní experiment.** Ověřený veřejný instalační balík DLSS 5 NR přímo od NVIDIA k 19. 11. 2026 není dostupný. Níže uvedený druhý ZIP poskytuje Veyra, nikoli NVIDIA ani FrameLab. ShortFuse je upravený NVIDIA runtime. Práva k použití a redistribuci těchto souborů zde nejsou potvrzena; odkaz ani import je neuděluje. Tuto cestu použij, pokud k souborům máš příslušné oprávnění. [Vyjádření NVIDIA](https://forums.developer.nvidia.com/t/official-dlss-5-neural-rendering-plugin-access-for-unreal-engine-5-8-2/382731/11) · [Původ a podmínky komponent Veyry](https://github.com/Likely7/Veyra-NRVideo/blob/33dfdf71197e3056e250465150eb9541d0bb24ce/THIRD_PARTY_NOTICES.md#user-provided-shortfuse-nr-310916-2026-10-17-local-testing).

## 1. Stáhni a rozbal FrameLab

1. **[Stáhnout FrameLab preview.13 pro Windows x64](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.13/FrameLab-0.2.0-preview.13-windows-x64-full.zip)**. Přesná velikost je na [stránce stahování](https://opicebot.cz/framelab/#download).
2. Pravé tlačítko na ZIP → **Extrahovat vše** → nová složka například na Ploše. Nepracuj uvnitř ZIPu. Počítej přibližně se 4 GB volného místa pro oba stažené archivy, rozbalení a import.
3. Otevři **1 - Spustit FrameLab.cmd**. V kartě **Vstup** vyzkoušej CS:GO nebo Spider-Mana. Frame generation i titulky lze používat hned, NR ještě čeká na externí soubory. Pro OCR může Windows vyžadovat doinstalování anglického rozpoznávání textu; postup je v [základním návodu](https://opicebot.cz/framelab/guide/).

## 2. Stáhni přesný externí balík

- **[Externí download: Veyra-2.1.9-NVIDIA-win64-portable.zip — 731 MB](https://github.com/Likely7/Veyra-NRVideo/releases/download/v2.1.9/Veyra-2.1.9-NVIDIA-win64-portable.zip)**.
- [Stránka vydání autora a seznam souborů](https://github.com/Likely7/Veyra-NRVideo/releases/tag/v2.1.9). Když přímý odkaz nefunguje, otevři **Assets** a najdi přesně uvedené jméno.
- Nepoužívej AMD, Source code ani variantu s dodatkem Virtual Camera. Průvodce přijímá pouze tento ověřený archiv. **ZIP nerozbaluj a Veyru nemusíš spouštět.**
- Tento download není na našem serveru. Oficiální [NVIDIA DLSS download](https://developer.nvidia.com/rtx/dlss) nyní obsahuje jiné technologie DLSS; libovolné `nvngx_dlss.dll`, `nvngx_dlssd.dll` nebo `nvngx_dlssg.dll` tento NR soubor nenahradí.

## 3. Průvodce vše zkopíruje

1. Ukonči FrameLab. Ve složce FrameLabu otevři **4 - Doplnit enhancer.cmd**. Stejný průvodce jde otevřít přes **AI / NR → Doplnit / zkontrolovat enhancer**; před importem pak FrameLab zavři.
2. Klikni **2 - Vybrat stažený ZIP a doplnit soubory** a vyber stažený `Veyra-2.1.9-NVIDIA-win64-portable.zip`.
3. Počkej na **Soubory jsou připravené**. Kontroluje se SHA-256 celého ZIPu i obou DLL; při chybě se žádný neověřený soubor neinstaluje. Licence a původ se přenesou spolu se soubory. Neinstaluje se ovladač ani systémová služba a není potřeba administrátor.
4. Znovu otevři **1 - Spustit FrameLab.cmd**, přejdi do **AI / NR** a zapni **Zapnout enhancer (F6)**. Pokud se GPU inicializace nepovede, enhancer se vypne a ukáže konkrétní chybu; původní obraz zůstane dostupný.

## 4. První porovnání

- Začni limitem **720p**, silou **65 %** a **Děleným porovnáním**. Dělicí čáru posouvej posuvníkem. **F8** zmrazí stejný snímek, **F7** přepíná původní / upravený / dělený obraz, **F6** efekt vypne.
- V **Runtime / diagnostika** je výchozí **ShortFuse NR 310.9.16**, komunitní varianta ověřená zde na RTX 4050 Laptop a RTX 5070 Ti. **NVIDIA NR 310.8 original** byla na RTX 5070 Ti ověřena i se dvěma navazujícími průchody. Ostatní karty tím nejsou potvrzené.
- Při zapnutém frame genu platí **vstup → frame gen → enhancer → české titulky → monitor**. Původní obraz v NR porovnání znamená snímek před enhancerem, tedy případně už generovaný. Pomalé AI může snížit počet zobrazených snímků a zvýšit latenci.
- **[Skutečné porovnání před / po s posuvnou lištou](https://opicebot.cz/framelab/#enhancer)**. Změna pixelů není automaticky lepší obraz; kontroluj obličeje, HUD a rychlý pohyb.

## Když se něco nepovede

| Problém | Co udělat |
| --- | --- |
| Nemám kartu AI / NR | Máš preview.10 nebo starší build bez podpory NR. Stáhni celý preview.13; samotné přidání DLL do preview.10 nestačí. |
| Kontrolní součet nesouhlasí | Stáhni přesný NVIDIA ZIP 2.1.9 bez dodatku. Zkontroluj dokončené stahování. Průvodce úmyslně nepřijímá neznámé verze. |
| Soubor se používá / přístup odepřen | Ukonči všechny své instance FrameLabu a zkus import znovu. Celá složka musí být zapisovatelná. |
| Průvodce je zablokovaný správcem | Neměň zásady počítače. Se souhlasem správce rozbal přesný externí ZIP a použij ruční cesty níže. |
| Model nejde inicializovat | Ověř NVIDIA ovladač a profil. Zkus 360p/540p, uvolni paměť GPU a začni s FG 2×. Konkrétní chybu najdeš v AI / NR a `app/nr-cache/nr-host.log`. |
| Stažení Veyry už není dostupné | Použij vlastní dříve získaný ověřený ZIP, pokud k němu máš oprávnění. Bez kompatibilního runtime NR nefunguje; zbytek FrameLabu dál ano. Neověřené DLL z náhodných webů nejsou náhrada. |

Ruční import pro správce: z kořene rozbalené Veyry zkopíruj `runtime/experimental/nr-ampere/nvngx_dlssnr.dll` do `FrameLab/app/runtime/nr-sf/nvngx_dlssnr.dll` a `runtime/experimental/nr-original/nvngx_dlssnr.dll` do `FrameLab/app/runtime/nr-original/nvngx_dlssnr.dll`. Zachovej původní licenční soubory z `licenses` pod `FrameLab/app/licenses/nr-external`. Nezaměňuj tyto DLL za přibalený `app/nr-host/nvngx.dll`: ten je malý adaptér FrameLabu, nikoli AI model ani ovladač NVIDIA.

Ověřený externí ZIP: 731012476 bajtů; SHA-256 `4225ecb11a8e96c2826b4b0b798feb811d7ae237f3edf0b627f3b6895c79f362`. Přesné identity obou DLL jsou v [auditu](NR-LICENSING-STATUS.txt). Přesun downloadu na cizí web není vyřešení licence; veřejný FrameLab tyto DLL nepřebaluje ani neskrývá jejich původ.

**Celá obrazovka:** F11 automaticky skryje porovnání a ukáže jeden výsledný obraz s nastavenou silou enhanceru. Esc/F11 vrátí nastavení. [Obrazový návod fullscreen](https://opicebot.cz/framelab/guide/fullscreen.html).

## Počet průchodů NR

V **AI / NR → Počet průchodů NR** vyber **1, 2, 3 nebo 4**. Výchozí je jeden, i při načtení staršího nastavení. Není to násobič frame genu: dva průchody upraví snímek dvakrát, nevytvoří dvojnásobné FPS.

Každý průchod dostává plný výsledek předchozího a má vlastní historii obrazu. **Síla efektu** se přimíchá až na konečný výsledek. F7/F8 umožní porovnat stejný vstupní snímek. Údaj **celkem … ms** měří všechny průchody posledního dokončeného snímku včetně čekání na GPU. Není to čas jednoho průchodu ani celková latence.

Více průchodů potřebuje více výkonu i VRAM a může zesílit artefakty. Zkus dva při stejném limitu 720p a stejné síle. Při propadu výstupních FPS vrať jeden nebo sniž výpočetní rozlišení. Původní a ShortFuse profil se mohou lišit. Starší NR porovnání na webu používají **jeden průchod**, nejsou tedy důkazem kvality více průchodů.

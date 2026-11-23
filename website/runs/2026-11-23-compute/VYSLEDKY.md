# Lokální překlad přes CUDA, Vulkan a CPU

Součást veřejného FrameLab preview.18. Měření níže pochází z vývojové verze z 23. 11. 2026; nová verze tím nemění původní naměřené hodnoty.

## Jak zapnout

1. V **Titulky → Způsob překladu** zvol **Local / offline**.
2. Vyber **CUNI-MH** (EN → CS) nebo **TranslateGemma** (další podporované jazyky). Používají se stejné ověřené soubory GGUF i stávající tlačítko stažení modelu.
3. V **Výpočet překladu** vyber **Automaticky**, **CUDA / NVIDIA**, **Vulkan / AMD, Intel, NVIDIA** nebo **CPU / RAM**.
4. Pro CUDA/Vulkan lze zvolit konkrétní **GPU pro překlad**. Tato volba je nezávislá na grafice pro obraz, frame gen a enhancer. Výběr se ukládá podle stabilního UUID, nikoli pořadového čísla; za běhu se překládá na aktuální index zařízení. Obnovení seznamu znovu načte zařízení a volnou paměť CUDA.
5. Standardně se kontroluje celý model na GPU. Volba **Povolit GPU + RAM (pomalejší)** dovoluje runtime část modelu zpracovat na CPU. Pokud se model načte jen na CPU navzdory zvolenému GPU, aplikace to odmítne; pro čistý CPU režim použij explicitní volbu CPU. Skutečný backend a umístění se zobrazí po načtení.
6. Zapni **Překládat titulky**. Změna backendu nebo GPU překlad vypne, aby šel nový výběr zkontrolovat před načtením modelu.

**OPUS-MT / CPU** zůstává samostatný lehký překladač na CPU; uvedený výběr backendu se týká GGUF modelů CUNI/Gemma. Režim CPU / RAM tedy znamená původní zvolený GGUF model, nikoli jeho automatickou náhradu za OPUS.

## Instalace a hardware

Připnutý přenosný runtime Ollama 0.40.16 už obsahuje Vulkan. CUDA Toolkit ani Vulkan SDK se kvůli tomuto režimu neinstalují. Je potřeba funkční aktuální ovladač GPU s Vulkanem. První stažení GGUF modelu stáhne i společný přenosný runtime; existující modely se nestahují znovu. Chybějící Vulkan DLL je rozpoznána jako neúplný runtime a obslouží ji oprava přes stávající stažení/ověření.

Automatika upřednostní samostatnou GPU, na NVIDIA CUDA, na dalších zařízeních Vulkan; pokud GPU nenajde, zvolí CPU a uvede to ve stavu. Je to volba dostupného zařízení, nikoli automatický benchmark nejrychlejšího řešení. Integrovaná GPU používá sdílenou RAM a může být pomalejší než CPU. Na tomto konkrétním Intel UHD tomu tak bylo.

Vulkan podpora se týká překladu. Podpora NR enhanceru a jednotlivých frame-gen backendů se tím nemění. AMD GPU zde nebyla fyzicky k dispozici; byla připravena Vulkan cesta, nikoli ověřena konkrétní Radeon karta. [Oficiální dokumentace Ollama / Vulkan](https://docs.ollama.com/gpu#vulkan-gpu-support).

## Orientační porovnání

32 překladů: čtyři stejné krátké repliky na každé konfiguraci. Nejde o další 500položkový kvalitativní benchmark ani měření hry s FG/NR. Vzorky byly překládány bez OCR, FG a enhanceru, postupně, na běžícím desktopu. Kontext 2048 tokenů; stejné GGUF soubory a prompty. CUDA měla zapnutý Flash Attention, Vulkan/CPU vypnutý. V tabulce je medián samotného synchronního překladu; start a zahřátí modelu jsou zvlášť v datech.

| Model | CUDA / RTX 4050 | Vulkan / RTX 4050 | Vulkan / Intel UHD | CPU / i5-13420H |
|---|---:|---:|---:|---:|
| CUNI-MH 7B Q3_K_M | 0.74 s | 5.26 s | 21.50 s | 4.28 s |
| TranslateGemma 4B Q4_K_M | 0.49 s | 2.52 s | 11.46 s | 3.53 s |

CUNI v těchto čtyřech případech vytvořilo na všech cestách stejné výstupy. U Gemmy se dvě CPU formulace lišily; rozdíly numeriky/backendu mohou ovlivnit generování i při nulové teplotě. Vulkan sám nezvyšuje kvalitu jazyka. Na tomto notebooku doporučujeme CUDA; Intel UHD přes Vulkan funguje, ale naměřená prodleva se pro běžné živé titulky nehodí. Naměřené časy z této malé sady nepředpovídají výkon AMD ani Intel Arc.

## Ověření celé aplikace

- Skutečné obrázky → RapidOCR → filtr → pracovní proces → CUNI na Intel UHD / Vulkan: **6/6** případů. Stejná cesta s TranslateGemma na RTX 4050 / Vulkan: **6/6**. Dva samostatné názvy osob se odfiltrují; oslovení, věta s číselným kódem, „No!“ a pokyn bez interpunkce se přeloží. Tyto funkční testy nejsou měřením FPS hry.
- U pomalých modelů se starší výsledek nezahodí, pokud čerstvé OCR stále potvrzuje tentýž text. Zmizelý nebo změněný text se tím neudržuje neomezeně; reset zdroje/modelu ruší staré výsledky. Regresní test pokrývá potvrzený, změněný, prázdný i zastaralý OCR vstup. Skutečná prodleva se stále měří od původního snímku.
- Rozdělené načtení **GPU + RAM** skutečně přeložilo větu: testovací harness vynutil 8 GPU vrstev CUNI na RTX / Vulkan, runtime ohlásil 1 089 522 892 bajtů na GPU z celkových 4 021 697 902. Tím se ověřilo přijetí částečného načtení a inference; automatické rozhodování při nedostatku VRAM nebylo samostatně zátěžově změřeno. Běžná volba v aplikaci nechává počet vrstev runtime.
- Nativní sada **18/18** prošla; po opravě životnosti titulků znovu prošel cílený regresní test. Python ověřuje stabilní GPU UUID, oddělení procesního prostředí, explicitní umístění modelu, start a ukončení runtime. Testy stahování ověřily navázání, kontrolní součet a opravu chybějící Vulkan knihovny; během prvních pokusů docházelo k resetům místního HTTP spojení, finální cílené běhy prošly.
- Nabídka backendů a Intel/RTX byla zkontrolována v živé aplikaci; volba Intel přežila zavření a nové spuštění. Aplikace je ponechána na **Automaticky**, což zde vybere CUDA / RTX 4050, s překladem vypnutým do ručního spuštění.

Záznamy finálních průchodů: `native-vulkan-intel-freshness.log`, `native-vulkan-rtx-freshness.log`, `partial-placement.json`. Starší neúspěšné diagnostické běhy jsou ponechány odděleně a nejsou vydávány za úspěšné výsledky. Naměřená kompatibilita se týká CUNI Q3 a Gemma 4B Q4, nikoli všech větších variant nebo všech starších GPU.

## Technická implementace

- `tools/subtitle_compute.py`: NVML UUID pro CUDA, Vulkan 1.1 deviceUUID a fyzické pořadí pro Vulkan; softwarové Vulkan CPU adaptéry nejsou vydávány za GPU. Výběr platí jen pro soukromý proces FrameLabu, nemění systémové proměnné.
- `tools/subtitle_gpu.py`: vlastní loopback runtime, kontrola vybraného zařízení, explicitní CPU a volitelný offload, skutečná velikost umístěná na GPU z `/api/ps`, diagnostika `logs/subtitle-compute-*.json`. Pro Vulkan se povolí i integrovaná GPU; cizí CUDA/ROCm/Vulkan masky a vynucení knihoven se pro tento proces vyčistí.
- `SubtitleSettings` verze 12 zachovává starší nastavení; změna zařízení načte model znovu. Ovládání v `SubtitleComputeControls.hpp`; backend i cílová GPU se předávají pracovnímu procesu. Název GPU zůstává ve stavu i po resetu OCR/zdroje.
- Balicí skripty zahrnují nový modul. Změna nevyžaduje cloudový překlad ani OpenAI klíč.

Originální vstupy této malé sady jsou w500-005, w500-006, w500-008 a w500-009 z dříve zmrazeného Wesnoth datasetu. Překlady, přesné časy, GPU paměť a inicializace jsou v místním `compute-results.json` a jednotlivých záznamech. Kompletní předchozí kvalitativní test zůstává na https://opicebot.cz/framelab/runs/2026-11-23-api500/.

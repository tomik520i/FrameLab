# Podpora GPU — preview.18

Generování snímků, překlad a enhancer mají odlišné hardwarové požadavky. Podpora RIFE na určité grafice neznamená podporu NR ani GPU překladu.

| Funkce | Výpočetní cesta | Zde ověřeno |
|---|---|---|
| D3D11 Optical Flow | Vhodný hardware Direct3D 11; bez runtime určeného pouze pro NVIDIA | Intel UHD + RTX 4050 |
| RIFE 4.22 lite / DirectML | Vhodná hardwarová GPU s DirectX 12 a ovladačem; nevyžaduje CUDA | Intel UHD + RTX 4050 |
| NVIDIA VFG | RTX 40/50 | RTX 4050; starší měření na RTX 5070 Ti |
| Původní profil NR | Podporovaná architektura RTX 50 / Blackwell a externí runtime | Dřívější test na RTX 5070 Ti |
| Profil NR ShortFuse | Povolené architektury řad RTX 20/30/40/50; komunitní experiment | RTX 4050; nejde o certifikaci všech karet |
| OPUS-MT + OCR | CPU; OPUS překládá EN → CS | Přibalené, lokální |
| TranslateGemma / CUNI | CUDA, Vulkan nebo CPU; dostatek dostupné paměti | RTX 4050 přes CUDA/Vulkan, Intel UHD přes Vulkan, CPU; AMD a větší Gemma zde nebyly testovány |

AMD Radeon a starší GeForce mohou použít přenositelné cesty frame genu, pokud hardware a ovladač splňují jejich požadavky. Žádná AMD ani starší GeForce nebyla při tomto vydání fyzicky změřena. Intel UHD je otestovaný, ale RIFE je na této konkrétní integrované grafice při 720p příliš pomalé.

NR je na AMD, Intelu, GTX a neznámých architekturách blokovaný v rozhraní, klávesových zkratkách, obnovených nastaveních i při inicializaci. FrameLab interpoluje hotový obraz; nejde o nativní integraci DLSS 5 do hry.

## Jak začít

Otevři **Frame gen → Výpočetní backend**. Na slabší grafice začni Optical Flow, pro první test RIFE zvol Low / 2×. RIFE ONNX modely a DirectML runtime jsou přibalené. Chybějící soubory nebo nepodporovaný hardware RIFE vypnou s vysvětlením; aplikace výpočet skrytě nepřepne na CPU.

Ukázku RIFE spustíš přes **Start FrameLab - RIFE.cmd**. Parametr `--adapter N` vybírá DXGI adaptér pro vykreslování a DirectML. Násobiče 3× a 4× vyžadují dva, respektive tři průchody modelem na dvojici snímků a mohou být výrazně pomalejší.

Volba Low mění výpočet pohybu, ne výstupní rozlišení. Při prvním načtení může model chvíli kompilovat. Nech načtení doběhnout, než nastavení znovu změníš.

Překladač má vlastní výběr zařízení v **Titulky → Local / offline → Výpočet překladu**. Viz [návod pro CUDA, Vulkan a CPU](local-translation-compute.cs.md). Vulkan pro překlad nezpřístupní NR enhancer na nepodporované grafice.

## Naměřené přehrávání 720p

Stejná ukázka GTA, vstup přibližně 30 FPS, 2×, fullscreen a VSync. V tomto izolovaném testu neběžel překlad měnícího se dialogu. Prezentace potvrzené Windows nejsou fyzické měření snímků na monitoru.

| GPU + backend | Prezentace potvrzené Windows za sekundu | Latence aplikace v ms |
|---|---:|---:|
| Intel UHD / Optical Medium | 59.3 | 58.0 |
| Intel UHD / RIFE Low | 13.5 | 162.8 |
| Intel UHD / RIFE Medium | 9.9 | 214.4 |
| RTX 4050 / Optical Medium | 59.5 | 54.6 |
| RTX 4050 / RIFE Low | 59.2 | 59.8 |
| RTX 4050 / RIFE Low + NR 360p | 59.2 | 72.3 |
| RTX 4050 / RIFE Medium | 49.5 | 67.9 |

Samostatný test se zadrženým prostředním snímkem použil 44 trojic × 6 konfigurací (264 výstupů) a rekonstruoval 15 → 30 FPS. RIFE Low dosáhlo RGB MAE 6,69 oproti 13,04 u Optical Medium na této jediné ukázce. Neznamená to „o 49 % hezčí obraz“ ani univerzální pořadí kvality.

[Skutečné porovnání pixelů a metodika](https://opicebot.cz/framelab/runs/2026-11-22-rife/?lang=cs).
[Implementace, opakovatelné příkazy a omezení](RIFE-DIRECTML.md).
[Oficiální požadavky DirectML](https://onnxruntime.ai/docs/execution-providers/DirectML-ExecutionProvider.html).

# Lokální vícejazyčné titulky / TranslateGemma

Preview.17 z 23. 11. 2026. Zachovává RIFE / DirectML, Optical Flow,
NVIDIA VFG, NR experiment i původní OPUS-MT a CUNI-MH. Zdrojem je text v obraze;
zvuk se nepřepisuje. Po stažení modelů probíhá OCR i překlad na tomto počítači.
Jazyk rozhraní aplikace zůstává čeština / angličtina.

## Jak to zapnout

1. Spusť `app/FrameLab.exe` z kompletní rozbalené složky.
2. V záložce **Titulky** vyber **Jazyk v obraze / OCR**. Musí odpovídat původním
   titulkům ve hře; nejde o automatickou detekci jazyka.
3. Vyber **Přeložit do jazyka**. Pro jiný pár než EN → CS se nabídne TranslateGemma.
4. Nech **RapidOCR / PP-OCRv5 multilingual**. V kompletním balíku jsou všechny
   uvedené OCR abecedy přibalené; nemusíš instalovat jazyk Windows.
5. Zvol překladový model. Pokud chybí, klikni **Stáhnout / pokračovat a ověřit**.
   Průběh ukazuje skutečné bajty a fázi ověření. Stažení lze pozastavit a obnovit.
6. Zapni **Překládat titulky**. První načtení může trvat desítky sekund; počkej
   na připravený model. Nedokončené stahování se nikdy nepoužije pro překlad.
7. Vyber čtecí oblast kolem původních titulků. Pokud se překládají i nápisy,
   SPZ nebo HUD, zmenši oblast, případně označ slovo jako vzor barvy/velikosti.
8. V umístění překryvu nastav polohu, velikost a pozadí. Původní text lze zobrazit
   společně s překladem. F11 zobrazí čistý výstup s aktivními efekty a titulky.

Ve veřejném ZIPu nejsou GPU překladové váhy ani GPU runtime. **4B Q4_K_M stáhneš
tlačítkem v aplikaci**; místní vývojový test už stažený model používal. Uložené nastavení a
staré profily se zachovávají. Při změně jazyka se vymaže starý překlad i jeho cache.
Změna jazyka u zapnuté TranslateGemma znovu načte příslušné procesy.

## Modely a paměť

| Volba | Stažení modelu | Orientační volná VRAM před načtením |
|---|---:|---:|
| TranslateGemma 4B Q4_K_M | 2,49 GB | 3,6 GiB + rezerva pro ostatní efekty |
| TranslateGemma 4B Q8_0 | 4,13 GB | 5,2 GiB + rezerva |
| TranslateGemma 12B Q4_K_M | 7,30 GB | 9,0 GiB + rezerva |

Jde o odhady, ne garantované nároky každého ovladače. Na 6GB RTX 4050 začni 4B Q4.
Q8 a 12B jsou dostupné tlačítkem, v tomto notebooku se jejich kvalita ani rychlost
neověřovala. Přesnější kvantizace sama nezaručuje lepší větu. Překladač ověřuje
úplné načtení na GPU; nepřepne se potichu na pomalý CPU offload. Dostupnost GPU
se řídí přibaleným runtime Ollama, nezávisle na GPU vybrané pro obraz v aplikaci.
U počítačů s více GPU proto údaj o volné paměti rendereru nemusí patřit GPU překladače.

Ollama je soukromý přenosný proces FrameLabu, poslouchá jen na `127.0.0.1` a má
zakázané cloudové funkce. Není potřeba instalovat službu nebo používat účet.
První GPU instalace, pokud runtime ještě chybí, stáhne navíc archiv 1,62 GB;
kvůli rozbalení nech přibližně 8 GB volného místa navíc. Modely leží v
`app/translation/models`. Pro přenos na další PC kopíruj celý balík, ne samotné EXE.

OPUS-MT a CUNI-MH zůstávají **pouze EN → CS**. Pro tento pár je lze dál používat.
TranslateGemma je rozšíření jazyků; netvrdíme, že její malá Q4 varianta překládá
do češtiny lépe než CUNI. V ověřovacích příkladech se objevily gramatické chyby.

## Jazyky a OCR

Katalog nabízí 40 explicitních jazykových variant: EN, CS, SK, DE, FR, ES, IT,
PT, PL, UK, RU, JA, KO, ZH-CN, ZH-TW, AR, EL, HI, TR, VI, NL, SV, NO, DA, FI,
HU, RO, BG, HR, SL, ET, LT, LV, ID, MS, TH, FA, UR, TA, TE.
Nejde o ověření všech 1 560 směrů překladu ani o úplný seznam možností modelu.
Japonské a čínské písmo sdílí OCR model; tradiční a zjednodušená čínština nejsou
garantovanou konverzí písma na výstupu překladače.

Používá se 12 rozpoznávacích modelů PP-OCRv5: anglický, latinka, východoslovanský,
cyrilice, čínský/japonský, korejský, řecký, arabský, dévanágarí, thajský,
tamilský a telugský. Přibalené OCR modely mají dohromady přibližně 114 MB.
OCR běží na CPU, aby nezabíralo VRAM frame genu a překladače.

Detekce textu stále používá původní PP-OCRv4. Stylizované písmo, více oddělených
segmentů, malé znaky a těsně zalomené řádky mohou způsobit chyby. Řádky se skládají
shora dolů podle velikosti znaků, arabské segmenty zprava doleva. Vertikální
japonské sazby nejsou předmětem této integrace. Rozšíření nepřidává STT.

Alternativní **Windows OCR** používá zvolený zdrojový jazyk a kontroluje jeho
dostupnost. Pokud ho Windows nemají, aplikace to vypíše. Různé verze Windows
nepodporují stejné jazykové OCR balíky. Jazyk se nikdy tiše nenahradí angličtinou.

Titulky se nyní vykreslují přes DirectWrite v rozlišení výstupu: systémové
náhradní fonty, spojování arabských znaků, pravolevý text a zalamování CJK.
Při chybějících glyfech se v nastavení objeví upozornění na doplnění systémových
fontů. Staré české atlasové písmo zůstává záložní cestou, například v testech
bez aktivního grafického zařízení. Názvy jazyků v nabídce jsou česky / anglicky.

## Vývoj a ověřování

- Sdílený katalog: `docs/subtitle-languages.json`; nativní hlavičku vytvoří
  `python tools/generate_subtitle_languages.py`.
- Verze nastavení titulků 9 přidává zdrojový a cílový kód, čte verze 1–8.
- `tools/prepare_translation.py` a `tools/prepare_ocr.py` přibalují katalogy,
  pomocné skripty a příslušné OCR modely. Váhy TranslateGemma se stahují přes
  existující správce modelů s pevně určenou revizí, délkou a SHA-256.
- `tests/subtitle_multilingual.py`: páry, hranice promptu, víceřádkový překlad,
  pořadí OCR segmentů a čísla v arabštině.
- `subtitle_multilang_test OUTPUT_DIR`: skutečné DirectWrite rasterizace,
  Unicode filtry, persistence a 44 obrazových příkladů v 22 jazycích.
- `tools/evaluate_multilingual_ocr.py --app APP --fixtures FIXTURES --output OUT`:
  OCR skutečných bitmap; ukládá očekávaný text, výsledek, CER a čas.
- `subtitle_language_switch_test APP FIXTURES`: skutečné OCR + GPU překlad,
  přepnutí DE→EN na DE→CS nad stejným vstupem a potom JA→EN, vyčištění a uvolnění.
- `tools/evaluate_translategemma.py --app APP --output OUT`: 56 krátkých pokusů
  v různých směrech. Ukládá přesné odpovědi, nikoli vymyšlené skóre kvality.

CLI příklad: `FrameLab.exe --rife --fg --low --video samples/ocr-demo.mp4 --subtitles
--subtitle-model translategemma-4b-q4km --source-language en --target-language de`.
Diagnostická data nově uvádějí oba jazykové kódy; starý klíč `cs` se ve výpisu
titulků zachovává jen pro skutečný český cíl. Načtení modelu znovu nabídne i
nezměněný titulek, který během dlouhého zahřívání propadl časovému limitu.

## Původ a licence

Gemma is provided under and subject to the Gemma Terms of Use found at
https://ai.google.dev/gemma/terms.

TranslateGemma od Google: https://huggingface.co/google/translategemma-4b-it .
Textové GGUF od mradermacher (4B) a bullerwins (12B) nejsou oficiální kvantizace
Google; neobsahují obrazový projektor. Používáme oddělené OCR a textový překlad.
Přesné revize, hashe a odkazy jsou v `subtitle-models.json`. Úplná licence je
v `docs/licenses/Gemma-LICENSE.txt`; další omezení používání jsou součástí podmínek.
PP-OCR / RapidOCR: Apache-2.0; modely jsou nezměněné a mají zapsané SHA-256.
Úprava pořadí arabského OCR odpovídá dekódování PaddleOCR, rovněž Apache-2.0.
Externí NR DLL se veřejně nepřibalují ani nezrcadlí.

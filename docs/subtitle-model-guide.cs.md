# Modely titulků, doba čtení a umístění

[English guide](subtitle-model-guide.md)

FrameLab čte anglické titulky viditelné v obrazu střihovky nebo místního videa, překládá je lokálně a vykresluje češtinu po FG a NR. Nepřepisuje zvuk ani nevykresluje SRT či vložené titulkové stopy.

## Začni přibaleným OCR

1. Otevři **Titulky** a vyber **RapidOCR / PP-OCRv5 English**. Kompletní ZIP obsahuje runtime, detektor i rozpoznávací model. Není potřeba jazykový doplněk Windows, správce ani další stažení.
2. Vyber přibalený **OPUS-MT / CPU**, zapni překlad a **Zobrazit oblast čtení**. Vymez jen pruh anglických titulků bez HUDu a chatu. Nastavení zkontroluj na umělém testu titulků v kartě Vstup.
3. Český překryv umísti samostatně. F11 ukáže výsledný obraz bez úchytů. Místní video počká na načtení modelu; Přehrát tuto pauzu zruší. Externí vstup zastavit nelze.

RapidOCR používá dvě CPU vlákna, detektor PP-OCRv4 a anglický rozpoznávač PP-OCRv5. **Windows OCR** zůstává lehčí volitelnou zálohou. Jen tento režim potřebuje anglickou OCR součást Windows. Pokud chybí, **Doplnit anglické OCR** nabídne stažení od Microsoftu s potvrzením správce. Jazyk Windows se nemění.

## Vyber překladový model

| Model | Stažení | Stav |
| --- | ---: | --- |
| OPUS-MT / CPU | Přibalený | Dvě CPU vlákna, bez GPU downloadu. |
| CUNI-MH 7B Q3_K_M | 3,52 GB | Otestováno na stolní RTX 5070 Ti. |
| CUNI-MH 7B Q5_K_M | 5,13 GB | Ověřeno stažení, hash a místní GPU načtení. Kvalita a rychlost bez benchmarku. |
| CUNI-MH 7B Q8_0 | 7,70 GB | Otestováno; větší spotřeba paměti. |

Vyber GPU model a stiskni **Stáhnout / pokračovat a ověřit**. Poprvé se stáhne i přenosný runtime Ollama (1,62 GB); nech přibližně 8 GB navíc na jeho uložení a rozbalení. Ručně neinstaluješ Ollamu, nepotřebuješ účet ani API klíč. Soubory musí projít kontrolou připnutého SHA-256.

Modely zůstávají v `app/translation/models`, runtime v `app/translation/gpu-runtime`. NTFS používá pro cache pevný odkaz; jiný souborový systém může potřebovat další kopii modelu. Přenášej celou složku aplikace. GPU váhy nejsou v release ZIPu. [Přesný katalog a zdroje](subtitle-models.json).

Volná VRAM se kontroluje při současně běžících efektech. Odhady Q3 4,4 GiB, Q5 6,0 GiB a Q8 8,5 GiB plus rezerva nejsou záruka pro konkrétní kartu. Částečný přesun na CPU se odmítne. Při nedostatku paměti sniž efekty nebo vyber CPU. Větší přesnost vah nezaručuje lepší větu.

Dosavadní test 300 vět na **základu preview.12** změřil medián Q3 212 ms a Q8 360 ms při VFG 2×, NR 65 % / 720p a původním OCR. Jde o čas přesného textového požadavku, ne celkové zpoždění titulků ani benchmark preview.14/Q5. Obě varianty měly v textovém předtestu 271/300 přijatelných překladů; hodnocení nebylo zaslepenou lidskou studií. [Všechna měření a překlady](https://opicebot.cz/framelab/translation/?lang=cs).

## Umísti český překryv

- **Vlastní poloha:** nastav kotvy nebo zapni **Přetahovat český překryv v náhledu**. Text i pozadí se pohybují společně, nezávisle na OCR oblasti.
- **Nad originál / Pod originál:** navazuje na rámeček OCR a zvolený odstup.
- **Při nedostatku místa:** přepni na opačnou stranu nebo omez překryv na viditelný obraz. Může zakrýt originál; příliš vysoký text se zmenší.

Více řádků zůstává uvnitř viditelné plochy při 4K, obrazu na výšku, fullscreenu, ořezu i 1:1. Velikost písma, krytí pozadí a **Zakrytí původní oblasti titulků** jsou samostatné. Fullscreen skryje OCR rámeček a úchyty; Escape/F11 vrátí nastavení.

## Časování a rozdělené věty

**Nejméně na obrazovce** má výchozí hodnotu 1,2 s, **Ponechat při čekání** 3 s. Čas běží až od zobrazení češtiny, i když překlad přišel pozdě. Dlouhý text dostane až 7,5 s; další hotový titulek ho může nahradit po minimu. OCR a překlad běží odděleně. Prázdný vstup už zobrazený titulek okamžitě nemaže. Pauza místního videa jej ponechá.

**Spojovat rozdělené věty** je výchozí zapnuté. Neúplná věta dostane pro překladač výpustku. Navazující část začínající malým písmenem do šesti sekund se může znovu přeložit společně s ní. Dokončená věta, nový popisek mluvčího, změna zdroje a přetáčení spojení blokují. Pokud se chybně spojí různé repliky, volbu vypni; označení neúplného vstupu výpustkou zůstane. Oprava odstranila pozorované domyšlené pokračování Q3 v místním testu, nezaručuje bezchybný překlad všech vět.

Fronta má nejvýše tři čekající texty a tři hotové titulky. Práce opožděná přes šest sekund se zahodí; rychlý dialog se stále může ztratit. Změny velikosti písmen, mezer a běžné interpunkce nespouštějí opakovaný překlad. Přetáčení, změna zdroje/oblasti/modelu nebo vypnutí starou práci vyčistí. Vypnutí uvolní proces modelu. [Srovnání OCR a omezení](subtitle-ocr.md).

## Offline provoz a původ komponent

CPU používá soukromé roury; CUNI spouští vlastní proces Ollama pouze na `127.0.0.1`, bez cloudu, s jedním modelem a požadavkem. Stahování neposílá obraz ani text titulků. Po přípravě vše funguje offline. Volitelné `--subtitle-trace PATH.jsonl` zapisuje soukromý diagnostický text; výchozí je vypnuté. RAW/FG záznamy neobsahují český překryv, NR ani zvuk.

OPUS-MT en-cs opus-2019-12-18 má licenci CC BY 4.0 a je převedený na CTranslate2 int8 bez přetrénování. CUNI-MH je Mistral 7B doladěný týmem CUNI pro EN→CS; původní karta uvádí MIT. GGUF kvantizace poskytuje Richard Erkhov. Katalog zachovává zdroje a hashe. CUNI používá původní šablonu Instruction / Glossary / Previous text / Input / Response, prázdnou historii a slovník, teplotu 0, ukončení novým řádkem, kontext 2048 a limit 256 tokenů. Spojené části jdou společně do Input.

Vývojář připraví komponenty pomocí `tools/prepare_translation.py` a `tools/prepare_ocr.py`; uživatel hotové aplikace nic z toho nespouští. Viz [sestavení](build-preview14.md), [OCR komponenty](ocr-components.json) a [překladové komponenty](translation-components.json). Testy pokrývají nastavení, okraje obrazu, hranice vět, skutečné neuronové OCR/CPU překlad, opožděné titulky a reset zdroje. Q5 bylo po ověření zapnuté na počítači vlastníka; řízený benchmark kvality ani rychlosti neproběhl.

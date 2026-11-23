# Lokální experiment: okno hry, výběr GPU a filtr titulků

Funkce jsou součástí veřejného preview.18. Níže uvedené vývojové testy pocházejí z 23. listopadu 2026.

## Hra nebo jiná aplikace na stejném PC

1. Spusť hru v okně nebo v bezokrajovém okně. Okno nech obnovené, neminimalizované.
2. Ve FrameLabu otevři **Vstup → Okno PC hry/aplikace**.
3. Obnov seznam, vyber konkrétní okno a spusť zachytávání. Nejprve zkontroluj náhled.
4. Nastav frame gen, enhancer a titulky stejně jako pro kartu nebo ukázkové video.
5. Stiskni **Zobrazit výsledek přes hru**. Přes klientskou část hry se zobrazí
   jediný výsledný obraz. Porovnávání, panely, obrysy OCR a diagnostika se skryjí.
   Klikání a klávesnice dál patří původní hře.
6. **Ctrl+Alt+F11** vrátí nastavení FrameLabu. Nezapomeň tuto zkratku před hraním.
   Aktivace FrameLabu z hlavního panelu nebo přes Alt+Tab rovněž opouští překryv.
   Pokud zkratku používá jiný program, FrameLab překryv raději nespustí.

Při přepnutí do jiné aplikace se překryv schová; při návratu ke hře se vrátí.
Zvuk hraje přímo původní hra. FrameLab ho v tomto režimu znovu nepřehrává.
Pohyb a změna velikosti okna se sledují automaticky. Po zavření nebo restartu
zdrojové hry vyber nové okno znovu. FrameLab sám sebe nenabízí jako zdroj.

**F11** je samostatné zobrazení výsledku na celou obrazovku. Překryv přes ovládanou
hru se zapíná výše uvedeným tlačítkem; F11 nenahrazuje předávání vstupu hře.
V běžném F11 režimu se kurzor po chvíli skryje. V překryvu kurzor vlastní hra.

Používá se Windows Graphics Capture (Windows 10 1903 nebo novější), bez injekce
do hry. Minimalizované, chráněné nebo některé exkluzivní fullscreen aplikace
nemusí poskytovat obraz. Není zaručena kompatibilita s každou hrou/anti-cheatem.
Současná cesta kopíruje snímek přes CPU do stávající pipeline; přidává režii a
latenci. Jde o experiment, nikoli slib nízké latence pro kompetitivní hraní.

## Která grafika počítá obraz

Otevři **Zobrazení → GPU pro zpracování obrazu**, vyber adaptér a stiskni
**Uložit GPU a restartovat FrameLab**. Po restartu zkontroluj název skutečně
použité grafiky vpravo nahoře. Okno PC hry musíš vybrat znovu.

Relace a GPU se ukládají i v případě, že Windows dočasně blokuje nahrazení
staršího souboru. Soubory `runtime-session.txt.tmp` a `gpu-settings.txt.tmp`
proto mohou obsahovat nejnovější dokončené nastavení; nemaž je při běžném úklidu.
Neúplný zápis se při načítání odmítne a použije se předchozí dokončená kopie.

Volba platí pro zpracování obrazu. Překladové modely používají vlastní runtime
a jeho výběr GPU. Není to přepínač, který přesune NVIDIA algoritmus na Intel/AMD.
Nepodporované NVIDIA efekty jsou na jiných adaptérech nedostupné; lze vybrat
kompatibilní Optical Flow/RIFE cestu. Zachytávání okna bylo zde prakticky
ověřeno na Intelu a RTX 4050 Laptop; fyzické AMD v této sestavě není.

## Displej během přehrávání

V **Zobrazení** je volba udržování displeje:

- řízení ponechat Windows;
- udržovat displej při přehrávání (výchozí);
- udržovat displej při viditelné aplikaci.

Po zastavení, minimalizaci nebo ukončení se požadavek uvolní. Nemění se napájecí
plán, neposouvá se uměle myš. Ruční uspání, zavření víka a bezpečnostní zamykání
Windows se neobcházejí. Testy ověřují registraci a uvolnění požadavku Windows;
nejde o dlouhodobý test každého firemního spořiče nebo politiky zamykání.

## Filtr titulků

V **Titulky → Filtr textu** lze vybrat vypnuto, základní nebo chytrý filtr.
Chytrý filtr je výchozí. Funguje lokálně a pracuje s původním OCR textem ještě
před tím, než mu spojování vět doplní interpunkci.

1. Jednoduchá pravidla odmítnou rozpoznatelné SPZ/kódy a jasný OCR šum.
2. Skutečné věty, oslovení s interpunkcí a běžné krátké povely se chrání.
3. Malý anglický DistilBERT model na CPU odmítá jen velmi jistá samostatná
   jména/osoby/místa/organizace, která pokrývají celý text. Model má asi 66 MB.
4. TranslateGemma může posoudit zbývající text připomínající štítek. Používá se
   už načtený překladový model. Neplatná odpověď nebo chyba text nezahodí.
5. CUNI-MH se jako klasifikátor nepoužívá: v měření nedodržoval instrukce KEEP/SKIP.
   Pomáhá mu model jmen a po překladu kontrola nezměněného samostatného názvu.

Pod nastavením je vidět, zda se model jmen skutečně načetl. Pokud chybí, aplikace
pokračuje s konzervativní náhradou; nezastaví kvůli tomu překlady. U jiných
zdrojových jazyků zůstávají základní pravidla. Anglický NER ani anglické výsledky
testu nelze vydávat za ověření filtru pro všechny jazyky.

**Text sám nemůže vždy rozlišit jméno, nápis a promluvu.** „Rose“, „May“ nebo
samostatné číslo může být skutečný dialog. Raději takové případy necháváme projít.
Názvy malými písmeny, smyšlená jména a štítky zakončené interpunkcí mohou uniknout.
Nejdůležitější proto dál zůstává správně zvolená oblast OCR a styl titulků.
Nežádoucí filtr lze kdykoli vypnout. Modely během přehrávání nic neposílají ven.

## Příprava a opakování měření

Vývojář spustí `python tools/prepare_entity_filter.py --app C:/cesta/k/app`.
Skript stáhne přesné připnuté modelové soubory a tokenizer pro Windows x64,
ověří hash a přibalí licence. Používá již připravený ONNX Runtime z `app/ocr/deps`.
K vydání jej přidej pomocí `package_release.py --entity-filter .../translation/filter`.
Uživatel kompletního připraveného balíku nic z toho spouštět nemusí.

Textovou sadu a všechna rozhodnutí zachovává `evaluate_integrated_filter.py`:

```powershell
app/translation/python/python.exe tools/evaluate_integrated_filter.py `
  --app app --dataset final589.jsonl --output vysledky --entities
```

Jde o měření **filtrace**, nikoli automatickou známku kvality českého překladu.
Pro měření souběhu musí během celého běhu skutečně běžet také FrameLab se
stejným videem, FG a NR; jeho CSV log zachycuje výkon. Hardware test
`pc_capture_test.exe` kontroluje zachytávání, ořez, změny velikosti a uvolnění
požadavku na displej. `subtitle_filter_pipeline_test.exe APP FIXTURES MODEL_ID`
ověřuje také skutečné OCR, komunikaci s pracovníkem a frontu titulků.

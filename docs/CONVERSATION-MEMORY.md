# Conversation memory / Paměť rozhovoru

The experiment and every measured output are documented at
[Context Lab](https://opicebot.cz/framelab/runs/2026-11-24-context-memory/?lang=en).
Česká verze reportu je [zde](https://opicebot.cz/framelab/runs/2026-11-24-context-memory/?lang=cs).

See also [live validation / ověření v aplikaci](CONTEXT-LIVE-VALIDATION.md) for the subsequent same-model evidence-selection prototype and its real OCR/queue/render tests.

## Application setting / Nastavení aplikace

In the development build, **Subtitles → Conversation memory → Use recent dialogue** is a shared, opt-in setting. The same admission,
timing and reset policy serves local CUNI/TranslateGemma, OpenAI and the optional
custom-server adapter. Provider-specific prompts differ, but the source-history
selection does not. OPUS-MT cannot use this feature. Keep it off for CUNI unless
you deliberately want to experiment: the tested Q3 model often copied old lines.

Live memory only admits source captions whose translation survived the content
filter. It never remembers the translated answer as a source fact. The diagnostic
above also replays raw/rejected OCR to stress the model with extra clutter; that
is a harder input condition than the application buffer. Settings or source
changes clear memory, as does turning translation off. No transcript is written
to disk by this feature. Existing explicitly enabled debug logging is separate.
When history is on, cached translations are bypassed: repeated words can mean
something different in a new conversation. Large current captions fall back to
current-only translation instead of being silently truncated. History can raise
latency and API input-token charges. The quality experiment covers EN → CS;
other language pairs use the same policy but have not received this context test.

Ve vývojovém sestavení je v **Titulky → Paměť rozhovoru → Použít poslední repliky rozhovoru** společné, výchozím stavem vypnuté nastavení. Stejná
pravidla výběru, časování a mazání používají místní CUNI/TranslateGemma, OpenAI
i volitelný adaptér vlastního serveru. Liší se formát promptu pro konkrétní model,
nikoli výběr historie. OPUS-MT tuto funkci neumí. U CUNI doporučujeme vypnout:
testované Q3 často kopírovalo předchozí repliky.

Živá paměť přijímá jen originální titulky, jejichž překlad prošel filtrem obsahu.
Z české odpovědi nevyrábí zdrojová fakta. Výzkumný test navíc přehrává syrová
a odmítnutá OCR pozorování pro zátěž balastem; to je obtížnější vstup než aplikační
buffer. Změna zdroje či překladového nastavení nebo vypnutí překladu paměť maže.
Funkce sama nepíše přepis na disk; výslovně zapnuté diagnostické logování je zvlášť.
Při zapnuté historii se nepoužívá překladová cache: opakovaná slova mohou mít
v novém rozhovoru jiný význam. Velký aktuální titulek se přeloží bez historie,
místo aby se tiše ořízl. Historie může zvýšit prodlevu i cenu vstupních API tokenů.
Hodnocení kontextu proběhlo EN → CS; ostatní páry mají stejná pravidla, ale tento
kontextový test na nich provedený nebyl.

## What is actually being remembered?

The prototype keeps source OCR observations, their times and any explicitly
supplied speaker label. It does not learn facts from its own translations.
An absent speaker label remains unknown. Nearby timestamps are evidence of
proximity, not evidence that two people are speaking to one another.

`translation_timeline.py` joins adjacent repeated/growing caption observations.
A repeated sentence after an intervening turn remains a separate turn. A pause
over ten seconds creates a candidate temporal segment. A reset clears the
transcript. The prompt receives at most six source turns from the preceding
60 seconds, bounded by 1,000 source characters. The in-memory transcript has a
128-record cap; the model does not receive an entire gaming session.

`translation_memory.py` also implements the older comparison arms: the last six
raw observations and a 35-second bounded, deduplicated buffer. The admission
filter rejects a few obvious HUD/code patterns, but deliberately keeps ambiguous
short speech and unrelated banter. It is not an oracle dialogue classifier.

Character profiles in the controlled examples are supplied simulator facts.
They are **not** identities extracted from the game. Contradictory gender facts
become unknown. This experiment does not establish reliable automatic speaker
identification, gender inference or long-term character biographies.

## Co si paměť skutečně uchovává?

Prototyp ukládá zdrojová OCR pozorování, jejich časy a případné výslovné označení
mluvčího. Nevyrábí fakta z vlastních překladů. Chybějící mluvčí zůstává neznámý.
Blízké časy potvrzují časovou blízkost, nikoli společné téma nebo rozhovor.

`translation_timeline.py` spojuje sousední opakované či prodlužované titulky.
Opakovaná věta po jiné replice zůstává samostatnou replikou. Pauza přes deset
sekund vytvoří kandidátní časový úsek. Reset vymaže přepis. Model dostane nejvýše
šest replik z předchozích 60 sekund, s limitem 1 000 znaků zdrojového textu.
Přepis v paměti má strop 128 záznamů; celé hraní se do promptu neposílá.

`translation_memory.py` obsahuje i původní srovnávací varianty: posledních šest
syrových pozorování a omezenou paměť s deduplikací a stářím 35 sekund. Jednoduchý
filtr vyřazuje část zjevných HUD nápisů a kódů. Nejisté krátké repliky a nesouvisející
hlášky ale záměrně ponechává. Není to spolehlivý detektor dialogu.

Profily postav v řízených scénách jsou předem zadaná fakta simulátoru. **Nejde**
o identity získané z herního záznamu. Rozpor v rodu vytvoří neznámou hodnotu.
Tento test nedokazuje spolehlivé automatické rozpoznávání mluvčích, odvozování
rodů ani dlouhodobé životopisy postav.

## Dataset and evaluation / Soubor a hodnocení

- Original public session: 32,844 OCR completions, 15,190 style-line observations,
  nine reset events and 1,747 translation jobs. The game identification was
  supplied by the owner; it is not established by the numeric/text log.
- 64 target jobs in 16 chronological four-job windows, plus 24 authored stress
  cases. Earlier OCR observations are replayed causally, including plates, signs,
  misread words, partial captions, repetitions and filler. Previous Czech outputs
  and old quality labels never enter memory.
- Six separate pilot cases. The timestamped strategy is an exploratory follow-up
  on the same target set, not an untouched second evaluation set. Its changes
  include text selection/stitching as well as timestamps.
- Four strategies × 88 targets × four models = 1,408 main outputs. Pilots and the
  twelve additional CUNI profile-only controls are separate.
- Direct, unblinded Codex review. No API judge and no local judge model. Five
  corrupted/already-Czech game targets are excluded from the 0–4 quality score,
  but their outputs remain available. Unknown game-speaker gender is not graded.
  Known gender (12 authored cases) and lexical features (six cases) are reported
  separately. These are diagnostic scores, not human-validated accuracy.

- Původní veřejná relace má 32 844 dokončených OCR úloh, 15 190 pozorování řádků,
  devět resetů a 1 747 překladových úloh. Určení hry pochází od vlastníka;
  samotný textový a číselný záznam ho nepotvrzuje.
- Cílový výběr tvoří 64 úloh v 16 úsecích po čtyřech a 24 vytvořených zátěžových
  scén. Paměť vzniká z časově předcházejícího OCR včetně SPZ, nápisů, chyb,
  fragmentů, opakování a filleru. Dřívější české výstupy ani známky se do ní nedávají.
- Pilot má šest oddělených případů. Časovaná varianta je navazující průzkum na
  stejných cílových větách, ne nový nezávislý test. Mění i výběr a spojování textu.
- Čtyři varianty × 88 cílů × čtyři modely = 1 408 hlavních výstupů. Piloty a dvanáct
  doplňkových zkoušek CUNI pouze s profilem se uvádějí zvlášť.
- Přímé, neslepé posouzení Codexem; žádný API ani lokální hodnoticí model. Pět
  nečitelných nebo již českých herních vstupů je mimo skóre 0–4, ale výstupy zůstaly.
  Neznámý rod herního mluvčího se neboduje. Známý rod (12 scén) a konkrétní jméno
  či význam slova (šest scén) mají samostatné výsledky. Nejde o přesnost ověřenou lidmi.

## Reproduction / Opakování

Use Python 3.11+, a complete FrameLab translation runtime and already verified
model downloads. Close other local inference jobs for comparable GPU timings.
Use a new output directory for every run; the benchmark refuses to overwrite
existing evidence. The API test is optional and makes billable translation calls.
It never requests an assessment of translation quality.

Použij Python 3.11+, kompletní překladový runtime FrameLabu a již ověřené modely.
Pro srovnatelnější časy ukonči jiné lokální inference. Každému běhu dej novou
výstupní složku; existující důkazy skript nepřepisuje. API test je volitelný
a platí se pouze překlady. Hodnocení kvality přes API neprovádí.

From the repository root / Z kořene repozitáře:

```powershell
# Reuse the frozen published dataset / Použij zmrazený zveřejněný soubor.
python tools/benchmark_translation_memory.py `
  --dataset website/runs/2026-11-24-context-memory/data/dataset-timeline-v1.json `
  --runtime C:/FrameLab/app/translation `
  --output C:/FrameLab-tests/new-memory-cuni `
  --model cuni-q3km --split heldout --modes baseline naive guarded timeline

# Optional paid translation. Keep the key outside the repository/package.
# Volitelný placený překlad. Soubor klíče patří mimo repozitář a balíček.
python tools/benchmark_translation_memory.py `
  --dataset website/runs/2026-11-24-context-memory/data/dataset-timeline-v1.json `
  --output C:/FrameLab-tests/new-memory-luna `
  --model gpt-6-luna --key-file C:/private/openai-key.txt `
  --api-cap-usd 0.20 --split heldout --modes baseline naive guarded timeline
```

Run API experiments serially: the budget check includes completed sibling runs
in the same output parent folder and conservatively reserves unknown timeout
costs. Do not copy the old ratings to newly generated outputs. Frozen historical
prompts are stored alongside the raw outputs; current application code may evolve.

API experimenty spouštěj postupně: rozpočtová kontrola zahrnuje dokončené sousední
běhy ve stejné nadřazené složce a rezervuje neznámé náklady po timeoutu.
Na nově vygenerované překlady nekopíruj staré známky. Přesné historické prompty jsou
uložené u výstupů; současný aplikační kód se může dále měnit.

To rebuild source snapshots, download the original public
[debug archive](https://opicebot.cz/framelab/runs/2026-11-20-3h/data/raw-debug-1795196244267574.zip),
then / Pro přepočet vstupní paměti stáhni původní veřejný archiv a spusť:

```powershell
python tools/extract_memory_stream.py raw-debug-1795196244267574.zip fresh/ocr-stream.jsonl
python tools/build_memory_dataset.py --stream fresh/ocr-stream.jsonl --output fresh/dataset.json
python tools/translation_timeline.py --dataset fresh/dataset.json --stream fresh/ocr-stream.jsonl --output fresh/timeline.json
python -m unittest discover -s tools -p test_translation_memory.py
```

The extractor verifies every event-file SHA-256 against the original manifest.
It reads ZIP members without extracting arbitrary filesystem paths. The main
test measures text-request time, not live caption delay, game FPS or OCR accuracy.
Frame generation and enhancement were off. Model startup and server-reported
compute times are separate from full request times.

Extraktor kontroluje SHA-256 všech souborů událostí podle původního manifestu.
Čte členy ZIP bez rozbalování libovolných cest. Hlavní test měří čas textového
požadavku, nikoli živé zpoždění titulků, FPS hry nebo přesnost OCR. Frame gen
a enhancer byly vypnuté. Načtení modelu a serverový výpočet se oddělují od času
celého požadavku.

## Model-specific limitations / Omezení modelů

[CUNI's paper, Appendix A](https://aclanthology.org/2024.wmt-1.16.pdf) says that
its glossary/history fields were left empty during training. Their presence in
the template does not establish trained support for conversation memory. The
measured context-copying failures are therefore a deployment concern, not a
reason to silently enable memory for every model.

[Článek autorů CUNI, příloha A](https://aclanthology.org/2024.wmt-1.16.pdf) uvádí,
že slovník a předchozí text nechávali při trénování prázdné. Samotná pole v šabloně
neznamenají naučenou práci s pamětí rozhovoru. Naměřené přenášení starých vět je
důvod pro opatrné doporučení, nikoli pro automatické zapnutí všem modelům.

The external Gemma is a separate user-operated server model, not a bundled
desktop requirement. Its server reserves about 22 GB of RTX 3090 VRAM with a
214,272-token context configuration. This experiment sends only a small source
excerpt and does not change the server's allocation, launch settings or model.

Externí Gemma je samostatný model na serveru uživatele, nikoli součást požadavků
desktopového balíčku. Server s kontextem 214 272 tokenů při kontrole zabíral asi
22 GB VRAM RTX 3090. Experiment posílá jen krátký výřez zdroje a nemění přidělenou
paměť, spouštěcí nastavení ani model serveru.

# Context memory: live validation / Ověření v běžící aplikaci

The follow-up **75-scene actual-app study is complete**: 300 runs and 900 original framebuffer proofs. [Interactive CS/EN results](https://opicebot.cz/framelab/runs/2026-11-24-context75/) · [Protocol](CONTEXT75-PROTOCOL.md). The earlier pilot below remains preserved.

Navazující **test 75 scén v aplikaci je dokončený**: 300 běhů a 900 původních obrazových důkazů. Původní pilot níže zůstává zachovaný.

## Scope / Rozsah

**Development experiment, default OFF. This is not a released quality guarantee.**
The follow-up 25 Czech and 50 multilingual examples use the actual FrameLab
pipeline. Standalone prompt triples remain preliminary diagnostics, not substitutes
for those application demonstrations.

**Vývojový experiment, výchozí stav VYPNUTO. Nejde o záruku kvality vydané verze.**
Navazujících 25 českých a 50 vícejazyčných ukázek používá skutečnou cestu aplikací.
Samostatné trojice promptů zůstávají pouze předběžnou diagnostikou.

## What the candidate does / Co zkoušená metoda dělá

1. OCR reads the original picture. Only accepted source captions enter memory.
2. The selected translator itself chooses a small set of useful original lines.
   No other local or remote model summarizes for it.
3. Each generated fact must cite existing observation IDs. This checks source
   membership, **not semantic truth**. Generated claims never enter the translation
   prompt; the cited original lines and the two latest accepted lines do.
4. Recent candidates: 16 lines, 120 seconds, 2,400 source characters. Retained
   evidence: eight lines, 900 characters, ten minutes. Combined selection input
   stays within 2,400 characters. No previous translated answers are reused.
5. First update after two accepted captions, then at most one per eight source
   seconds with two new captions. The current v4 candidate starts one background update immediately after accepting a caption.
   The current caption does not wait for that update; initially it uses recent
   history. A local model or serial server can still suffer inference contention.
6. Source generation changes, backwards time, or a gap over 60 seconds clear
   memory. A late background result from an old generation is discarded. Invalid
   responses stop memory generation until reset, with recent-history fallback.

1. OCR přečte původní obraz. Do paměti vstupují jen přijaté zdrojové titulky.
2. Tentýž vybraný překladač si sám vybírá užitečné původní repliky. Žádný jiný model
   mu shrnutí nevytváří.
3. Vytvořená fakta musejí odkazovat na existující pozorování. Kontrola ověří ID,
   **nikoli pravdivost tvrzení**. Překladač dostane původní citované věty a poslední
   dvě přijaté repliky, nikoli vygenerovaná tvrzení.
4. Kandidáti: 16 replik, 120 sekund, 2 400 znaků. Uchované důkazy: osm replik,
   900 znaků, deset minut. Společný vstup výběru má nejvýše 2 400 znaků. Vlastní
   přeložené odpovědi se do zdrojového kontextu nevracejí.
5. První aktualizace po dvou replikách, další nejvýše jednou za osm sekund při dvou
   nových replikách. Současný kandidát v4 spouští aktualizaci na pozadí hned po přijetí repliky; aktuální titulek na ni
   nečeká a zpočátku používá poslední repliky. Místní model nebo server zpracovávající
   jen jeden požadavek současně může přesto zpomalit oba výpočty.
6. Změna zdroje, návrat času nebo pauza přes 60 sekund paměť smaže. Opožděný výsledek
   ze starého zdroje se zahodí. Chyba generování vypne další aktualizace do resetu;
   zůstává krátká historie původních vět.

## Why this candidate / Proč tento kandidát

Direct review covered the original 1,408 translations, separate summary pilots,
rolling and long-memory stress cases, 40 disjoint validation captions and 24
additional confirmation captions. The last two sets were reviewed with anonymous,
deduplicated output labels before revealing model/method identities. No paid
model judge was used. These are small, clustered diagnostics, not population
accuracy or proof that memory always helps.

Plain summaries sometimes preserved the right fact while translation ignored it.
Passing selected original dialogue through the normal history field retained
some ambiguous meanings across filler. It also produced regressions. On the
24-caption confirmation set, selected evidence scored 97.92 versus 96.88 for
Luna without context, and 82.29 versus 80.21 for server Gemma. Ordinary six-line
history scored 97.92 and 83.33 respectively. Thus selected memory is **not an
overall winner** on ordinary dialogue; its specific purpose is retention across
intervening chatter. CUNI failed the summary task and cannot enable this builder.

Přímé hodnocení zahrnulo původních 1 408 překladů, samostatné pokusy se shrnutím,
průběžnou pamětí a dlouhou odbočkou, 40 oddělených ověřovacích replik a dalších
24 potvrzovacích. Poslední dvě sady byly hodnocené pod anonymními označeními před
odhalením modelu a metody. Žádný placený model nebyl hodnotitelem. Jde o malé
diagnostické sady s navazujícími větami, nikoli obecnou přesnost nebo důkaz, že
paměť vždy pomáhá.

Shrnutí někdy obsahovalo správný fakt, který překladač ignoroval. Předání vybraných
původních vět jako běžné historie pomohlo některým dvojznačnostem přežít odbočky,
ale způsobilo i regrese. Na 24 potvrzovacích replikách mělo 97,92 proti 96,88 bez
kontextu u Luny a 82,29 proti 80,21 u serverové Gemmy. Běžná historie šesti vět
dosáhla 97,92 a 83,33. Nejde tedy o **celkového vítěze** běžného dialogu; konkrétní
přínos hledáme v uchování významu přes jiné hlášky. CUNI nezvládlo vytváření
shrnutí, proto tento výběr zapnout nemůže.

## Actual app experiment / Zkouška skutečné aplikace

Two authored 86-second scenes burn English dialogue into gameplay pixels.
Each contains an opening situation, the identical ambiguous line, twenty filler
lines including a plate and HUD label, the repeated ambiguous line, an explicit
topic reversal and the line again. One starts in a garden, the other in a bar.
The background is a repeated existing GTA clip; this is a controlled simulation,
not a claim that the authored dialogue came from that game.

The real executable decodes the video, runs RapidOCR, filtering, translation,
memory and subtitle rendering with NVIDIA VFG 2× and NR enabled on the RTX 4050
laptop. Compare OFF, recent history and selected evidence. Store native OCR/job/
display logs, explicit memory traces, unmodified outputs, dropped work, timing,
FPS and actual screenshots. A model answer that never reaches the screen is not
a successful subtitle. Translation caching in OFF mode is part of the real
system and must be reported, including wrong reuse after a topic change.

Dvě autorské 86sekundové scény mají anglický dialog vypálený přímo do herního
obrazu. Obsahují úvod, totožnou dvojznačnou větu, dvacet odboček včetně SPZ a
nápisu, opakování věty, výslovnou změnu tématu a další opakování. Jedna začíná na
zahradě, druhá v baru. Pozadí tvoří opakovaná stávající ukázka GTA; jde o řízenou
simulaci, nikoli tvrzení, že autorský dialog pochází z této hry.

Skutečný program dekóduje video a provádí RapidOCR, filtr, překlad, paměť i kreslení
titulků se zapnutým VFG 2× a NR na notebooku RTX 4050. Porovnávají se vypnutý
kontext, poslední repliky a vybrané důkazy. Ukládají se skutečné OCR/job/display
logy, explicitní záznam paměti, nezměněné odpovědi, zahozená práce, prodlevy, FPS
a screenshoty. Odpověď modelu, která se na obrazovku nedostala, není úspěšný titulek.
Cache ve vypnutém režimu je součást skutečného systému a musí být popsaná včetně
chybného opakování po změně tématu.

The nineteen completed runs include the first synchronous variant, which lost the
delayed target in the translation queue. Version 2 updates memory in the background.
Luna then displayed all six target occurrences in the two selected-memory runs
with the intended sense; one had a minor imperative shift. Server Gemma still
failed the bar-to-garden correction. This supports further testing of the candidate,
not a general recommendation or a binary release. Sequential API runs can
have different network latency; repeated runs are required for causal performance
claims. Paid usage includes both translation and memory calls, never judging.

Devatenáct dokončených běhů zahrnuje první synchronní variantu, která ztratila
opožděnou klíčovou repliku ve frontě překladače. Druhá aktualizuje paměť na pozadí.
Luna pak ve dvou bězích s vybranou pamětí zobrazila všech šest cílových výskytů
se správným významem; jeden měl drobný posun příkazu. Serverová Gemma stále
nezvládla změnu z baru na zahradu. To podporuje další zkoušení kandidáta,
nikoli obecné doporučení nebo vydání balíčku. Po sobě jdoucí API běhy mohou mít jiné síťové prodlevy; pro
tvrzení o příčině výkonového rozdílu je nutné opakované měření. Placené použití
zahrnuje překlad i tvorbu paměti, nikdy hodnocení.


## Scheduling and identifier follow-up / Navazující ověření načasování a ID

V3 starts selection after accepting the source caption, giving the model time
before the next line arrives. A live server run then exposed mixed identifiers:
Gemma cited `o69` from diagnostic `event_id=69` although the actual source was
`o4`. Strict validation stopped the builder and used recent-history fallback.
V4 exposes only `id`, `text` and `speaker` to selection; renderer IDs remain in
local diagnostic records. All four v4 runs completed with no citation errors
and displayed all twelve target occurrences. This is a small diagnostic, not a
reliability estimate. The exact frozen variants and failed run remain published.

Luna got the intended sense in all six v4 targets, with one minor imperative
shift. Server Gemma still failed the bar-to-garden correction despite receiving
both new garden lines. Correct meanings after the v3 builder stopped are not
credited to long-term memory. Prefer recent history for this tested server
model; selected memory remains an opt-in candidate for wider Luna validation.
Forty-four Python tests pass; CI now runs them as well as the 18 native tests.

V3 spouští výběr hned po přijetí zdrojové repliky. Reálný běh serverové Gemmy pak
odhalil záměnu ID: model vytvořil `o69` z diagnostického `event_id=69`, přestože
zdrojová věta měla ID `o4`. Kontrola správně zastavila tvorbu paměti a ponechala
krátkou historii. V4 předává výběru pouze `id`, `text` a `speaker`; čísla událostí
zůstávají v místním diagnostickém záznamu. Čtyři běhy v4 neměly chybu odkazů a
zobrazily všech dvanáct cílových výskytů. Jde o malý diagnostický test, nikoli
odhad spolehlivosti. Přesné varianty kódu i neúspěšný běh zůstávají zveřejněné.

Luna ve všech šesti cílech v4 zachovala správný význam, jednou s drobným posunem
příkazu. Serverová Gemma opět nezvládla přesun z baru na zahradu, přestože obě
nové zahradní věty dostala. Správné významy po vypnutí paměti ve v3 nepřipisujeme
dlouhodobé paměti. U tohoto serverového modelu zatím doporučujeme krátkou historii;
u Luny je výběr původních replik kandidátem na širší ověření. Prošlo 44 Python
testů; nově běží také v CI vedle 18 nativních testů.

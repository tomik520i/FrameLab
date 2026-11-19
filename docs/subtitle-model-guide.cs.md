# Lokální překlad titulků a umístění českého překryvu

[English guide](subtitle-model-guide.md)

FrameLab čte **anglické titulky, které už jsou v obraze**, překládá je na tomto PC
a vykresluje češtinu po generování snímků a enhanceru. Zvuk se nepoužívá. Lepší
překladový model neopraví text, který OCR nepřečetlo správně.

## Rychlé spuštění

1. Otevři **Titulky**. Pokud OCR chybí, stiskni **Doplnit anglické OCR** a potvrď
   správce ve Windows. Potom vyber model a zapni překlad.
2. Zapni **Zobrazit oblast čtení** a vymez pruh těsně kolem anglických titulků.
   Vynech chat, minimapu a úkoly. Překlad čeká na dvě shodná čtení.
3. Velikost a polohu **českého překryvu** nastav nezávisle na oblasti čtení.
   F11 zobrazí čistý výsledek bez editačních prvků.

Windows musí mít nainstalovanou anglickou součást **OCR**. Aplikace ji automaticky
ověří a nabídne instalaci jedním tlačítkem přímo od Microsoftu. Potřebuje internet
a potvrzení správce; může trvat několik minut. Jazyk Windows se tím nemění.
Po instalaci se dostupnost znovu ověří. Až aplikace ohlásí připravené OCR, zapni
překlad. Pokud Windows potřebují restart, aplikace na to upozorní. Zrušené UAC
lze zkusit znovu. Jazykové nastavení Windows zůstává ruční záložní cestou.
Rozpoznávání pak funguje offline. CPU překladač už je v kompletním ZIPu aplikace.

## Který model vybrat

| Model | Stažení | Co od něj čekat |
|---|---:|---|
| OPUS-MT / CPU | Přibalený | Dvě CPU vlákna, paměť grafiky zůstává pro obraz. |
| CUNI-MH 7B Q3_K_M | 3,52 GB | Menší GPU model specializovaný na angličtinu do češtiny. |
| CUNI-MH 7B Q8_0 | 7,70 GB | Přesnější uložení vah, podstatně vyšší nároky na paměť. |

U GPU modelu stiskni **Stáhnout / pokračovat a ověřit**. První stažení doplní také
**přenosný Ollama runtime o velikosti 1,62 GB** z oficiálního vydání. Pro jeho
archiv a rozbalení nech přibližně 8 GB volného místa navíc. Stahování můžeš
pozastavit a obnovit. Před použitím se celý soubor ověří pomocí SHA-256.
Nepotřebuješ účet, API klíč ani systémovou instalaci Ollamy.

[Katalog modelů](subtitle-models.json) obsahuje přesné verze, velikosti, hashe
a odkazy. Modely jsou v `app/translation/models`, runtime v
`app/translation/gpu-runtime`. Na NTFS používá inference pevný odkaz na stejný
soubor; jiný souborový systém může potřebovat další kopii modelu. Při přesouvání
přenosné aplikace vezmi celou složku. GPU modely jsou volitelné stažení mimo ZIP.

Aplikace měří volnou VRAM při současně zapnutých efektech. Do volné kapacity se
promítá rozlišení, FG, enhancer i další programy. Doporučení je odhad s rezervou,
nikoli záruka podle názvu grafiky. Při nedostatku paměti zkus Q3 nebo CPU, sniž
pracovní rozlišení enhanceru nebo ukonči jinou GPU aplikaci. Při načítání se
kontroluje fyzická volná paměť a úplné umístění modelu na GPU. Částečný přesun na
CPU skončí vysvětlenou chybou. Výběr z více GPU zatím není v ovládání; testujeme
s jednou NVIDIA grafikou.

**Q8 nemusí překládat každou větu lépe.** V textovém srovnání stejných 300 vět
na stolním PC dosáhly obě varianty 271 přijatelných překladů. Q3 mělo dvě zásadní
chyby, Q8 pět; 175 výstupů bylo shodných. Hodnotil asistent, nejde o zaslepenou
lidskou studii. Tento předběžný textový test neurčuje výkon při souběhu s OCR,
frame genem a NR. Starší report **Hy-MT2 Q8** se týká jiného modelu o velikosti
1,8B, nikoli CUNI-MH 7B Q8.

Na RTX 5070 Ti při současném VFG 2×, NR 65 % / 720p a aktivní CPU OCR cestě měl Q3 medián **212 ms**, Q8 **360 ms**. Jde o dobu překladu přesného textu, ne celé zpoždění titulků. [Kompletní měření a všech 300 párů](https://opicebot.cz/framelab/translation/?lang=cs).

## Český text a jeho černé pozadí

- **Vlastní poloha:** nastav vodorovnou/svislou polohu a ukotvení, nebo zapni
  **Přetahovat český překryv v náhledu**. Text a pozadí se posouvají společně.
  Přetažení přepne na vlastní polohu; oblast OCR se tím nezmění.
- **Pod originál / Nad originál:** poloha se počítá od příslušného okraje oblasti
  OCR a zvolené mezery. Oblast proto vymez těsně kolem původních titulků.
- **Když není místo:** vyber přesun na opačnou stranu, nebo přisunutí k okraji
  obrazu. Když překryv nevejde ani na jednu stranu, zůstane uvnitř obrazu a může
  se s angličtinou překrývat. Náhled na tuto úpravu upozorní.

Poloha i mezera jsou uložené vzhledem k obrazu, takže přežijí změnu velikosti
okna, rozlišení i fullscreen. Počítá se s více řádky a okraji černého pozadí.
Překryv zůstává uvnitř viditelné části i v režimu vyplnění/ořezu nebo 1:1, kdy
samotný zdrojový obraz může přesahovat okno.
Neobvykle dlouhý text se zmenší, aby zůstal viditelný. Velikost písma a krytí
pozadí mají vlastní nastavení.

**Zakryt oblast původních titulků** je samostatný černý obdélník přes oblast OCR.
Pokud chceš ponechat angličtinu viditelnou, tuto volbu vypni. Černé pozadí českého
textu funguje nezávisle.

Fullscreen ukazuje jen výsledný obraz a aktivní překlad. Nemá úchyty, obrysy OCR,
popisky ani porovnávací dělicí čáru. Esc nebo F11 tě vrátí do nastavení.

## Odezva, přepínání a soukromí

OCR a překlad běží v oddělených pracovních vláknech. Výchozí interval čtení je
300 ms a vyžadují se dvě shodná čtení. Ve frontě zůstává jen nejnovější čekající
obraz a text. Změna nebo zmizení anglického textu vymaže předchozí překlad;
opožděný výsledek už neplatné věty se nezobrazí. Cache uchovává nejvýše 128 vět.
Změna zdroje vyčistí čekající titulky. Vypnutí překladu uvolní model i procesy,
které pro něj FrameLab spustil. Načítání a zahřátí modelu nezastavuje vykreslování.

CPU překladač komunikuje soukromou rourou. GPU překladač spouští vlastní Ollamu
pouze na `127.0.0.1`, s vypnutými cloudovými funkcemi, jedním modelem a jedním
požadavkem současně. Obraz a překládané titulky zůstávají na tomto PC. Při
stahování se připojuje k vydavatelům modelu/runtime; titulky se jim neposílají.
Chyby pracovních procesů jsou v `app/logs`.

Malý, pohyblivý, stylizovaný nebo krátce zobrazený text může OCR minout.
Rozpoznávání, čekání na stabilní text i samotný překlad přidávají zpoždění.
Zmrazený náhled ponechá překlad platný v okamžiku zmrazení. Analytické RAW/FG
záznamy nezahrnují český překryv ani NR; zachycují vstup a generování snímků.

## Původ komponent

Přibalený CPU model je Helsinki-NLP / OPUS-MT `en-cs/opus-2019-12-18.zip`,
v tomto konkrétním archivu pod licencí **CC BY 4.0**. Byl převeden na
CTranslate2 4.9.0 int8 bez dalšího trénování. Atribuce, licence i SentencePiece
soubory zůstávají přibalené. Přesné vstupy obsahuje
[translation-components.json](translation-components.json).

CUNI-MH je Mistral 7B doladěný týmem CUNI WMT pro překlad do češtiny.
Archivovaná původní karta modelu uvádí MIT; kvantizace GGUF poskytuje Richard
Erkhov. Odkazy vedou na existující repozitáře vydavatelů, FrameLab tyto váhy
nepřejmenovává ani nezrcadlí. Katalog připíná konkrétní revize, hashe a licenci
oficiálního runtime. Používáme původní formát promptu CUNI s prázdnou historií
a slovníkem, teplotou 0, kontextem 2048 tokenů a limitem 256 tokenů.

Podrobnosti sestavení a testů jsou v [anglickém návodu](subtitle-model-guide.md).
Adaptér NR je ve výchozím sestavení zapnutý, externí modelové DLL se do veřejného
balíku nepřibalují. `FRAMELAB_EXPERIMENTAL_NR=OFF` vynechá i adaptér.

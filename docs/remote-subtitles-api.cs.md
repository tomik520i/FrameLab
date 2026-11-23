# Remote Play, samostatný překryv titulků a OpenAI API

Součást veřejného balíčku preview.18. Původní vývojová měření pocházejí z 23. listopadu 2026; jejich podmínky a výsledky se vydáním nemění. Překlad přes vlastní API klíč je volitelný a placený zvlášť. Místní překlad zůstává dostupný.

## Co je implementováno

- Vstup z okna PC přes Windows Graphics Capture na zvolené GPU. Režim **Jen titulky přes původní okno** snímá 5× za sekundu, pozastaví FG a NR a zobrazí samostatný průhledný překryv. Zdrojová hra se nepřekresluje jako video přes sebe.
- F11 v tomto režimu minimalizuje nastavení a vrací zdrojové okno. Ctrl+Alt+F11 vrátí nastavení. Kliknutí prochází přes titulek; překryv se skryje při přepnutí do jiné aplikace, minimalizaci či zániku zdroje, pozastavení nebo vypršení titulku.
- České i další znaky se vykreslují pomocí DirectWrite v rozlišení cílového okna. Nastavení polohy, velikosti, pozadí a zakrytí originálu se používá i pro samostatný překryv. Vlastní pozicí lze překlad umístit pod originál.
- V Titulcích zůstává **Local / offline** a přibyla možnost **OpenAI API / BYOK**. Lokální modely nebyly odstraněny. Cloudový poskytovatel neposílá obrázky ani zvuk; posílá rozpoznaný text s překladovými instrukcemi. OCR a snímání dál využívají PC.
- Seznam modelů se načítá z účtu přes `/v1/models`. Jejich parametry určuje samostatný ověřený soubor `docs/openai-model-profiles.json`, protože endpoint schopnosti modelů nevrací. Neověřená ID nejsou volitelná. Reasoning a temperature se řídí profilem konkrétního modelu; neodhadují se z názvu.
- Výchozí GPT-6 Luna používá reasoning `none`, teplota se neposílá. Modely vyžadující reasoning mají jiné povolené volby. Změna poskytovatele/modelu placený překlad vypne. Po API chybě není automatická placená smyčka opakování.
- Klíč je ve Windows Credential Manager pro současného uživatele (`FrameLab/OpenAI/APIKey`), mimo balíček, nastavení, logy a parametry procesu. Maskované pole umožňuje uložení a odstranění. API zůstává při běžném spuštění vypnuté, dokud je uživatel nepovolí.
- Volání používají Responses API, TLS, pevnou adresu OpenAI, zákaz přesměrování, timeout, `store=false` a strukturovaný JSON. `store=false` neznamená nulové logování poskytovatelem. Chyby nevracejí obsah autorizace. Log spotřeby obsahuje model a tokeny, nikoli API klíč nebo rozpoznaný text.

## Ověření

- 18/18 CTest; samostatné testy Python API a existujícího filtru.
- Nativní zachytávání znovu prošlo na Intel UHD i RTX 4050 Laptop: správný výřez klientské části, pohyb za překrývajícím oknem, změna rozměrů, minimalizace/obnovení, uzavření zdroje, výběr GPU a uvolnění požadavku na nezhasínání displeje.
- Šest případů skutečné cesty OCR → proces překladače → OpenAI API: dialog, samostatná jména, číselný kód a krátká replika.
- Viditelná zkouška: původní okno s anglickým textem „John, wait! The code is 1234.“, překlad „Johne, počkej! Kód je 1234.“, F11/minimalizace, průchod kliknutí do počítadla zdroje, návrat klávesovou zkratkou, posun pod originál. Snímky jsou v obrazovém návodu.
- 500 shodných EN → CS položek pro OPUS/CPU, CUNI-MH Q3_K_M, TranslateGemma 4B Q4_K_M a GPT-6 Luna. Původní lidské reference z Wesnoth, pevný seed a hash. Hlavní benchmark nemá OCR/FG/NR; není měřením latence hry. API a lokální běh se překrývaly, místní modely běžely postupně.
- Hodnocení zahrnuje chrF++, BLEU a anonymizované posouzení 2 000 překladů jedním GPT-6.1 Terra. Toto AI posouzení není nezávislé lidské hodnocení; jedna překladová varianta i posuzovatel jsou od stejného dodavatele. Report zveřejňuje i jednotlivé poznámky a všechny chyby.
- Samostatná sada 100 případů filtru: odstraněno 36/40 nežádoucích textů, ztraceno 0/40 jasných replik, 20 nejednoznačných případů evidováno odděleně. Přirozenou repliku nelze vždy odlišit od stejného nápisu jen podle textu.

## Návod a data

- [Obrazový návod CZ](https://opicebot.cz/framelab/guide/remote-subtitles.html)
- [Illustrated guide EN](https://opicebot.cz/framelab/guide/remote-subtitles-en.html)
- [500 položek, výsledky, ceny a kompletní log](https://opicebot.cz/framelab/runs/2026-11-23-api500/)
- [Reprodukce testu](https://opicebot.cz/framelab/runs/2026-11-23-api500/data/REPRODUCE.md)

PlayStation Remote Play a Xbox Remote Play mohou poskytovat PC okno bez střihové karty; aktuální oficiální postupy a odkazy jsou v návodu. Konzole nebyla při této zkoušce k dispozici, takže nebylo ověřeno skutečné připojení PS/Xbox ani chování jejich aktuálních aplikací při zachytávání. Exkluzivní fullscreen, chráněný obsah a aplikace blokující zachytávání nejsou automaticky podporovány; doporučené je okno nebo režim bez okrajů.

## Distribuce a další práce

Webový návod/report je nezávislý na vydání aplikace. Současná změna nezveřejňuje nový ZIP, NR DLL ani modelové váhy. Balicí skripty už zahrnují API adaptér, profily a nové návody pro budoucí vydání. Před novým releasem je potřeba samostatně sestavit a ověřit úplný balíček a jeho distribuovatelné závislosti.

Přesné nové zdroje jsou v pracovním stromu; veřejný audit nabízí jen allowlist skriptů, promptů, profilů a dat. Uživatelský API klíč, seznam soukromých modelů účtu ani osobní nastavení v exportu nejsou.

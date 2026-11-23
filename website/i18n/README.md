# Česká a anglická verze webu / Czech and English website

## Česky

Všech 32 HTML stránek má českou a anglickou variantu. Návody mají samostatné párové soubory; úvodní stránka, licence, galerie a analýzy se přepínají na stejné adrese pomocí `?lang=cs` / `?lang=en`. Volba se ukládá v prohlížeči a přenáší do interních odkazů. Přepnutí v reportech zachovává vyhledávání, filtry a stránku výsledků.

Přeloženy jsou nadpisy, vysvětlení, metodika, ovládací prvky, popisky, přístupnost i zobrazované hodnoticí poznámky. Skutečné vstupy OCR, překlady modelů, referenční věty, citované chybné výrazy, identifikátory modelů a původní screenshoty zůstávají beze změn. Jinak by porovnání přestalo ukazovat skutečný výsledek. Původní JSON, CSV, logy a archivované zdrojové balíky zůstávají původními důkazy, nikoli novým překladem dat.

Texty rozhraní jsou ručně připravené. Pracovní návrhy překladů poznámek vznikly na lokálním modelu a byly přečteny a opraveny Codexem, zejména u gramatických chyb, záporů a citací. Nejde o nové hodnocení modelů ani ověření lidským překladatelem. Lokalizace nepoužila žádné placené API požadavky. Historické poznámky, které prohlížeč reportu nezobrazuje, nejsou součástí jazykových katalogů a zůstávají v původních datech.

Údržba: uprav `i18n/ui.json`, existující `data-cs` / `data-en` nebo oba zdrojové návody. Po generování návodů či změně stránky spusť `python tools/localize_website.py` a `python tools/check_website_localization.py` z kořene repozitáře (vyžadují beautifulsoup4). Zkontroluj i dynamické texty v JS. Poznámky jsou v `i18n/notes/`, jejich cesty v `notes-index.json`; stejný klíč musí mít všude stejný překlad. Chunks nesmějí měnit originál obsažený v klíči. Před nasazením otestuj obě verze přes HTTP, včetně mobilního zobrazení a filtrů. Jazykové soubory přenes před HTML. Přibalené offline návody používají přímé odkazy a nepotřebují `fetch` přes `file://`.

## English

All 32 HTML pages have Czech and English versions. Guides use paired files; the homepage, licenses, galleries and analyses switch at the same address with `?lang=cs` / `?lang=en`. The browser remembers the selection and carries it through internal links. Switching reports preserves searches, filters and pagination.

Headings, explanations, methodology, controls, captions, accessibility labels and displayed review notes are translated. Actual OCR inputs, model translations, reference sentences, quoted faulty wording, model identifiers and original screenshots remain unchanged so comparisons still show the recorded results. Original JSON, CSV, logs and archived source bundles remain evidence, rather than translated datasets.

UI text was authored manually. Review-note drafts used a local model and were read and corrected by Codex, especially for grammatical terminology, negation and quotations. This is not a new model assessment or validation by a human translator. Localization used zero paid API requests. Historical annotations not displayed by the report viewer are excluded from locale catalogs and retained in the original data.

Maintenance: edit `i18n/ui.json`, existing `data-cs` / `data-en` attributes, or both source guides. After rendering guides or changing a page, run `python tools/localize_website.py` and `python tools/check_website_localization.py` from the repository root (beautifulsoup4 required). Also check dynamic JS text. Notes live in `i18n/notes/`, with paths in `notes-index.json`; repeated keys must have identical translations. Never change the source text embedded in a note key. Before deployment, test both languages over HTTP, including mobile layout and filters. Upload locale assets before HTML. Packaged offline guides use direct links without requiring `fetch` over `file://`.

# FrameLab preview.13 — instalace a první spuštění

**[Stáhnout kompletní FrameLab pro Windows x64](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.13/FrameLab-0.2.0-preview.13-windows-x64-full.zip)** · [Všechny soubory vydání](https://github.com/tomik520i/FrameLab/releases/tag/v0.2.0-preview.13) · [English](START_HERE.md)

## 1. Stáhnout → rozbalit → spustit

1. Stáhni výše uvedený soubor **windows-x64-full.zip**. GitHubové **Source code** je pro vývojáře, není to hotová aplikace.
2. Pravým tlačítkem na ZIP → **Extrahovat vše**. Celou složku dej například do nové krátké složky na Ploše. Dlouhé zanořené cesty mohou ve Windows způsobit chybu 0x80010135; v tom případě rozbalení zopakuj do kratší cesty a žádné soubory nepřeskakuj. Nespouštěj program přímo ze ZIPu.
3. Otevři **1 - Spustit FrameLab.cmd**. Spustí se přibalená třísekundová ukázka CS:GO ve smyčce.
4. V kartě **Vstup** můžeš přepnout na **Spider-Mana** nebo na test **Vyzkoušet titulky EN > CZ**. Pro ukázky nepotřebuješ střihovou kartu.

To je celý postup instalace. **Python, pip, NVIDIA SDK ani kompilátor neinstaluješ.** CPU model už je přibalený; volitelné CUNI modely se stahují přímo tlačítkem v aplikaci. Všechny potřebné soubory aplikace jsou přibalené. Složky `app`, `tools`, `deps` a `licenses` nech u sebe; nestačí překopírovat samotné EXE. První spuštění nevyžaduje přihlášení ani automatické stahování modelu.

Požadavky: **Windows 10/11 x64**, NVIDIA grafika s Direct3D 11, aktuální ovladač grafiky a pro živý vstup ovladač capture karty. NVIDIA VFG vyžaduje **RTX 40/50 (Ada/Blackwell)**. Alternativní Optical Flow je experimentální a má jinou kvalitu. Fyzicky byly zkoušené RTX 5070 Ti a RTX 4050 Laptop; starší RTX nejsou tímto vydáním hardwarově ověřené. ARM, Linux a macOS nejsou podporované sestavení.

Aplikace není digitálně podepsaná. Ověř původ stažení a případně SHA-256 podle vydání; není potřeba vypínat antivirus ani ochrany Windows.

## 2. Vyzkoušet frame generation

| Co otevřít | Co se spustí |
| --- | --- |
| `1 - Spustit FrameLab.cmd` | CS:GO bez dopočtu |
| `2 - NVIDIA RTX 40-50.cmd` | CS:GO + NVIDIA VFG 2× / Medium |
| `3 - Optical Flow.cmd` | CS:GO + alternativní Optical Flow 2× |
| `Ukazka - Spider-Man.cmd` | Přibalený Spider-Man |
| `Ukazka - Ceske titulky.cmd` | Označený umělý test anglického textu + český překlad + VFG |

V kartě **Frame gen** zapni generování, vyber backend a začni **2× / Střední**. 3× a 4× mohou přidávat artefakty, latenci a zahozené snímky. NVIDIA VFG a Optical Flow jsou alternativy; nezapínají se oba za sebou. Počet odeslaných snímků není záruka skutečných FPS monitoru.

## 3. Připojit konzoli nebo druhé PC

1. Zdrojové zařízení připoj HDMI k capture kartě, kartu k tomuto PC podle návodu výrobce.
2. Zavři jiné programy, které kartu právě používají.
3. **Vstup → Zdroj**: vyber svoji kartu. Zkus automatický režim nebo 1920 × 1080 a odpovídající vstupní FPS.
4. Klikni na **Spustit vstup**. V kartě **Zvuk** vyber výstup a hlasitost.
5. Zapni požadovaný dopočet v **Frame gen**.

Rozlišení a frekvence jsou z nabídky ovladače. Když ovladač nabízí 4K nebo širokoúhlý režim, objeví se v seznamu. Nejde o slib, že karta přijme každý signál; ovladač může požadavek odmítnout nebo upravit. Automaticky neznamená detekci skutečného HDMI režimu. Veškeré 4K/vysoké frekvence nebyly v této verzi fyzicky otestované. HDCP se neobchází a HDR není implementované.

## 4. Anglické titulky v obraze → český překryv

1. Otevři **Titulky**. Pokud anglické OCR chybí, stiskni **Doplnit anglické OCR**, potvrď UAC a počkej na připravený stav. Jde o jednorázové stažení od Microsoftu; jazyk Windows se nemění.
2. Vyber přibalený **OPUS-MT / CPU**, nebo **CUNI-MH Q3_K_M / Q8_0** a stiskni **Stáhnout / pokračovat a ověřit**. První GPU model doplní i vlastní přenosný runtime. Ollamu ručně neinstaluješ a nepotřebuješ účet ani API klíč.
3. Zapni překlad a **Zobrazit oblast čtení**. Vymez jen pruh s anglickými titulky; vynech HUD a chat.
4. Zvol **Pod originál**, **Nad originál** nebo **Vlastní poloha**. Volba **Přetahovat český překryv v náhledu** umožní táhnout text i černé pozadí společně. Oblast OCR se nemění. Více řádků zůstane uvnitř viditelné části obrazu, i při ořezu a ve fullscreenu. Neobvykle dlouhý text se zmenší, aby se vešel.
5. F11 zobrazí čistý výsledek. Pro kontrolu použij umělou ukázku: „The door is locked.“ → „Dveře jsou zamčené.“.

**[Celý návod: modely, stahování a umístění titulků](docs/subtitle-model-guide.cs.md)**. Q3 stahuje 3,52 GB, Q8 7,70 GB. První GPU model potřebuje také runtime archiv 1,62 GB a přibližně 8 GB místa navíc na runtime a rozbalení. Volná VRAM se kontroluje při současně zapnutých efektech. Model se načítá na pozadí; částečný přesun výpočtu na CPU se tiše nepovolí. CPU překladač funguje bez stahování GPU modelů.

**Čte se obraz, nikoli zvuk. Není tu STT ani cloudový překlad.** OCR potřebuje dvě shodná čtení. Po stažení komponent funguje vše offline. Malý, stylizovaný či krátce zobrazený text se může přečíst špatně; lepší model neopraví chyby OCR. Změna nebo zmizení původního textu vyčistí český titulek a opožděný výsledek se už nezobrazí.

## 5. Monitor, okno a klávesy

V **Zobrazení** vyber monitor, dostupnou velikost okna a režim **vměstnat / vyplnit s ořezem / 1:1**. FrameLab nemění rozlišení ani obnovovací frekvenci Windows; ty nastav v systémovém nastavení displeje. Nabídka okna respektuje pracovní plochu monitoru.

- **F11**: jeden výsledný obraz s nastavenými efekty a titulky. Porovnání, panely, popisky a rámeček OCR se automaticky skryjí. Zmrazené porovnání se při přechodu rozběhne. **Escape / F11**: návrat do nastavení.
- **F1**: skrýt/zobrazit ovládací panel pouze v okně. Ve fullscreenu nic skrývat nemusíš.
- **F8**: zmrazit porovnání v okně. Dělicí čáru NR lze táhnout myší. Ve fullscreenu se porovnání nezobrazuje.
- Kurzor ve fullscreenu zmizí po **2 sekundách** nečinnosti nad obrazem. Pohybem se objeví.
- **[Obrazový návod: fullscreen krok za krokem](https://opicebot.cz/framelab/guide/fullscreen.html)**. V ZIPu otevři **FULLSCREEN.html**; obrázky fungují i offline.
- **CZ / EN**: změna jazyka. Kompletní balík začíná anglicky; tlačítkem EN / CZ přepneš do češtiny.

## 6. Co ZIP obsahuje a co není součástí

Přibalené jsou aplikace, oficiální NVIDIA VFG runtime, potřebné Microsoft runtime DLL, dvě herní ukázky, umělý OCR test, překladový model a jeho Python, exportér s FFmpeg, návody a původní licence. Velké závislosti jsou v release ZIPu, nikoli v historii Git repozitáře.

**Chceš také enhancer? [Dva downloady a průvodce: kompletní návod NR](docs/enhancer-install.cs.md).** Preview.13 už obsahuje potřebný adaptér a tlačítko AI / NR. Otevři `4 - Doplnit enhancer.cmd`: nabídne přesný externí odkaz a import staženého ZIPu bez ručního kopírování. Externí Veyra ZIP má dalších 731 MB. NR DLL nejsou v našem balíku; jde o komunitní zdroj s nepotvrzenými právy k použití a distribuci, nikoli oficiální download NVIDIA. Import je určen pro uživatele s příslušným oprávněním. [Původ a podmínky](docs/NR-LICENSING-STATUS.txt).

Po doplnění a zapnutí NR je pořadí vstup → zvolený frame gen → enhancer → český překryv → monitor. Bez NR se tento krok vynechá. Další zpracování může přidat latenci nebo snížit výstupní FPS.

Licence a zdroje: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), [původ komponent](docs/runtime-distribution.md), [herní ukázky](docs/sample-provenance.md). Licence programu nepřelicencovává NVIDIA komponenty ani herní obsah.

## 7. Řešení problémů

| Problém | Postup |
| --- | --- |
| Chybějící DLL / model / ukázka | Rozbal znovu celý **full.zip** do nové složky. Nepoužívej automatický GitHub Source code ZIP pro spuštění. |
| NVIDIA VFG nejde | Ověř RTX 40/50 a ovladač; zkus spouštěč Optical Flow. Podrobnosti jsou v `app/logs`. |
| Černý živý vstup | Ověř kabel, signál a náhled výrobce karty; zavři další aplikace; zkus automaticky nebo 1080p. |
| Překlad nic neukazuje | Ověř anglické OCR a rámeček; vyzkoušej umělý OCR test. Text musí být chvíli stabilní. |
| Překlad čte HUD | Zmenši oblast čtení pouze na titulky. |
| Nízký výkon / duchy | Sniž násobič na 2×, porovnej vypnutý FG, sniž vstupní rozlišení. |
| Nevidím všechny karty nastavení | Použij šipky vpravo u záložek nebo zvětši okno. |
| Obraz na HDMI passthrough nemá FG | Zpracovaný výstup je v okně FrameLabu / na jeho monitoru, ne v passthrough karty. |

Analytické RAW/FG nahrávky neobsahují český překryv ani NR a nenahrávají zvuk. Recorder potřebuje dost RAM a místa. [Podrobnosti](docs/recordings.md).

## Aktualizace, odstranění a kontrola stažení

Nové vydání rozbal do **nové složky**. Starou kopii ponech, dokud novou nevyzkoušíš. Nastavení není nutné přenášet. Program neinstaluje službu ani ovladač; odinstalace je smazání jeho složky po ukončení aplikace (včetně případných záznamů, které si nejprve ulož jinam).

Pro volitelnou kontrolu ZIPu v PowerShellu spusť `Get-FileHash .\FrameLab-0.2.0-preview.13-windows-x64-full.zip -Algorithm SHA256` a porovnej výsledek se souborem **SHA256SUMS.txt** u vydání. [Sestavení ze zdrojů](docs/build-preview13.md) je samostatná vývojářská cesta.

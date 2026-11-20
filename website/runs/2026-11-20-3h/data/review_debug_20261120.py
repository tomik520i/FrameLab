"""Explicit single-assistant judgments of the frozen 300-pair sample.

This is an audit record, not an automatic scorer. No unreviewed row gets a grade.
Run after analyze_debug_run.py; no model calls or application changes.
"""
import json
from collections import Counter
from pathlib import Path

BASE = Path('analysis-output/debug-run-1795196244267574/data')
rows = json.loads((BASE/'quality-sample.json').read_text(encoding='utf-8'))
grades = {}

def add(ids, label, cs, en):
    for i in ids:
        assert i not in grades, i
        grades[i] = dict(id=i, grade=label, note_cs=cs, note_en=en)

# Every ID below was read alongside its English and Czech text. Punctuation,
# stutter reduction and unknowable speaker gender alone are not penalized.
add([4,13,19,21,24,26,29,39,49,52,55,68,79,85,86,88,91,95,100,106,111,117,121,134,139,144,148,153,160,161,175,176,186,188,194,195,197,199,206,208,221,222,226,240,245,285,287,304,314,315,321,323,324,327,328,331,337,341,342,347,356,366,368,369,376,379,401,408,409,412,415,416,419,429,446,466,469,475,486,487,494,495,503,523], 'good',
    'Smysl logovaného vstupu zachován; případný rod nebo referent bez obrazu nelze ověřit.',
    'Logged meaning preserved; speaker gender or referents cannot always be verified without the video.')
add([539,543,544,547,551,556,557,560,576,592,594,596,598,606,626,627,629,631,633,654,663,665,666,668,675,677,690,691,693,697,701,704,706,723,733,740,750,761,765,773,779,787,797,810,814,816,828,829,833,834,838,845,846,854,856,864,870,874,876,877,879,892,924,945,951,954,977,980,983,986,990,995,1001,1005,1013,1016,1019,1020,1029,1041,1043], 'good',
    'Smysl logovaného vstupu zachován; případný rod nebo referent bez obrazu nelze ověřit.',
    'Logged meaning preserved; speaker gender or referents cannot always be verified without the video.')
add([1047,1053,1054,1055,1061,1078,1080,1082,1091,1109,1118,1120,1127,1147,1157,1163,1205,1208,1211,1214,1229,1247,1271,1272,1282,1283,1286,1289,1324,1330,1347,1355,1360,1375,1389,1399,1401,1414,1415,1420,1426,1433,1435,1448,1467,1472,1481,1494,1495,1510,1516,1518,1519,1521,1556,1568,1598,1604,1608,1613,1614,1662,1664,1672,1676,1690,1694,1697,1699,1702,1705,1708,1712,1723], 'good',
    'Smysl logovaného vstupu zachován; případný rod nebo referent bez obrazu nelze ověřit.',
    'Logged meaning preserved; speaker gender or referents cannot always be verified without the video.')

minor = {
6: ('Vynecháno „Too bad“, hlavní podmínka a zápor zůstávají.', 'Drops “Too bad”; the main condition and negation survive.'),
30: ('Vynechán souhlas „yeah, absolutely“, hlavní ujištění zůstává.', 'Drops the agreement “yeah, absolutely”; retains the main reassurance.'),
36: ('„Nebude jim nic procházet“ je nepřesně převedeno na „nedostanou se z toho“.', 'Shifts “will not get away with it” to “will not get out of it”.'),
165: ('Doslovné „bojuje“ místo přirozeného „má potíže / sotva vychází“.', 'Literal “fights” for “is struggling”; awkward but recognizable.'),
201: ('Význam zůstává, ale „čekám na setkání“ zní nepřirozeně formálně.', 'Meaning survives, but the Czech wording is stiff and overly formal.'),
211: ('„Objímač“ je doslovný, nepřirozený výraz.', 'Literal, unnatural Czech noun for “hugger”.'),
228: ('Nepřirozená vazba smlouvy na základnu místo zakázky od základny.', 'Awkward contract relation: a contract for the base instead of business with it.'),
313: ('Obecné „nenechají se chytit“ se mění na minulý čas.', 'Changes a general statement into past tense.'),
351: ('Doplňuje pomoc sobě, ač „help out“ příjemce neurčuje.', 'Adds helping oneself where the source does not identify the beneficiary.'),
367: ('„Vyřídím to venku“ je doslovně „vezmu to ven“; okolní dialog naznačuje hovor.', 'Literal “take it outside”; surrounding dialogue suggests taking a call outside.'),
420: ('Ztrácí odstín „je to na nás tak poznat?“', 'Loses the nuance “is it that obvious about us?”.'),
527: ('Doslovné „přiveďte to“ nepůsobí jako přirozené pobídnutí; přesné gesto není v logu.', 'Literal “bring it” is unnatural; the exact gesture is unavailable in the log.'),
535: ('Vynechána zmínka o tancování.', 'Omits the reference to dancing.'),
638: ('Chybná vazba „s staršími“ místo „se staršími“.', 'Minor Czech preposition/euphony error.'),
769: ('„Slezl ze stolu“ místo přirozeného „odešel od stolu“ v herním kontextu.', '“Climb down from the table” instead of leaving the game/table.'),
844: ('Přidáno nadbytečné „Ne“, význam zákazu zachován.', 'Adds a redundant “No”; the prohibition is preserved.'),
913: ('Dialog přeložen, ale přimíchaný HUD text „LOOT“ zůstává anglicky.', 'Dialogue translated, but mixed-in HUD text “LOOT” remains English.'),
916: ('Dialog přeložen správně; přimíchané „PICK UP“ vypuštěno. To může být pro titulky žádoucí.', 'Dialogue correct; omits mixed-in “PICK UP”. This may be desirable for subtitles.'),
930: ('„U dálnice“ posunuto na „mimo dálnici“.', 'Changes “off/near the highway” to “outside the highway”.'),
975: ('„Doklad“ přeložen nepřirozeně jako „nějaká identifikace“.', 'Unnatural “some identification” instead of an ID/document.'),
996: ('Srozumitelné, ale kostrbaté české znění „nebo cokoliv“.', 'Understandable but awkward Czech wording for “or anything”.'),
1126: ('Vynecháno úvodní „Very sure“, ujištění zachováno.', 'Drops “Very sure”; retains the reassurance.'),
1174: ('Název zbraně zachován, obecné slovo PISTOL nepřeloženo.', 'Weapon name preserved, but the generic word PISTOL is untranslated.'),
1233: ('Formální „Oceňuji to“ místo přirozeného poděkování.', 'Formal literal wording instead of a natural thank-you.'),
1244: ('Nepřirozené „Mám tu hovor“ místo „Telefonuju“.', 'Awkward literal wording instead of “I am on the phone”.'),
1357: ('„Good thing going“ zúženo na dobrou práci; zdroj to výslovně neurčuje.', 'Narrows “a good thing going” to a good job without explicit source support.'),
1373: ('Úvodní „my to uděláme“ posunuto na „ty to chceš udělat“.', 'Shifts “we are going to do this” to “you want to do this”.'),
1385: ('Význam ovládání zůstává, přirozenější by bylo „plyn“ než „akcelerátor“.', 'Meaning retained; “throttle” would be more idiomatic in Czech.'),
1422: ('Vynecháno převzetí zboží před doručením.', 'Omits obtaining the goods before delivering them.'),
1431: ('Podmínka „pokud se bude mluvit“ posunuta na „jestli chcete mluvit“.', 'Changes “if there is any talking” to “if you want to talk”.'),
1520: ('„Off“ posunuto na „v deltě“, navíc nepřirozené číslo podstatného jména.', 'Location preposition shifts; also awkward Czech number agreement.'),
1596: ('„Vadný kus / nefunkční věc“ zjednodušeno na „chyba“.', 'Reduces “a dud” to a generic error.'),
1710: ('Zákaz neztratit se v myšlenkách je česky velmi kostrbatý.', 'The instruction not to get lost in thoughts is very awkward in Czech.'),
1743: ('„Navštívená místa“ změněno na neobratné „návštěvní místa“.', 'Awkward Czech phrase for visited locations.')}
major = {
204: ('Obrácený zápor: uděláme párty → neuděláme párty.', 'Negation reversal: we will make it a party → we will not.'),
431: ('Problémy s penězi a pronajímatelem změněny na má peníze a byt.', 'Money/landlord problems become having money and an apartment.'),
505: ('„Shots“ jsou v okolním dialogu panáky, nikoli povel střílet (ID 494–503).', '“Shots” are drinks in the surrounding dialogue, not an order to shoot (IDs 494–503).'),
587: ('Budoucí získání sympatií změněno na minulé „vyhrál jsi ji“.', 'Future winning her over becomes past “you won her”.'),
676: ('„Měli u sebe“ změněno na „měli v plánu“; delší ID 677 už je správně.', 'Possession becomes intention; the expanded source in ID 677 is translated correctly.'),
712: ('„Smoke him“ v kontextu hry znamená porazit ho; „vykuř ho“ je chybný význam.', '“Smoke him” in the game context means beat him, not the Czech sexual/literal smoking sense.'),
741: ('„Necháš si to líbit?“ přeloženo jako „vezmeš si to?“', '“Are you going to take that?” loses its tolerate-the-insult idiom.'),
798: ('Idiom „a je konec“ doslovně převeden na to, co napsala žena.', 'The idiom “that is the end” is rendered literally as what a woman wrote.'),
848: ('„Picked up“ je zde pořízení věci, ne „pochytil“; navazující ID 851 mluví o nákupu.', 'Here “picked up” means acquired, not learned; ID 851 explicitly mentions a purchase.'),
849: ('Model doplní „nápad“, přestože navazující ID 851 upřesňuje koupenou věc.', 'Invents an “idea” where the subsequent ID 851 clarifies a purchased item.'),
907: ('Celý srozumitelný pokyn PICK UP LOOT zůstává nepřeložen. Je to HUD, ne dialog.', 'The complete meaningful instruction PICK UP LOOT is untranslated. This is HUD, not dialogue.'),
923: ('Bankovní účet změněn na adresu; následující ID 924 potvrzuje účet.', 'A bank account becomes an address; ID 924 confirms the account context.'),
928: ('Přidána neexistující odpověď druhého mluvčího „Ne“.', 'Invents a second speaker’s reply “No”.'),
1060: ('Drzost změněna na nervy a úvěrové hodnocení na úvěr.', 'Audacity becomes nerves and creditworthiness becomes a loan.'),
1085: ('Přidána neexistující odpověď druhého mluvčího „Dobře“.', 'Invents a second speaker’s reply “Okay”.'),
1186: ('Večírek tvé známé z vězení změněn na tvůj vězeňský večírek; mění vztahy.', 'A party thrown by your ex-con acquaintance becomes your prison party, changing relationships.'),
1319: ('Model přidá ryby, které ve zdroji nejsou, a ztratí význam špinavých šmouh.', 'Invents fish and loses the dirty-stains/skidmarks meaning.'),
1423: ('Vynecháno „strapped“ = ozbrojení, podstatná informace o poslíčcích.', 'Omits “strapped” = armed, a material fact about the delivery men.'),
1428: ('Slangové „piece“ = zbraň zůstane jako neurčitý kus (okolí ID 1423).', 'Slang “piece” = gun becomes an unspecified piece (context ID 1423).'),
1500: ('Místo shozu zásilky změněno na místo seskoku.', 'A package drop becomes a parachute-jump location.'),
1577: ('„Zvládnu to“ zmizí: věta je přestavěna na „mám hloupou nohu“.', 'Loses “I can handle this”, rebuilding it as “I have this stupid leg”.'),
1616: ('Hrozba promluvit proti někomu přeložena jako špatné přimlouvání; vypuštěno „Think again“.', 'A threat to speak against someone becomes poor advocacy; also drops “Think again”.'),
1726: ('Místní název Key Lento změněn na klávesu Lento (srov. ID 1205).', 'The place name Key Lento becomes a keyboard key (compare ID 1205).'),
1737: ('„Vykradené trezory“ zůstane jako směs SAFES VYKRADEN.', '“Safes robbed” becomes a broken English/Czech mixture.')}
unknown = {
375: ('Bez scény nelze určit těžší versus silněji.', 'Without the scene, harder could mean more difficult or more forcefully.'),
1281: ('Směs HUD a jména nemá dost souvislého významu pro posouzení věty.', 'Mixed HUD fragments and a name lack enough coherent meaning to judge as a sentence.'),
1633: ('U eliptického „I do / we do“ chybí otázka pro spolehlivé posouzení doplněného „mám“.', 'The elliptical “I do / we do” needs its question to judge the added Czech verb reliably.')}
for label, group in [('minor',minor),('major',major),('unjudgeable',unknown)]:
    for i, (cs,en) in group.items(): add([i],label,cs,en)
assert set(grades) == {r['id'] for r in rows}, (set(grades)-{r['id'] for r in rows}, {r['id'] for r in rows}-set(grades))
review = dict(reviewer='Codex, single AI reviewer; not independent human validation',
    sample_plan='quality-sample-plan.json', context='Logged English source, Czech output and selected adjacent logged lines. No reference video or verified reference translation.',
    rubric={
        'good':'No clear material error relative to the logged input. Not a guarantee of perfect grammar or correct OCR.',
        'minor':'Awkward wording, small omissions or nuance shifts; main proposition remains recognizable.',
        'major':'Material meaning change, invented proposition/reply, important omission, wrong idiom, or substantially untranslated content.',
        'unjudgeable':'Insufficient context or corrupted/fragmentary source prevents a defensible judgment.'},
    limitations=['One non-blinded AI reviewer; labels are subjective and all are inspectable.', 'Random sample of unique source/output pairs, not independent scenes or unique complete sentences.', 'Includes partial/revised captions and HUD contamination, not just dialogue.', 'Unknown gender, punctuation and stutter removal alone are not penalized.', 'No human reference, OCR ground truth, BLEU or claim of general translation accuracy.'],
    counts=dict(Counter(g['grade'] for g in grades.values())), rows=[dict(r,**{k:v for k,v in grades[r['id']].items() if k!='id'}) for r in rows])
(BASE/'quality-review.json').write_text(json.dumps(review,ensure_ascii=False,indent=2),encoding='utf-8')
print(review['counts'])

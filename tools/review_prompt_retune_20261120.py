"""Explicit labels for new dev outputs; profile names are absent from the review sheets.
Some known error examples were inspected with profile names earlier; this is not a fully blind or independent review.
"""
import json
from pathlib import Path
BASE=Path('analysis-output/prompt-retune-debug-20261120/dev-review')
GROUPS=[]
def batch(first,last,good,minor,major,unknown):GROUPS.append((first,last,good,minor,major,unknown))
batch(1,60,
 [3,8,9,10,11,12,14,15,17,18,19,21,23,24,25,26,27,28,29,30,32,33,35,39,40,41,42,43,44,47,50,52,53,54,55,56,57,58,59,60],
 {1:'Vynechává, že recenze má být špatná.',2:'Kostrbatě zdvojené hodnocení a doslovné too bad.',4:'Place zúženo na dům bez výslovného určení ve vstupu.',5:'Place zúženo na byt bez výslovného určení ve vstupu.',6:'Idiomatické nic jim neprojde posunuto na nedostanou se z toho lehce.',13:'Budoucí závazek změněn na chtění.',16:'Vynecháno Nice.',20:'What about her posunuto na co s ní.',22:'Doslovné bojuje místo má problémy.',36:'Zakázka od základny nepřesně na základnu.',37:'Kulečníkové solids nepřirozeně jako ploché barvy.',38:'Obecné nechytí je změněno na minulost.',45:'Vynechává důraz na skutečně pořádné peníze.',48:'Doplňuje sebe jako příjemce pomoci, kostrbaté vedlejší práce.',49:'Doplňuje sebe jako příjemce pomoci.',51:'Doslovné vzít to ven místo vyřídit venku (hovor).'},
 {7:'Připojuje in there k Jasonovi, ačkoli v okolním dialogu jde o pobyt mluvčí v koupelně.',31:'Obrací kladné uděláme párty na neuděláme.',34:'Podmínku až se příště rozhodneš změnilo na možnost že se rozhodneš krást.',46:'Přidává odpověď druhého mluvčího Jasně.'}, {})

batch(61,120,
 [61,64,65,69,75,76,77,78,79,80,81,83,84,85,86,87,88,89,90,91,92,93,95,97,99,101,102,103,105,108,110,111,116,119,120],
 {70:'Bring it in stále doslovné přinesení; gesto v logu není jisté.',71:'Bring it in stále nepřirozené přivedení.',72:'Tancování zachováno, ale kostrbaté je toho moc tance.',73:'Vynechává tancování.',74:'Rozkaz sáhnout si/cítit to se mění na oznamovací cítíš to.',82:'Pro osoby nevhodný tvar modely místo modelové/modelky.',98:'Step off the table v kontextu hry doslovně slézt ze stolu.',107:'Pořízení věci posunuto na její výběr; nevymýšlí učení jako baseline.',112:'HUD slovo LOOT ponecháno anglicky.',113:'Přimíchaný pokyn PICK UP nepřeložen.',114:'Dialog správný, HUD pokyn vynechán.',115:'Zřízení účtu změněno na najití; ženský rod zájmena místo účtu.',117:'U dálnice posunuto na mimo dálnici.',118:'U/sjezd z dálnice změněno na přímo na dálnici.'},
 {62:'Finanční a nájemní problémy změněny na majetek.',63:'Finanční a nájemní problémy změněny na majetek.',66:'Panáky chybně jako střílení.',67:'Panáky chybně jako střelba.',68:'Panáky chybně jako střílení.',94:'Porazit protivníka chybně jako vykuř ho.',96:'Necháš si to líbit chybně jako vezmeš si to.',100:'Idiom to je konec doslovně co napsala.',104:'Domyšlená odpověď Co?',106:'Pořízení věci změněno na naučení se.',109:'Domyšlená odpověď Ne.'}, {})

batch(121,184,
 [121,122,123,124,125,128,130,131,132,134,137,138,139,140,145,146,147,149,150,151,152,153,154,157,158,159,161,163,164,165,166,170,171,172,173,174,175,176,177,180],
 {126:'Doslovné dát peníze dovnitř a dotknout se jich; finanční omezení zůstává srozumitelné.',133:'Kostrbaté pro zbytek těch ostatních.',135:'Odkaz na večírek konkrétního bývalého vězně změněn na vězeňský mejdan.',136:'Odkaz na konkrétního bývalého vězně změněn na vězeňský mejdan; Huh nepřeloženo.',141:'Doslovné vzala loď a zdvojené výpustky.',148:'Společné pokud to uděláme změněno na tvoje chtění.',155:'Neosobní pokud bude potřeba mluvit změněno na jestli budeme mluvit.',160:'Místo shozu balíčků nepřesně jako místo výsadku.',162:'Vynechává Delta z lokalizace; přistání na vodě zachováno.',168:'Špatně se za někoho přimluvit je nepřirozené vyjádření možnosti ho poškodit.',178:'Nenech se jimi ztratit je vadná čeština místo neztrať se v nich.',179:'Nenech se jimi ztratit je vadná čeština místo neztrať se v nich.',183:'Množné trezory změněny na jednu bezpečnostní schránku.',184:'Nesprávný tvar nejčastější navštěvované, význam zachován.'},
 {182:'Místní název Key Lento změněn na klíč; mapové názvy potvrzuje okolí ID1205–1211.',127:'Z podmínky omezeného výběru peněz dělá jediné možné jednání.',129:'Úvěrová bonita změněna na úvěr.',143:'Skidmarks v čištění lodě chybně jako stopy po hlubokomořských skluzavkách.',144:'Skidmarks změněny na stopy po kluzkých plochách.',156:'Domyšlená odpověď Ne.',167:'Vynecháno Think again a význam přimluvení/vlivu je změněn na říkání slov.',169:'Já/my ano změněno na já vím a vypuštěno we do.',181:'Domyšlená odpověď Dobře.'},
 {142:'Směs VAPID damage a jména bez úplné věty/referentu.'})

def main():
    rows=json.loads((BASE/'new-candidates.json').read_text(encoding='utf-8'));grades={}
    for first,last,good,minor,major,unknown in GROUPS:
        current={}
        for kind,group in [('good',{i:'Smysl logovaného vstupu zachován.' for i in good}),('minor',minor),('major',major),('unjudgeable',unknown)]:
            for i,note in group.items():
                key=f'P{i:03}';assert key not in grades and key not in current
                current[key]=dict(grade=kind,note_cs=note)
        assert set(current)=={f'P{i:03}' for i in range(first,last+1)}
        grades.update(current)
    result=[dict(r,**grades[r['candidate']]) for r in rows if r['candidate'] in grades]
    (BASE/'new-grades.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Explicit new judgments',len(result),'/',len(rows))
if __name__=='__main__':main()

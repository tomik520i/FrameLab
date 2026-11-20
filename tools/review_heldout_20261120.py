"""Explicit assessment of every new validation output, with profile names hidden.
This remains one subjective AI reviewer; it is not independent human validation.
"""
import json
from pathlib import Path
P=Path('analysis-output/prompt-retune-debug-20261120/heldout-review');GROUPS=[]
def batch(first,last,good,minor,major,unknown):GROUPS.append((first,last,good,minor,major,unknown))
batch(1,74,
 [1,2,3,5,6,8,9,11,12,14,18,20,21,22,23,24,25,27,30,31,32,34,36,37,38,40,42,43,44,46,47,48,49,50,51,52,55,56,57,58,60,62,66,67,70,71,72],
 {4:'Důrazové alright doslovně v pořádku.',7:'Chytit místo vyzvednout balíček.',10:'Mama jako mami je v barovém oslovení ženy nepřesné.',16:'Vynecháno, že jde o limity transakcí.',19:'Budoucí neukážeš změněno na nechceš a přidává neurčené je.',26:'Vynechává upřesnění krádeže v obchodě.',28:'Crew jako posádka a quite jako početná zužuje význam party.',39:'Vypouští vulgární důraz.',41:'Vypouští vulgární důraz, hlavní smysl správný.',45:'Pořízení auta zúženo na koupi.',61:'Kokain neurčitě jako fet, čas about to zjednodušen na chce.',64:'V letecké scéně jdi místo leť/vrať se.',65:'Department zde úřad, ministerstvo příliš specifické.',68:'Pokyn prodej změněn na oznamovací prodávám.',69:'Deal neurčitě jak to chodí, čas by the time posunut na než.',73:'Vynecháno nahoru.'},
 {13:'Domyšlená odpověď Já vím.',15:'Domyšlená odpověď Já nechci.',17:'Zlobení přístroje za počasí změněno na roli v počasí.',29:'Mobilní signál změněn na služby.',35:'Zamířit/srovnat na úder změněno na seřadit střelu.',53:'Doplňuje vymyšlený účel aby vám koupili.',54:'Práce kterou mohou nabídnout změněna na práci, kterou dostali.',59:'Domyšlená ztráta paměti a nákupy; shozy změněny na kapky.',63:'Lidé, od kterých se vyzvedává zásilka, změněni na vyzvedávané osoby.',74:'Příchod Andyho změněn na dosažení úspěchu.'},
 {33:'Námořnická hláška bez obrazového referentu nemá jistý doslovný/přenesený význam.'})
batch(75,117,
 [76,78,79,80,82,84,86,89,90,91,92,94,95,96,98,102,103,105,107,108,110,112,113,114,115,116,117],
 {75:'V odpovědi na směr chybí k vodě.',77:'Vynechána otázka Move a kostrbaté zdvojení teď.',81:'Nepřirozené máš to správné co říct.',83:'Cloud seeding nepřirozeně jako rozsévání mraků.',85:'Nepřirozené program propuštění.',87:'Ujišťovací otázka změněna na prosté přitakání.',93:'Praní/bitevní chování zúženo na povolené zápasy.',97:'Vynecháno Simple as that a oslabené pretty.',99:'Ztrácí vulgární důraz.',100:'Oblečení nepřesně jako šaty.',101:'Přidává minulost a byl, které nejsou výslovné.',104:'Struggling doslovně bojuje místo má problémy.',106:'Otázka na spuštění alarmu změněna na zda jej slyší.',111:'Přidává dneska, které ve vstupu není.'},
 {88:'Domyšlená odpověď Miluju.',109:'Matka tvého dítěte změněna na tvoji matku.'}, {})

batch(118,147,
 [118,120,121,123,124,125,126,128,129,130,132,133,137,138,139,140,141,143,146,147],
 {122:'Hacknutí vynecháno, pokyn jít změněn na společné zajdeme.',127:'Pořízení věci zúženo na koupi.',134:'Chytit a výsadky místo vyzvednout zásilku a provést shozy.',136:'Dialog přeložen, přimíchaný pokyn SIT vynechán.',142:'Nepřirozené na prolet, změna oslovení na vykání.',144:'Vynechává přivlastnění your lady.',145:'Z neosobně investovaných peněz dělá výslovnou investici mluvčího.'},
 {119:'Pokračování o ptácích změněno na rozkaz posluchači ujistěte se.',131:'Odjezd/odhlášení nájemníka změněno na pronajmutí bytu.'},
 {135:'He up je neúplný fragment s neurčitým významem.'})

batch(148,175,
 [149,151,155,158,159,160,161,163,164,168,169,171,172,173,174,175],
 {150:'Dials doslovně jako číselníky.',153:'U ženy z předchozího dialogu mění rod na mužský jeden; referent je v sousedním řádku.',154:'Kulečníkové plné koule chybně jako ploché vzory.',156:'Zakázka od letecké základny nepřesně na základnu.',157:'Vynechává místní vazbu kde mám být.',162:'Good one po vtipu posunuto na pochvalu práce.',165:'And all doslovně a všechno.',166:'Nepřirozené vzpomínat vzdáleně.',167:'Aligátoři změněni na krokodýly.',170:'Těsný tanec u někoho nepřirozeně na mě.'},
 {148:'Změna banky změněna na fyzické přestěhování.',152:'Volba vpravo na seznamce a idiom looks like work přeloženy doslovně správně a práce.'}, {})

batch(176,206,
 [177,181,184,186,187,190,191,192,194,196,197,198,199,200,201,202,203,205,206],
 {179:'Míchá budoucí a přítomný čas, hlavní smysl zachován.',180:'Špatný stav auta změněn na obecnou nadávku.',182:'Stick ve vtipu o páce chybně lepkavý.',185:'Idiom o snášení ponižování doslovně jíst hovna.',188:'Kostrbaté čtvrťáková mince; hlavní pokyn správný.',189:'Přidává neurčené množné jim.',193:'Z toho co říkáš změněno na co jsem slyšela, mírný posun.',204:'Nasazení pout z kontextu chybně jako oblečení.'},
 {176:'Dokončí fragment vymyšlenou radou zkusit něco jiného.',178:'Požadavek bankéři na bankovní úkon změněn na jdi do banky.',183:'Nelíbila se mi představa, že tam je sama změněno na nechtěla jsem ji nechat samotnou.',195:'Lidé s kontakty změněni na nesmyslný řetězec spojení.'}, {})

batch(207,219,
 [207,208,209,210,211,212,214,215,217,218,219],
 {216:'Keep our heads down doslovně držet hlavy dole místo nevyčnívat.'},
 {213:'Swipování na seznamce změněno na kreditní karty.'}, {})

batch(220,245,
 [220,221,222,223,224,228,229,233,235,236,237,238,240,241,242,243],
 {225:'U piva nesprávný ženský rod jednu.',226:'Vynechává povinnou pracovní četu/detail.',227:'Být spolu ve vězení doslovně jako uvnitř.',231:'Vynechává early, časné působení špatného vlivu.',232:'Velké/pořádné tacos nepřirozeně tlusté.',234:'Coming right up doslovně se blíží místo hned přijde na řadu.',239:'Nasazení pout z kontextu jako oblečení.',244:'Změkčuje vulgární důraz.',245:'Přistávání změněno na přilétání.'},
 {230:'Kulečník chybně jako bazén.'}, {})

batch(246,267,
 [246,247,249,252,253,254,255,256,259,260,262,263,264,265],
 {251:'HUD pokyn PICK UP LOOT zůstává anglicky; dialog přeložen.',257:'Právě mě ubytovává změněno na budoucí ubytuje a přidává účel.',267:'Pravopis spořící místo spořicí.'},
 {248:'Slang on one chybně jako na jednom místě.',250:'Kulečníkový faul scratch změněn na poškrábání.',258:'Swipování změněno na nesrozumitelné proplácání.',261:'Snížení výšky letu změněno na snížení ceny.',266:'Kulečníkové mantinely Rails změněny na jméno Railsi.'}, {})

batch(268,292,
 [268,269,270,272,274,275,278,279,280,281,282,283,284,286,288,291],
 {276:'Nezvládl jsem to nepřesně jako nemohl jsem.',277:'Hugger neobratně jako objímač.',285:'Začít pracovat posunuto na jít do práce.',290:'Rozlučkové take it easy nepřesně jako uklidněte se.',292:'Vypouští vulgární důraz.'},
 {271:'Makléř změněn na jiného člověka od makléře.',287:'Hození/podání zásilky změněno na odhození pryč.',289:'HAT změněno na nesmyslnou klubovku.'},
 {273:'Nejasný fragment Dr yo bez ověřitelného významu.'})

batch(293,315,
 [294,295,296,297,300,302,303,305,306,307,308,309,311,313,314],
 {293:'Vypouští vulgární důraz.',298:'Nutnost vyzvednout změněna na možnost společného vyzvednutí.',301:'Mousy hair chybně myší vlasy; převod výšky na 160 cm je správný.',304:'Neobratné dostanu se z šatů místo převléknu se.',310:'Přidává dlouho, které není výslovné.',312:'Whoops ponecháno anglicky.'},
 {299:'Zápor předchozího zlomení nosu mění na nekoupil.',315:'Ověření identity jsi to ty změněno na štve tě to.'}, {})

batch(316,347,
 [316,318,321,322,323,324,326,327,329,331,332,333,334,335,336,337,339,341,342,344,345],
 {319:'U balíčku nepřirozené šel místo spadl/trefil se.',320:'Několik účtů změněno na jeden.',325:'Neurčitá dobrá situace zúžena na dobrou práci.',338:'Rails zůstává anglicky místo kulečníkových mantinelů, zbytek věty přeložen.',340:'Měl jsem tě vyzvednout změněno na šel jsem pro tebe.',343:'Rozkaz řídit změněn na oznamovací řídíš.',346:'Pozdrav hey there chybně místně hej tady.',347:'Přítomné nevrže změněno na minulost.'},
 {317:'Zkusit se dostat do týmu Narcos změněno na pracovat u narkotiků.',328:'Špatná úvěrová bonita změněna na urážku osoby debil.',330:'Domyšlená odpověď Nevím.'}, {})

batch(348,363,
 [348,350,351,352,353,354,357,360,362,363],
 {349:'Odpovědnost za událost změněna na vlastnictví bylo to tvoje.',356:'Zájem o něco neobratně jako byl na.',361:'Založení účtu neobratně jako domluvit jednu; navíc chybný rod referentu.'},
 {355:'Lidé s kontakty změněni na nesmyslné spojené lidi.',358:'Název služby FruitFace a videohovor změněny na ovocné přivítání.',359:'Úvěrová bonita změněna na placení na tvůj účet.'}, {})

batch(364,416,
 [365,367,368,369,370,372,374,375,377,380,381,382,383,385,386,387,388,389,390,391,392,393,394,396,400,402,403,404,405,408,409,410,411,415,416],
 {366:'Nepříjemný zápach kuliček proti molům jako voní.',371:'Taneční pohyby neurčitě jako triky.',376:'Chytit a výsadky místo vyzvednout zásilku a provést shozy.',379:'V taneční scéně doslovné co máš místo co umíš.',398:'Odchod do doby zjištění posunut na odchod při zjištění.',399:'Skupina Kingfishers změněna na jednoho Kingfishera.',412:'Glamour jako zábava, místo půvabu/luxusu.'},
 {364:'Dohlížející úředník naléhá změněno na vedoucí sleduje.',373:'Uzavření záležitosti změněno na odložení k ledu.',378:'Piloti z navazujícího příběhu zaměněni za brýle.',384:'Nemám účet změněno na nemám rád hotovost, se zcela domyšlenou větou.',395:'Omluva za něčí chování změněna na lítost nad ním.',397:'Úvěrová bonita změněna na úvěr.',401:'Zajišťuji Jasonovi výdělek změněno na placení za jeho zadek.',406:'Drogová dávka eightball změněna na samotnou malou osmičku; význam slovní hříčky mizí.',407:'Vtip že je Donnie sjetý změněn na vlajícího Donnieho.',413:'Souhlas/slang jdu do toho změněn na jsem na dně.',414:'Vypadat cool změněno na vypadat nemocně.'}, {})

def main():
 rows=json.loads((P/'new-candidates.json').read_text(encoding='utf-8'));grades={}
 for first,last,good,minor,major,unknown in GROUPS:
  current={}
  for grade,items in [('good',{i:'Smysl logovaného vstupu zachován.' for i in good}),('minor',minor),('major',major),('unjudgeable',unknown)]:
   for i,note in items.items():
    key=f'H{i:04}';assert key not in grades and key not in current
    current[key]=dict(grade=grade,note_cs=note)
  assert set(current)=={f'H{i:04}' for i in range(first,last+1)},(first,last)
  grades.update(current)
 result=[dict(r,**grades[r['candidate']]) for r in rows if r['candidate'] in grades]
 (P/'new-grades.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 print('Explicit validation judgments',len(result),'/',len(rows))
if __name__=='__main__':main()

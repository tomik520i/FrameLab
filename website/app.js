'use strict';
const cs = {
  skip:'Přejít na obsah',navEvidence:'Jak to vypadá',navSetup:'Jak začít',navSupport:'Podpora',download:'Stáhnout',
  hero1:'ZAPOJ SVOJI RTX',hero2:'DO OBRAZU.',heroDescription:'Pošli obraz ze střihovky do PC. Nech NVIDIA GPU dopočítat mezisnímky. Hraj přes FrameLab na svém monitoru.',heroCta:'Stáhnout hotovou aplikaci',heroNote:'Zdarma · přenosná aplikace pro Windows x64 · EN / CZ',origin:'Konzole, střihovka a RTX v počítači. Začalo to myšlenkou na GTA VI: mohla by ta grafika udělat hraní ve 30 FPS plynulejší?',heroVideo:'ORIGINÁL → 2× VFG / ZPOMALENÍ 4×',inspect:'Prohlédnout porovnání ↗',tagNote:'Začni na 2×. Pak zkoušej dál.',pipe1:'ZACHYTIT',pipe1sub:'Konzole, druhé PC, zdroj videa',pipe2:'DOPOČÍTAT',pipe3:'ZOBRAZIT',pipe3sub:'Celá obrazovka + zvukový výstup',
  evidenceLabel:'01 / UKÁZKY A DŮKAZY',evidenceTitle:'Míň dohadů.<br>Víc porovnání.',evidenceIntro:'Skutečné nahrávky. Originály vedle generovaných snímků. Zpomal si to a sleduj hrany, HUD i místa, kde se dopočet rozbije.',balanced:'ZÁKLAD',combat:'SOUBOJ',stress:'ZÁTĚŽOVÝ TEST',videoTiming:'3 s ZDROJE → 12 s PŘEHRÁVÁNÍ',inputChanges:'Změny vstupního obrazu/s',outputChanges:'Známé změny RGB výstupu/s',capturePixels:'Přijaté rozlišení',quality:'Kvalita / VSync',submitted:'Odeslaná ID obrazů/s',latency:'Příjem RAW → odeslání',fullAnalysis:'Přečíst kompletní analýzu ↗',evidenceZip:'Stáhnout snímky + nastavení + klipy ↓',measurementNote:'Jde o různé průchody hrou, rozlišení a nastavení, nikoli kontrolované pořadí násobičů. Čísla popisují obrazy v aplikaci, ne FPS monitoru změřená kamerou. Zarovnaná videa odstraňují zpoždění zpracování; verze se skutečným časováním jsou v ZIPu. Latence nezahrnuje střihovku, render hry ani displej.',
  pixelLabel:'SNÍMKY. VČETNĚ TĚCH NEPOVEDENÝCH.',pixelTitle:'Víc snímků může<br>znamenat i duchy nohou.',pixelBody:'Jeden kop, dva skutečné vstupy, dvě dopočítané polohy. Postava se posune, ale zůstanou po ní průsvitné končetiny. Rozmazání pohybu a částice zásahu jsou už v originálu; přidané obrysy jsou chyba interpolace.',nativePixels:'Otevřít výřezy v původním měřítku ↗',expand:'Kliknutím zvětšit ↗',gif2:'Originál vs. 2× — GIF zpomalený 6×',gif3:'Originál vs. 3× — GIF zpomalený 6×',saveGif:'Uložit GIF ↓',
  uiLabel:'02 / VLASTNÍ ŘÍDICÍ PULT',uiTitle:'Sleduj, co tvoje<br>grafika počítá.',uiIntro:'Zdroj, rozlišení a vstupní FPS vybíráš samostatně. Při hraní sleduj měnící se vstup, generovaný výstup, zpoždění fronty a vytížení GPU.',screenshotNote:'Rozhraní preview.9 při přehrávání nahraného RAW obrazu. Čísla z této ukázky nejsou novým měřením výkonu při hraní. Aplikace přepíná mezi češtinou a angličtinou.',featureAudio:'Výběr zvukového výstupu',featureCapture:'Režimy dostupné podle ovladače',featureRecord:'15sekundový diagnostický záznam',featureCrops:'Porovnání RAW / generovaný / RAW',
  downloadLabel:'03 / MÍŇ NASTAVOVÁNÍ. VÍC ZKOUŠENÍ.',downloadTitle:'Rozbal.<br>Zapoj.<br><span class="acid">Zkus to.</span>',downloadIntro:'Kompletní preview.13 obsahuje NVIDIA VFG, obě herní ukázky, lokální české titulky včetně modelu, exportér a adaptér NR. Rozbal a spusť. Pro NR stačí průvodce externím stažením a importem níže.',nrSetup:'2 · Doplnit enhancer — přesný download a import ↗',nrSetupNote:'Externí komunitní ZIP: dalších 731 MB. NR DLL nejsou přibalené; odkaz ani import licenci neuděluje.',fullDownload:'Stáhnout kompletní FrameLab',requirements:'Windows 10/11 x64, ovladače NVIDIA GPU a střihovky; pro překlad anglické OCR Windows. Začni 1080p, Střední a 2×. Experimentální aplikace bez podpisu.',checksums:'Kontrolní součty souborů ↗',nvidiaTitle:'RTX 40 a RTX 50',nvidiaBody:'Oficiální nezměněný NVIDIA VFG runtime pro Ada/Blackwell (RTX 40/50). Ověřeno na RTX 5070 Ti a RTX 4050 Laptop. Adaptér NR je přibalený; externí modelové DLL doplníš podle návodu.',opticalTitle:'Alternativa pro starší RTX',opticalBody:'Vestavěné GPU hledání pohybu a posouvání obrazu. Jiný algoritmus a jiná kvalita. Zamýšlené i pro starší RTX; hardware řad 20/30 zde zatím nebyl otestovaný.',otherDownloads:'Aplikaci už máš?',addon:'NVIDIA runtime doplněk pro FrameLab',sourceZip:'Odpovídající zdrojové kódy',tuningZip:'Testy kvality a odezvy',allVersions:'Všechna vydání na GitHubu',licenseNote:'Zdrojový kód FrameLabu: GPL-3.0. NVIDIA soubory mají vlastní podmínky, přiložené v balíčku. Projekt není spojený s NVIDIA ani jí schválený.',licenses:'Licence součástí a původ souborů ↗',
  setupLabel:'04 / PRVNÍ OBRAZ VE TŘECH KROCÍCH',setupTitle:'Ze ZIPu na monitor.',setupIntro:'Konzole je jen jeden ze zdrojů. FrameLab přijme kompatibilní capture zařízení přes DirectShow a zpracuje jeho snímky na NVIDIA GPU.',step1:'Rozbal celý ZIP.',step1body:'Nech všechny složky pohromadě na místě, kam můžeš zapisovat. FrameLab nepotřebuje instalátor ani instalaci jako správce.',step2:'Vyber spouštěč.',step2body:'Otevři 1 - Spustit FrameLab.cmd a uvidíš přibalené CS:GO. RTX 40/50: 2 - NVIDIA RTX 40-50.cmd. Alternativa: 3 - Optical Flow.cmd. Oba FG spouštěče začínají na 2×.',step3:'Vyber zdroj. Spusť ho.',step3body:'Ve Vstupu zkus Spider-Mana, test titulků, nebo vyber střihovku a Spustit vstup. Začni automaticky nebo 1080p. F11 otevře celou obrazovku.',rigLabel:'SKUTEČNĚ TESTOVÁNO NA',rigLink:'Testovací sestava a živé vytížení GPU ↗',
  supportLabel:'EXPERIMENT OD PRDELNIOPICE',supportTitle:'Líbí se ti ten pokus?<br>Pomoz mu dál.',supportBody:'FrameLab je zdarma. Pokud chceš podpořit další testování a vývoj, můžeš přispět přes můj portál Opicebot.',donate:'Podpořit přes Opicebot',donateNote:'CZK přes PayPal · Darovací stránka je jen česky · Dobrovolné',githubPoster:'OTEVŘI<br>ZDROJÁKY.',footer:'Vstup ze střihovky. Prostor pro pokusy. © 2026 FrameLab contributors.',footerLicenses:'Licence',issues:'Nahlásit problém ↗',lightboxNote:'Obrázek v plném rozlišení. Pro detaily použij zoom prohlížeče. Escape zavře náhled.'
};
cs.visualGuide='1 · Stáhnout, rozbalit a spustit — obrazový návod ↗';
cs.guideRevision='Preview.13: automaticky čistý fullscreen, ostřejší české titulky, tažení dělicí čáry a obrazové návody v ZIPu.';
cs.fullscreenGuide='3 · Čistý fullscreen: monitor, efekty a návrat do nastavení ↗';
cs.installGuide='Kompletní návod k instalaci (CZ / EN) ↗';
Object.assign(cs, {
  "faqHeading": "Než začneš",
  "faqIntro": "Instalace, efekty a běžné používání — pro preview.13.",
  "rigLink": "Obě sestavy a rozsah testování ↗",
  "rigDesktop": "STOLNÍ PC / ŽIVÝ VSTUP",
  "rigLaptop": "NOTEBOOK / LOKÁLNÍ UKÁZKY",
  "rigDesktopScope": "PS5 přes AVerMedia GC573. Živé testy NVIDIA VFG a měření vytížení GPU.",
  "rigLaptopScope": "VFG, experimentální NR, čistý fullscreen a lokální OCR → čeština na přibalených videích a testovacích obrazech. Bez nového měření vstupu z PS5.",
  "faq1": "Co mám stáhnout a jak FrameLab spustím?",
  "faq1body": "Stáhni kompletní ZIP preview.13, rozbal ho celý do krátké cesty, například C:\\FrameLab, a otevři „1 - Spustit FrameLab.cmd“. První start přehraje přibalenou ukázku. V balíku jsou obě herní videa, NVIDIA VFG, lokální překladač s modelem a obrazové návody. Potřebuješ funkční ovladač GPU; pro střihovku také její ovladač. Další krok pro enhancer je popsaný níže.",
  "faq1link": "Obrazový návod ke spuštění ↗",
  "faq2": "Je enhancer v balíku? Jak ho zapnu?",
  "faq2body": "Přibalený je adaptér a průvodce nastavením. Modelové NR DLL se stahují zvlášť z externího zdroje a importují podle návodu. Potom v AI / NR enhancer zapni úpravu, nastav sílu a v okně ji porovnej s originálem. Jde o experiment nad hotovým obrazem, ne o oficiální integraci DLSS 5 do hry. Externí soubory mají vlastní licenční podmínky; samotný import oprávnění k jejich použití ani šíření nenahrazuje.",
  "faq2link": "Stažení a import enhanceru ↗",
  "faq3": "Můžu používat frame gen, enhancer a české titulky současně?",
  "faq3body": "Ano, jednotlivé funkce můžeš zapínat nezávisle a používat společně. Frame gen přidává mezisnímky, enhancer upravuje obraz a české titulky se kreslí přes výsledné zobrazení. S více efekty roste zátěž i zpoždění. Začni na 1080p a 2×; pak přidej enhancer a titulky a zkontroluj plynulost na své sestavě.",
  "faq4": "Jak dostanu na celou obrazovku jen výsledný obraz?",
  "faq4body": "Efekty a cílový monitor nastav v aplikaci a stiskni F11. Fullscreen automaticky skryje porovnání, dělicí čáru i ovládací panely a ukáže jeden výsledný obraz s vybranými efekty a případnými českými titulky. Myš se po 2 sekundách nečinnosti schová. Escape nebo F11 vrátí nastavení; původní porovnání zůstane připravené v okně.",
  "faq4link": "Obrazový návod pro fullscreen ↗",
  "faq5": "Jak fungují české titulky a jak jsou spolehlivé?",
  "faq5body": "V nastavení Titulky zapni lokální překlad a vyber oblast, kde hra zobrazuje anglický text. Čte se obraz, ne zvuk ani mikrofon. Model je v balíku; Windows potřebuje anglické OCR. Po jeho doplnění překlad funguje offline. Kvalita je experimentální: test 300 textů odhalil vynechané věty i chybnou herní terminologii. Malý nebo málo kontrastní text se může ztratit. Oblast omez na titulky, aby se nepřekládal i HUD.",
  "faq6": "Co čekat od frame genu a počítadla FPS?",
  "faq6body": "NVIDIA režim používá Maxine Video Frame Generation, nikoli DLSS Frame Generation integrované ve hře. 2× / 3× / 4× určuje požadovaný dopočet; skutečný výsledek závisí na zdroji a výkonu. Vyšší číslo nezrychlí herní logiku ani ovládání a může přidat deformace či zpoždění. FPS v aplikaci popisuje její zpracování a odesílání snímků, ne nezávislé změření fyzického monitoru.",
  "faq7": "Co zkontrolovat, když ze střihovky nejde obraz?",
  "faq7body": "Nejdřív spusť přibalené video. Pokud funguje, ověř HDMI signál v aplikaci výrobce karty, správný zdroj a dostupný režim ve Vstupu. Zavři další aplikace používající stejnou střihovku a zkus Automaticky nebo 1080p. Nabízená rozlišení a FPS závisí na ovladači zařízení; režim musí podporovat karta i připojený zdroj.",
  "faq8": "Proč záznam potřebuje hodně paměti a nemá zvuk?",
  "faq8body": "Současný záznam slouží k rozboru kvality snímků. Ukládá plné obrazy do RAM a při exportu na disk, takže ve vysokém rozlišení rychle roste spotřeba paměti i velikost souborů. Nahrávání může snížit výkon a zvuk do diagnostického záznamu neukládá. Pro běžné sledování nech záznam vypnutý."
});
Object.assign(cs, {
  "subtitleTitle": "České titulky.<br>Tvoje GPU. Tvůj obraz.",
  "subtitleIntro": "Přečti anglický dialog přímo z obrazu, přelož ho na tomto PC a umísti češtinu nezávisle. Bez mikrofonu a cloudového překladu.",
  "subtitleOcrTitle": "Když chybí, stačí tlačítko.",
  "subtitleOcrBody": "FrameLab dostupnost anglického OCR ověří. Doplnit anglické OCR stáhne součást Windows od Microsoftu po potvrzení správce. Jazyk Windows se nemění; rozpoznávání pak funguje offline.",
  "subtitleModelTitle": "Vyber model. Stáhni přímo v aplikaci.",
  "subtitleModelBody": "CPU překladač je přibalený. Volitelné CUNI Q3 (3,52 GB) a Q8 (7,70 GB) se stahují s pokračováním a kontrolou souborů. První GPU model doplní přenosný runtime (1,62 GB). Bez účtu a instalování Ollamy.",
  "subtitlePlaceTitle": "Nad. Pod. Nebo přetáhnout.",
  "subtitlePlaceBody": "Posuň český text i pozadí bez změny oblasti čtení. Více řádků zůstane uvnitř viditelného obrazu, včetně ořezu a fullscreenu. Dlouhý text se zmenší, aby se vešel.",
  "subtitleLimits": "OCR může minout malý nebo krátce viditelný text; překlad se může mýlit a přidává zpoždění. Volná VRAM se kontroluje při zapnutých efektech. Vyšší přesnost vah nezaručuje lepší češtinu.",
  "guideRevision": "Preview.13: stahování modelů CUNI, doplnění OCR jedním tlačítkem, přetahování češtiny a ochrana okrajů viditelného obrazu.",
  "downloadIntro": "Kompletní preview.13 obsahuje NVIDIA VFG, obě herní ukázky, CPU překladač, exportér a adaptér NR. Rozbal a spusť. Výchozí angličtinu přepneš tlačítkem do češtiny. Volitelné CUNI GPU modely stáhne aplikace.",
  "faq5body": "Otevři Titulky a ověř dostupnost OCR. Pokud chybí, Doplnit anglické OCR ho stáhne od Microsoftu po potvrzení UAC. CPU model je přibalený, CUNI Q3/Q8 jsou volitelné downloady přímo v aplikaci. Vyber jen pruh anglických titulků. Češtinu umísti nad/pod něj nebo přetáhni zvlášť; více řádků zůstane ve viditelném obrazu. Rozpoznávání a překlad jsou experimentální a přidávají zpoždění. Zvuk se nepřepisuje a překlad nejde do cloudu."
});
Object.assign(cs, {
  "localVideo0": "Tvoje video. Tvoje nastavení.",
  "localVideo1": "Otevři místní film se zvukem, pozastav ho, přetoč nebo opakuj. Pak zkus FG, enhancer a OCR titulky. Střihovku nepotřebuješ. NR má nově 1–4 navazující průchody; začni jedním a porovnávej.",
  "localVideo2": "Ověřeno SDR MP4 H.264/AAC. Další kodeky závisí na Windows. Více průchodů NR může zesílit artefakty i zpoždění; nejde o násobič frame genu.",
  "localVideo3": "Návod k přehrávání (EN / CZ) ↗"
});
const en = Object.fromEntries([...document.querySelectorAll('[data-i18n]')].map(el=>[el.dataset.i18n,el.innerHTML]));
const takes = {
  '2x':{id:'82183953',resolution:'2560 × 1440',quality:'Medium',vsync:false,input:'40.0',output:'79.0',submitted:'80.0',latency:'45.36 ms',title:['The 2× baseline.','Základ ve 2×.'],description:['Extra positions between originals. The practical starting point, with occasional edge and HUD errors.','Nové polohy mezi originály. Rozumný začátek, s občasnými chybami hran a HUDu.']},
  '3x':{id:'112947656',resolution:'1920 × 1080',quality:'Medium',vsync:false,input:'40.0',output:'105.6',submitted:'110.8',latency:'48.96 ms',title:['A third step in the motion.','Tři kroky v pohybu.'],description:['Both generated phases are present. Foliage occlusion creates ghosted limbs; small HUD ticks warp.','Obě generované fáze jsou přítomné. Při překrytí listím vznikají duchové končetin a drobné značky HUDu se kroutí.']},
  combat:{id:'113768562',resolution:'2560 × 1440',quality:'Medium',vsync:true,input:'39.8',output:'89.87',submitted:'90.6',latency:'64.16 ms',title:['Combat makes it obvious.','Souboj to ukáže naplno.'],description:['Fast kicks, impacts and camera turns. Real extra positions, but also ghost legs and a distorted aiming reticle.','Rychlé kopy, zásahy a otáčení kamery. Skutečné nové polohy, ale také duchové nohou a deformovaný zaměřovač.']},
  '4x':{id:'84744531',resolution:'2560 × 1440',quality:'Medium',vsync:false,input:'39.73',output:'83.8',submitted:'87.87',latency:'77.84 ms',title:['Four times is a stress test.','Čtyřnásobek je zátěžový test.'],description:['Earlier scheduler, different gameplay. Severe silhouette and background distortion. More requested frames do not guarantee a better result.','Starší plánování, jiný průchod hrou. Výrazné deformace postavy a pozadí. Vyšší násobič nezaručuje lepší výsledek.']}
};
let language='en';let selected='2x';let manualLanguage=false;
try{const saved=localStorage.getItem('framelab-language');if(saved==='cs'||saved==='en'){language=saved;manualLanguage=true}}catch{}
function updateTake(changeVideo=false){
  const take=takes[selected],c=language==='cs'?1:0;
  for(const [id,value] of Object.entries({'take-id':'TAKE / '+take.id,'take-title':take.title[c],'take-description':take.description[c],'input-rate':take.input,'output-rate':take.output,'take-resolution':take.resolution,'take-quality':take.quality+' / '+(c?(take.vsync?'zapnuto':'vypnuto'):(take.vsync?'on':'off')),'take-submitted':take.submitted,'take-latency':take.latency}))document.getElementById(id).textContent=value;
  document.getElementById('take-report').href=`https://github.com/tomik520i/FrameLab/blob/main/examples/${take.id}/README${c?'.cs':''}.md`;
  document.getElementById('take-zip').href=`downloads/FrameLab-${take.id}-comparison-example.zip`;
  document.getElementById('video-download').href=`media/${selected}-slow.mp4`;
  if(changeVideo){const video=document.getElementById('comparison-video');video.pause();video.poster=`media/${selected}-poster.webp`;video.querySelector('source').src=`media/${selected}-slow.mp4`;video.load()}
  for(const tab of document.querySelectorAll('[data-take]')){const active=tab.dataset.take===selected;tab.setAttribute('aria-selected',String(active));tab.tabIndex=active?0:-1}
  document.getElementById('take-panel').setAttribute('aria-labelledby','tab-'+selected);
}
function setLanguage(lang,persist=false){language=lang;document.documentElement.lang=lang;const dictionary=lang==='cs'?cs:en;
  document.querySelectorAll('[data-i18n]').forEach(el=>{if(dictionary[el.dataset.i18n]!==undefined)el.innerHTML=dictionary[el.dataset.i18n]});
  const button=document.getElementById('language');button.textContent=lang==='cs'?'EN':'CZ';button.setAttribute('aria-label',lang==='cs'?'Switch to English':'Přepnout do češtiny');
  if(persist){manualLanguage=true;try{localStorage.setItem('framelab-language',lang)}catch{}}
  updateTake();
}
document.getElementById('language').addEventListener('click',()=>setLanguage(language==='en'?'cs':'en',true));
const tabs=[...document.querySelectorAll('[data-take]')];
tabs.forEach((tab,index)=>{tab.addEventListener('click',()=>{selected=tab.dataset.take;updateTake(true)});tab.addEventListener('keydown',event=>{let next;if(event.key==='ArrowRight')next=(index+1)%tabs.length;if(event.key==='ArrowLeft')next=(index+tabs.length-1)%tabs.length;if(event.key==='Home')next=0;if(event.key==='End')next=tabs.length-1;if(next!==undefined){event.preventDefault();tabs[next].focus();tabs[next].click()}})});
document.querySelectorAll('.gif-row details').forEach(details=>details.addEventListener('toggle',()=>{const img=details.querySelector('img');if(details.open)img.src=img.dataset.lazySrc;else img.removeAttribute('src')}));
const lightbox=document.getElementById('lightbox');let opener;
document.querySelectorAll('[data-image]').forEach(button=>button.addEventListener('click',()=>{opener=button;const image=lightbox.querySelector('img');image.src=button.dataset.image;image.alt=button.querySelector('img').alt;lightbox.showModal()}));
lightbox.querySelector('button').addEventListener('click',()=>lightbox.close());
lightbox.addEventListener('click',event=>{if(event.target===lightbox)lightbox.close()});
lightbox.addEventListener('close',()=>{lightbox.querySelector('img').removeAttribute('src');opener?.focus()});
document.querySelectorAll('video').forEach(video=>video.addEventListener('play',()=>{document.querySelectorAll('video').forEach(other=>{if(other!==video)other.pause()})}));
setLanguage(language);
if(!manualLanguage)fetch('locale.php',{cache:'no-store',signal:AbortSignal.timeout(2000)}).then(r=>{if(!r.ok)throw new Error('locale');return r.json()}).then(data=>{if(!manualLanguage)setLanguage(['CZ','SK'].includes(data.country)?'cs':'en')}).catch(()=>{});
fetch('downloads/catalog.json?v=preview13').then(r=>{if(!r.ok)throw new Error('catalog');return r.json()}).then(c=>{const full=c.files.find(f=>f.name==='FrameLab-0.2.0-preview.13-windows-x64-full.zip');if(full)document.querySelector('.download-size').textContent=`Windows x64 · ${(full.bytes/1000000).toFixed(0)} MB · preview.13`}).catch(()=>{});

// Run after the native batch, so browser QA does not consume its GPU/CPU budget.
const {chromium}=require(process.env.FRAMELAB_PLAYWRIGHT_PATH||'../../web-tests/node_modules/playwright-core');
const fs=require('fs'),path=require('path'),assert=require('assert');
const experiment=path.resolve('../../outputs/context75-2026-11-24');
assert(JSON.parse(fs.readFileSync(path.join(experiment,'progress.json'))).completed,'Finish native measurements before browser QA');
const url=process.argv[2]||'http://127.0.0.1:18777/runs/2026-11-24-context75/';
const out=path.join(experiment,'web-qa');fs.mkdirSync(out,{recursive:true});
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
try{const page=await browser.newPage({viewport:{width:1440,height:1000}}),errors=[],failed=[];
page.on('pageerror',e=>errors.push(e.message));page.on('response',r=>{if(r.status()>=400)failed.push({url:r.url(),status:r.status()});});
await page.route('**/locale.php',r=>r.fulfill({contentType:'application/json',body:JSON.stringify({country:'US',language:'en'})}));
await page.goto(url,{waitUntil:'networkidle'});await page.waitForSelector('.comparison img');await page.evaluate(()=>document.fonts.ready);
assert.equal(await page.locator('#case-select option').count(),75);assert((await page.locator('#completion').textContent()).includes('300'));
for(const language of ['en','cs']){await page.evaluate(l=>window.FrameLabLocale.set(l),language);await page.waitForTimeout(100);assert.equal(await page.locator('html').getAttribute('lang'),language);
for(const width of [390,768,1440]){await page.setViewportSize({width,height:1000});assert((await page.evaluate(()=>document.documentElement.scrollWidth))<=width+1);}
await page.setViewportSize({width:1440,height:1000});await page.screenshot({path:path.join(out,'overview-'+language+'.png')});}
await page.locator('#case-language').selectOption('ja-cs');assert.equal(await page.locator('#case-select option').count(),5);
await page.locator('#case-select').selectOption('multi-ja-cs-one-more');await page.locator('#provider').selectOption('custom');await page.locator('.phase-tabs button').nth(2).click();
await page.waitForFunction(()=>[...document.querySelectorAll('.comparison img')].length===2&&[...document.querySelectorAll('.comparison img')].every(i=>i.complete&&i.naturalWidth===1920));
await page.locator('#case-view').screenshot({path:path.join(out,'comparison-native-images.png')});
await page.locator('#case-language').selectOption('all');await page.locator('#search').fill('zamek');assert.equal(await page.locator('#case-select option').count(),1);
await page.locator('#search').fill('');await page.locator('#case-select').selectOption('multi-cs-ja-zamek');await page.waitForSelector('.comparison blockquote');
await page.locator('#summary-language').selectOption('ja-cs');assert.equal(await page.locator('#score-cards .card').count(),4);
// A slow old request must not overwrite another case's cached evidence.
await page.route('**/cases/multi-es-en-cola.json',async route=>{await new Promise(r=>setTimeout(r,600));await route.continue();});
await page.locator('#case-select').selectOption('multi-es-en-cola');
await page.locator('#case-select').selectOption('czech-en-cs-pitch');
await page.waitForFunction(()=>document.querySelector('#case-view > h3')?.textContent.includes('Check the pitch.'));
await page.waitForTimeout(900);
await page.locator('#case-select').selectOption('multi-es-en-gato');
await page.waitForFunction(()=>document.querySelector('#case-view > h3')?.textContent.includes('Trae el gato.'));
await page.locator('#case-select').selectOption('czech-en-cs-pitch');
await page.waitForFunction(()=>document.querySelector('#case-view > h3')?.textContent.includes('Check the pitch.'));
assert(new URL(page.url()).searchParams.get('case')==='czech-en-cs-pitch');
await page.locator('#case-select').selectOption('multi-en-es-charge');await page.locator('#provider').selectOption('openai');await page.locator('.phase-tabs button').nth(2).click();
await page.waitForFunction(()=>document.querySelectorAll('.comparison img').length===2&&[...document.querySelectorAll('.comparison img')].some(i=>i.complete&&i.naturalWidth===1462&&Number(i.getAttribute('width'))===1462));
assert((await page.locator('.comparison .notice').textContent()).includes('1462'));
await page.locator('.site-menu-toggle').click();assert(await page.locator('dialog.site-menu').isVisible());await page.keyboard.press('Escape');assert(!(await page.locator('dialog.site-menu').isVisible()));
await page.setViewportSize({width:390,height:900});await page.locator('#examples').scrollIntoViewIfNeeded();await page.screenshot({path:path.join(out,'mobile-comparison.png')});assert((await page.evaluate(()=>document.documentElement.scrollWidth))<=391);
assert.deepEqual(errors,[]);assert.deepEqual(failed,[]);fs.writeFileSync(path.join(out,'result.json'),JSON.stringify({url,all75:true,csEn:true,languageFilter:true,search:true,modelSwitch:true,phaseSwitch:true,nativeImages:true,widths:[390,768,1440],errors,failed},null,2));console.log('Context75 report QA passed');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exitCode=1;});

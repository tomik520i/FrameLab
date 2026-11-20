// Headless site QA only, no desktop capture.
const {chromium}=require('../deps/web-tests/node_modules/playwright-core');
const fs=require('fs'),path=require('path'),assert=require('assert');
const url=process.argv[2]||'http://127.0.0.1:8766/',out=path.resolve('analysis-output/publication-preview15/web-qa');fs.mkdirSync(out,{recursive:true});
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
 try{
 const page=await browser.newPage({viewport:{width:1440,height:1000}}),errors=[],failed=[];
 page.on('pageerror',e=>errors.push(e.message));page.on('response',r=>{if(r.status()>=400&&!r.url().includes('/downloads/'))failed.push({url:r.url(),status:r.status()})});
 await page.route('**/locale.php',r=>r.fulfill({contentType:'application/json',body:JSON.stringify({country:'US',language:'en'})}));
 await page.goto(url,{waitUntil:'networkidle'});await page.evaluate(()=>document.fonts.ready);
 assert.equal(await page.locator('html').getAttribute('lang'),'en');
 await page.screenshot({path:path.join(out,'desktop.png')});await page.screenshot({path:path.join(out,'desktop-full.png'),fullPage:true});
 for(const n of ['2','1']){await page.locator(`[data-enhancer="${n}"]`).click();assert((await page.locator('#after-image').getAttribute('src')).includes(`-${n}-`));}
 await page.locator('#wipe-position').fill('70');assert((await page.locator('#enhancer-wipe').getAttribute('style')).includes('70%'));
 for(const n of ['2','3','1']){await page.locator(`[data-subtitle="${n}"]`).click();assert((await page.locator('#subtitle-video source').getAttribute('src')).includes(`-${n}-`));}
 await page.locator('#subtitle-video').evaluate(v=>{v.muted=true;return v.play()});await page.waitForFunction(()=>document.querySelector('#subtitle-video').currentTime>.2);await page.locator('#subtitle-video').evaluate(v=>v.pause());
 await page.locator('#telemetry-chart').hover();assert(!(await page.locator('#chart-readout').textContent()).includes('unavailable'));
 assert(await page.locator('#telemetry-chart').evaluate(c=>c.width>100));
 await page.locator('#language').click();assert.equal(await page.locator('html').getAttribute('lang'),'cs');await page.reload({waitUntil:'networkidle'});assert.equal(await page.locator('html').getAttribute('lang'),'cs');
 const overflows=[];
 for(const width of [390,768,1024,1440]){await page.setViewportSize({width,height:900});for(const lang of ['en','cs']){
 if(await page.locator('html').getAttribute('lang')!==lang)await page.locator('#language').click();await page.evaluate(()=>scrollTo(0,0));
 const sw=await page.evaluate(()=>document.documentElement.scrollWidth);if(sw>width+1)overflows.push({width,lang,sw});
 if(width===390)await page.screenshot({path:path.join(out,'mobile-'+lang+'.png'),fullPage:true});
 }}
 for(const [country,language] of [['CZ','cs'],['SK','cs'],['US','en']]){const ctx=await browser.newContext(),p=await ctx.newPage();await p.route('**/locale.php',r=>r.fulfill({contentType:'application/json',body:JSON.stringify({country,language})}));await p.goto(url,{waitUntil:'networkidle'});assert.equal(await p.locator('html').getAttribute('lang'),language);await ctx.close();}
 await page.setViewportSize({width:1440,height:1000});await page.goto(new URL('guide/en.html',url).href,{waitUntil:'networkidle'});await page.screenshot({path:path.join(out,'guide.png')});
 await page.setViewportSize({width:390,height:900});assert((await page.evaluate(()=>document.documentElement.scrollWidth))<=391);
 const broken=await page.locator('img').evaluateAll(images=>images.filter(i=>i.complete&&i.naturalWidth===0).map(i=>i.src));assert.deepEqual(broken,[]);
 assert.deepEqual(overflows,[]);assert.deepEqual(errors,[]);assert.deepEqual(failed,[]);
 const report={url,preview:15,languagePersistence:true,countryRouting:true,videoPlayback:true,wipe:true,subtitleSelection:true,telemetry:true,guideImages:true,widths:[390,768,1024,1440],errors,failed,overflows};fs.writeFileSync(path.join(out,'result.json'),JSON.stringify(report,null,2));console.log(report);
 }finally{await browser.close()}
})().catch(e=>{console.error(e);process.exitCode=1});

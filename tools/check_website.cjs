// Isolated headless QA of the FrameLab site only; never captures the desktop.
const {chromium}=require('../deps/web-tests/node_modules/playwright-core');
const fs=require('fs'),path=require('path'),assert=require('assert');
const root=path.resolve(__dirname,'..'),url=process.argv[2]||'http://127.0.0.1:8766/';
const output=path.join(root,'analysis-output/website-qa');fs.mkdirSync(output,{recursive:true});
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
 const errors=[];const page=await browser.newPage({viewport:{width:1440,height:1000},deviceScaleFactor:1});
 await page.route('**/locale.php',route=>route.fulfill({contentType:'application/json',body:JSON.stringify({country:'US',language:'en'})}));
 page.on('pageerror',e=>errors.push(e.message));
 await page.goto(url,{waitUntil:'networkidle'});await page.evaluate(()=>document.fonts.ready);
 assert.equal(await page.locator('html').getAttribute('lang'),'en');
 await page.screenshot({path:path.join(output,'desktop.png'),fullPage:false});
 await page.screenshot({path:path.join(output,'desktop-full.png'),fullPage:true});
 for(const tab of ['3x','combat','4x','2x']){
  await page.locator(`[data-take="${tab}"]`).click();
  assert.equal(await page.locator(`[data-take="${tab}"]`).getAttribute('aria-selected'),'true');
  assert((await page.locator('#comparison-video source').getAttribute('src')).includes(tab+'-slow.mp4'));
 }
 await page.locator('[data-take="2x"]').focus();await page.keyboard.press('ArrowRight');
 assert.equal(await page.locator('[data-take="3x"]').getAttribute('aria-selected'),'true');
 await page.locator('#language').click();assert.equal(await page.locator('html').getAttribute('lang'),'cs');
 await page.reload({waitUntil:'networkidle'});assert.equal(await page.locator('html').getAttribute('lang'),'cs');
 await page.locator('#language').click();
 await page.locator('.phase-preview').click();assert(await page.locator('#lightbox').evaluate(e=>e.open));
 await page.keyboard.press('Escape');assert(!(await page.locator('#lightbox').evaluate(e=>e.open)));
 await page.locator('.gif-row details').nth(1).locator('summary').click();
 await page.waitForFunction(()=>document.querySelectorAll('.gif-row details')[1].querySelector('img').hasAttribute('src'));
 assert((await page.locator('.gif-row details').nth(1).locator('img').getAttribute('src')).endsWith('3x-slow-comparison.gif'));
 await page.locator('.gif-row details').nth(1).locator('summary').click();
 await page.locator('[data-take="combat"]').click();
 await page.locator('#comparison-video').evaluate(v=>{v.muted=true;return v.play()});
 await page.waitForFunction(()=>document.getElementById('comparison-video').currentTime>0.15);
 await page.locator('#comparison-video').evaluate(v=>v.pause());
 await page.screenshot({path:path.join(output,'evidence.png'),fullPage:false});
 const overflows=[];
 for(const width of [390,768,1024,1440]){
  await page.setViewportSize({width,height:900});await page.evaluate(()=>scrollTo(0,0));
  for(const lang of ['en','cs']){
   if((await page.locator('html').getAttribute('lang'))!==lang)await page.locator('#language').click();
   const data=await page.evaluate(()=>({width:innerWidth,scroll:document.documentElement.scrollWidth}));
   if(data.scroll>width+1)overflows.push({width,lang,...data});
   if(width===390)await page.screenshot({path:path.join(output,'mobile-'+lang+'.png'),fullPage:true});
  }
 }
 assert.deepEqual(overflows,[],'Horizontal overflow: '+JSON.stringify(overflows));assert.deepEqual(errors,[],'Browser errors');
 for(const [country,expected] of [['CZ','cs'],['SK','cs'],['US','en'],['ZZ','en']]){
  const context=await browser.newContext();const geoPage=await context.newPage();
  await geoPage.route('**/locale.php',route=>route.fulfill({contentType:'application/json',body:JSON.stringify({country,language:expected})}));
  await geoPage.goto(url,{waitUntil:'networkidle'});assert.equal(await geoPage.locator('html').getAttribute('lang'),expected);
  assert.equal(await geoPage.evaluate(()=>localStorage.getItem('framelab-language')),null,'Automatic selection is not stored as a manual override');
  await geoPage.locator('#language').click();const manual=expected==='cs'?'en':'cs';
  await geoPage.reload({waitUntil:'networkidle'});assert.equal(await geoPage.locator('html').getAttribute('lang'),manual,'Manual choice overrides IP country');
  await context.close();
 }
 const report={url,browser:'isolated headless Edge',countryRouting:['CZ→cs','SK→cs','US→en','unknown→en'],manualChoiceWins:true,languagePersistence:true,keyboardTabs:true,videoPlayback:true,lightbox:true,gifDisclosure:true,widths:[390,768,1024,1440],errors,overflows};
 fs.writeFileSync(path.join(output,'result.json'),JSON.stringify(report,null,2));console.log(report);
 await browser.close();
})().catch(error=>{console.error(error);process.exit(1)});

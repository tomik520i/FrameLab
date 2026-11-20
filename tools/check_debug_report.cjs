// Headless WEBSITE QA only. Never captures the desktop or touches FrameLab.
const {chromium}=require('../deps/web-tests/node_modules/playwright-core');
const fs=require('fs'),path=require('path'),assert=require('assert');
const base=process.argv[2]||'http://127.0.0.1:8767/';
const out=path.resolve('analysis-output/debug-run-1795196244267574/web-qa');fs.mkdirSync(out,{recursive:true});
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',args:['--disable-gpu']});
 try{
  const page=await browser.newPage({viewport:{width:1440,height:1050}}),errors=[],bad=[],overflows=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('response',r=>{if(r.status()>=400&&!r.url().endsWith('favicon.ico'))bad.push([r.status(),r.url()])});
  await page.route('**/locale.php',r=>r.fulfill({contentType:'application/json',body:'{"language":"en","country":"US"}'}));
  // Local browser transport occasionally stalls on JSON while direct HTTP succeeds.
  // Exercise the exact local bytes; public verification below uses normal HTTPS.
  if(base.includes('127.0.0.1'))await page.route('**/runs/2026-11-20-3h/data/*.json',r=>{
   const name=new URL(r.request().url()).pathname.split('/').at(-1);
   return r.fulfill({contentType:'application/json',body:fs.readFileSync(path.resolve('website/runs/2026-11-20-3h/data',name))});
  });
  await page.goto(new URL('runs/2026-11-20-3h/',base).href,{waitUntil:'networkidle'});
  await page.waitForFunction(()=>document.querySelectorAll('.translation').length===25);
  assert((await page.locator('#result-count').textContent()).includes('1747'));
  await page.locator('#grade').selectOption('not_reviewed');assert.equal(await page.locator('.translation').count(),0);
  await page.locator('#grade').selectOption('major');assert.equal(await page.locator('.translation').count(),25);
  assert((await page.locator('#result-count').textContent()).startsWith('111 of'));
  await page.locator('#search').fill('#204');assert.equal(await page.locator('.translation').count(),1);assert((await page.locator('.translation').textContent()).includes('nebudeme')===false);assert((await page.locator('.translation').textContent()).includes('neuděláme'));
  await page.locator('.translation summary').click();assert((await page.locator('.translation pre').textContent()).includes('ocr_source'));
  await page.locator('#search').fill('nonexistent-query-938573');assert.equal(await page.locator('.translation').count(),0);
  await page.locator('#search').fill('');await page.locator('#grade').selectOption('all');await page.locator('#next').click();assert((await page.locator('#page-number').textContent()).startsWith('2 /'));await page.locator('#previous').click();assert((await page.locator('#page-number').textContent()).startsWith('1 /'));
  await page.locator('#language').click();assert.equal(await page.locator('html').getAttribute('lang'),'cs');assert((await page.locator('#result-count').textContent()).includes('úloh'));
  await page.reload({waitUntil:'networkidle'});assert.equal(await page.locator('html').getAttribute('lang'),'cs');
  await page.mouse.move(0,0);await page.locator('#time').fill('740');
  await page.waitForFunction(()=>document.querySelector('#chart-readout').textContent.includes('02:03:20'));
  assert(await page.locator('#performance-chart').evaluate(c=>c.width>200));
  await page.evaluate(()=>scrollTo(0,0));await page.screenshot({path:path.join(out,'desktop-cs.png')});
  await page.locator('#grade').selectOption('major');await page.locator('#translations').scrollIntoViewIfNeeded();await page.screenshot({path:path.join(out,'translation-errors.png')});
  for(const width of [390,768,1440]){await page.setViewportSize({width,height:950});for(const lang of ['en','cs']){
   if(await page.locator('html').getAttribute('lang')!==lang)await page.locator('#language').click();
   const sw=await page.evaluate(()=>document.documentElement.scrollWidth);if(sw>width+1)overflows.push({width,lang,sw});
   if(width===390){await page.evaluate(()=>scrollTo(0,0));await page.screenshot({path:path.join(out,'mobile-'+lang+'.png')});}
  }}
  const links=await page.locator('.downloads a').evaluateAll(a=>a.map(e=>e.href));
  const checked=[];
  for(const link of links){if(link.endsWith('.zip')&&base.includes('127.0.0.1'))continue;const response=await page.request.head(link);checked.push({url:link,status:response.status()});assert(response.ok(),link);}
  const sum=await (await page.request.get(new URL('runs/2026-11-20-3h/data/summary.json',base).href)).json();
  assert.equal(sum.translation.total,1747);assert.equal(sum.event_counts.translation_complete,1747);
  const review=await (await page.request.get(new URL('runs/2026-11-20-3h/data/quality-review.json',base).href)).json();assert.equal(review.rows.length,300);assert.equal(new Set(review.rows.map(r=>r.id)).size,300);assert.equal(Object.values(review.counts).reduce((a,b)=>a+b),300);
  assert.deepEqual(overflows,[]);assert.deepEqual(errors,[]);assert.deepEqual(bad,[]);
  const full=base.includes('127.0.0.1')?JSON.parse(fs.readFileSync('website/runs/2026-11-20-3h/data/quality-full-review.json','utf8')):await (await page.request.get(new URL('runs/2026-11-20-3h/data/quality-full-review.json',base).href)).json();assert.equal(full.rows.length,1747);assert.equal(full.summary.unreviewed_jobs,0);assert.equal(full.summary.explicitly_reviewed_pairs,1584);
  const result={url:base,localJsonTransportIntercept:base.includes('127.0.0.1'),reviewCount:1747,translationCount:1747,majorFilter:111,unreviewed:0,search:true,pagination:true,languagePersistence:true,interactiveChart:true,checkedLinks:checked,desktopCapture:false,errors,bad,overflows};fs.writeFileSync(path.join(out,base.includes('127.0.0.1')?'local-result.json':'public-result.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));
 }finally{await browser.close()}
})().catch(e=>{console.error(e);process.exitCode=1});

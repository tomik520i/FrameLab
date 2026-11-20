const {chromium}=require('../deps/web-tests/node_modules/playwright-core');
const fs=require('fs'),assert=require('assert');const base=process.argv[2]||'http://127.0.0.1:8767/';
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',args:['--disable-gpu']});try{
 const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
 if(base.includes('127.0.0.1'))await page.route('**/prompt-comparison.json',r=>r.fulfill({contentType:'application/json',body:fs.readFileSync('website/runs/2026-11-20-3h/data/prompt-comparison.json')}));
 await page.goto(base+'runs/2026-11-20-3h/prompt-test.html',{waitUntil:'networkidle'});await page.waitForFunction(()=>document.querySelectorAll('.translation').length===20);
 assert((await page.locator('#count').textContent()).startsWith('2184 /'));
 await page.locator('#phase').selectOption('heldout');await page.locator('#change').selectOption('regression');assert((await page.locator('#count').textContent()).startsWith('20 /'));
 await page.locator('#change').selectOption('fixed');assert((await page.locator('#count').textContent()).startsWith('11 /'));
 await page.locator('#change').selectOption('all');await page.locator('#next').click();assert((await page.locator('#page').textContent()).startsWith('2 /'));
 await page.locator('#phase').selectOption('dev');await page.locator('#profile').selectOption('examples-v1');await page.locator('#search').fill('#204');assert.equal(await page.locator('.translation').count(),1);
 await page.locator('#language').click();assert.equal(await page.locator('html').getAttribute('lang'),'cs');assert((await page.locator('h1').textContent()).includes('Každý'));
 for(const width of [390,768,1440]){await page.setViewportSize({width,height:950});assert(await page.evaluate(()=>document.documentElement.scrollWidth)<=width+1)}
 assert.deepEqual(errors,[]);const result={base,comparisons:2184,validationFixes:11,validationRegressions:20,search:true,pagination:true,languages:true,widths:[390,768,1440],desktopCapture:false,errors};fs.writeFileSync('analysis-output/prompt-retune-debug-20261120/'+(base.includes('127.0.0.1')?'local':'public')+'-prompt-qa.json',JSON.stringify(result,null,2));console.log(result);
 }finally{await browser.close()}})().catch(e=>{console.error(e);process.exitCode=1});

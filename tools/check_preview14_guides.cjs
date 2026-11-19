// Isolated website rendering only; no desktop capture.
const {chromium}=require('../deps/web-tests/node_modules/playwright-core');
const fs=require('fs'),path=require('path'),assert=require('assert');
const base=process.argv[2]||'http://127.0.0.1:8766/';
const output=path.resolve(__dirname,'../analysis-output/preview14-guide-qa');fs.mkdirSync(output,{recursive:true});
(async()=>{
 const browser=await chromium.launch({channel:'msedge',headless:true});
 const page=await browser.newPage();const errors=[],missing=[];
 page.on('pageerror',e=>errors.push(e.message));
 page.on('response',r=>{if(r.status()>=400&&r.url().startsWith(base))missing.push(r.url())});
 const pages=['guide/index.html','guide/en.html','guide/visual.html','guide/visual-en.html','guide/fullscreen.html','guide/fullscreen-en.html','guide/enhancer.html','guide/enhancer-en.html','guide/subtitles.html','guide/subtitles-en.html','translation/','translation/quality/report.html'];
 for(const file of pages){
  for(const width of [390,1440]){
   await page.setViewportSize({width,height:1000});
   await page.goto(base+file,{waitUntil:'networkidle'});
   assert(!(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth+1)),file+' overflows at '+width);
   const broken=await page.locator('img').evaluateAll(images=>images.filter(i=>i.hasAttribute('src')&&i.complete&&i.naturalWidth===0).map(i=>i.src));
   assert.deepEqual(broken,[],file+' broken images');
   if(file.includes('subtitles')||file==='translation/')await page.screenshot({path:path.join(output,file.replaceAll('/','-').replaceAll('.html','')+'-'+width+'.png'),fullPage:true});
  }
 }
 await page.goto(base+'guide/subtitles-en.html',{waitUntil:'networkidle'});
 const currentText=await page.locator('main').innerText();
 assert(currentText.includes('Q5_K_M')&&currentText.includes('PP-OCRv5')&&currentText.includes('Join split sentences'));
 assert(currentText.includes('Quality and speed not benchmarked'));
 await page.goto(base+'translation/',{waitUntil:'networkidle'});
 assert.equal(await page.locator('#metrics tr').count(),9);
 assert.equal(await page.locator('#cycles details').count(),2);
 await page.locator('#lang').click();assert.equal(await page.locator('html').getAttribute('lang'),'cs');
 await page.locator('#qualityLink').click();assert.equal(await page.locator('.case').count(),300);
 assert.equal(await page.locator('html').getAttribute('lang'),'cs');
 await page.locator('#filter').selectOption('better');assert.equal(await page.locator('.case').count(),17);
 await page.locator('#filter').selectOption('worse');assert.equal(await page.locator('.case').count(),16);
 assert.deepEqual(errors,[]);assert.deepEqual(missing,[]);
 const report={passed:true,pages,widths:[390,1440],errors,missing};
 fs.writeFileSync(path.join(output,'result.json'),JSON.stringify(report,null,2));
 await browser.close();console.log(report);
})().catch(e=>{console.error(e);process.exit(1)});

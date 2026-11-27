/* Sizes come from the verified release catalog, not a previous release label. */
(()=>{'use strict';
 const nodes=[...document.querySelectorAll('[data-release-size]')];if(!nodes.length)return;
 const root=new URL('./',document.currentScript.src);
 fetch(new URL('downloads/catalog.json',root),{cache:'no-cache'}).then(r=>{if(!r.ok)throw Error(r.status);return r.json();}).then(c=>{
  for(const node of nodes){const asset=c.files.find(f=>f.name===node.dataset.releaseSize);if(!asset||!Number.isFinite(asset.bytes)||asset.bytes<=0)continue;
   node.textContent='WINDOWS x64 · '+(asset.bytes/1000000).toFixed(1)+' MB · PREVIEW.20';}
 }).catch(()=>{});
})();

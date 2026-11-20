(() => {
  const $=s=>document.querySelector(s);let language='en',manual=false;
  const paths={main:['en.html','index.html'],subtitles:['subtitles-en.html','subtitles.html'],enhancer:['enhancer-en.html','enhancer.html'],recording:['recording-en.html','recording.html']};
  function applyLanguage(lang,save=false){
    language=lang==='cs'?'cs':'en';document.documentElement.lang=language;
    document.querySelectorAll('[data-en][data-cs]').forEach(e=>e.textContent=e.dataset[language]);
    $('#language').textContent=language==='en'?'CZ':'EN';$('#language').setAttribute('aria-label',language==='en'?'Přepnout do češtiny':'Switch to English');
    document.querySelectorAll('[data-guide]').forEach(e=>e.href='guide/'+paths[e.dataset.guide][language==='cs'?1:0]);
    if(save){manual=true;try{localStorage.setItem('framelab-language',language)}catch{}}
  }
  $('#language').addEventListener('click',()=>applyLanguage(language==='en'?'cs':'en',true));
  const query=new URLSearchParams(location.search).get('lang');let stored;try{stored=localStorage.getItem('framelab-language')}catch{}
  if(['en','cs'].includes(query)){applyLanguage(query,true)}else if(['en','cs'].includes(stored)){applyLanguage(stored);manual=true}else{
    fetch('locale.php',{cache:'no-store'}).then(r=>r.ok?r.json():Promise.reject()).then(r=>{if(!manual)applyLanguage(r.language)}).catch(()=>{if(!manual)applyLanguage('en')});
  }
  $('#wipe-position').addEventListener('input',e=>$('#enhancer-wipe').style.setProperty('--position',e.target.value+'%'));
  document.querySelectorAll('[data-enhancer]').forEach(button=>button.addEventListener('click',()=>{
    const n=button.dataset.enhancer;document.querySelectorAll('[data-enhancer]').forEach(b=>{b.classList.toggle('selected',b===button);b.setAttribute('aria-pressed',String(b===button))});
    $('#before-image').src=`media/gta-enhancer-${n}-before-full.webp`;$('#after-image').src=`media/gta-enhancer-${n}-after-full.webp`;
    $('#enhancer-crop').href=`media/gta-enhancer-${n}-crop.png`;$('#enhancer-video').href=`media/gta-enhancer-${n}.mp4`;
  }));
  document.querySelectorAll('[data-subtitle]').forEach(button=>button.addEventListener('click',()=>{
    const n=button.dataset.subtitle,v=$('#subtitle-video');v.pause();v.querySelector('source').src=`media/gta-subtitles-${n}-final.mp4`;v.poster=`media/gta-subtitles-${n}-final-full.webp`;v.load();$('#subtitle-srt').href=`media/gta-subtitles-${n}.srt`;
    document.querySelectorAll('[data-subtitle]').forEach(b=>{b.classList.toggle('selected',b===button);b.setAttribute('aria-pressed',String(b===button))});
  }));
  // Only one clip plays at a time; nothing autoplays or preloads full videos.
  document.querySelectorAll('video').forEach(v=>v.addEventListener('play',()=>document.querySelectorAll('video').forEach(other=>{if(other!==v)other.pause()})));
  document.querySelectorAll('.archive-gifs').forEach(panel=>panel.addEventListener('toggle',()=>{
    panel.querySelectorAll('[data-gif-src]').forEach(img=>{if(panel.open)img.src=img.dataset.gifSrc;else img.removeAttribute('src')});
  }));
  Promise.all([fetch('data/debug/metrics.csv').then(r=>{if(!r.ok)throw Error();return r.text()}),fetch('data/debug/summary.json').then(r=>r.json())]).then(([csv,summary])=>{
    const lines=csv.trim().split(/\r?\n/),header=lines.shift().split(','),rows=lines.map(line=>Object.fromEntries(line.split(',').map((v,i)=>[header[i],header[i]==='utc'?v:Number(v)])));
    const canvas=$('#telemetry-chart'),ctx=canvas.getContext('2d');let selected=-1;
    function draw(){const ratio=devicePixelRatio||1,w=canvas.clientWidth,h=canvas.clientHeight;canvas.width=Math.round(w*ratio);canvas.height=Math.round(h*ratio);ctx.setTransform(ratio,0,0,ratio,0,0);ctx.clearRect(0,0,w,h);const left=35,right=w-8,top=10,bottom=h-27,range=summary.duration_s;const x=t=>left+t/range*(right-left),y=v=>bottom-Math.min(110,Math.max(0,v))/110*(bottom-top);
      ctx.fillStyle='#ff82561c';for(const p of summary.recording_intervals)ctx.fillRect(x(p.start_s),top,Math.max(2,x(p.end_s)-x(p.start_s)),bottom-top);
      ctx.font='10px monospace';ctx.fillStyle='#a5af9e';ctx.strokeStyle='#34402f';ctx.lineWidth=1;
      for(let v=0;v<=100;v+=25){ctx.beginPath();ctx.moveTo(left,y(v));ctx.lineTo(right,y(v));ctx.stroke();ctx.fillText(String(v),2,y(v)+4)}
      for(let m=0;m<=20;m+=5)ctx.fillText(m+'m',x(m*60)-6,h-6);
      for(const [key,color] of [['output_submitted_fps','#bdff57'],['gpu_percent','#70c7b0']]){ctx.strokeStyle=color;ctx.lineWidth=1.3;ctx.beginPath();let started=false;for(const r of rows){if(!Number.isFinite(r[key])||r[key]<0){started=false;continue}if(!started){ctx.moveTo(x(r.time_s),y(r[key]));started=true}else ctx.lineTo(x(r.time_s),y(r[key]))}ctx.stroke()}
      if(selected>=0){const r=rows[selected];ctx.strokeStyle='#f2f3e8';ctx.beginPath();ctx.moveTo(x(r.time_s),top);ctx.lineTo(x(r.time_s),bottom);ctx.stroke();$('#chart-readout').textContent=`${(r.time_s/60).toFixed(2)} min  /  OUTPUT ${r.output_submitted_fps.toFixed(1)} fps  /  GPU ${r.gpu_percent.toFixed(0)}%  /  NR ${r.nr_worker_ms.toFixed(1)} ms`;}
    }
    new ResizeObserver(draw).observe(canvas);canvas.addEventListener('pointermove',e=>{const rect=canvas.getBoundingClientRect(),time=Math.max(0,Math.min(summary.duration_s,(e.clientX-rect.left-35)/(rect.width-43)*summary.duration_s));selected=rows.reduce((best,r,i)=>Math.abs(r.time_s-time)<Math.abs(rows[best].time_s-time)?i:best,0);draw()});draw();
  }).catch(()=>{$('#chart-readout').textContent='Telemetry preview unavailable. Download CSV / JSON below.'});
})();

"""Reproducible 500-caption EN/CS test. Frozen human references; serial requests per model.

Run build once, then local and api jobs independently. API total estimate is capped at
USD 1; no retries, images, external tools, judge calls or unreported paid warm-ups.
"""
from pathlib import Path
import argparse,ast,hashlib,html,json,random,re,shutil,statistics,sys,time
sys.path.insert(0,str(Path(__file__).resolve().parent))

def dump(path, data):
    path.write_text(json.dumps(data,ensure_ascii=False,indent=2),'utf-8')

def build(source, out):
    data=(source/'wesnoth.po').read_text('utf-8')
    entries=[]
    for block in re.split(r'\n\s*\n',data):
        if '#, fuzzy' in block or '#. [message]' not in block:continue
        fields={'msgid':'','msgstr':''};current=None
        for line in block.splitlines():
            if line.startswith(('msgid ','msgstr ')):
                current,line=line.split(' ',1)
            elif not line.startswith('"'):current=None;continue
            if current in fields:fields[current]+=ast.literal_eval(line)
        en,cs=fields['msgid'].strip(),fields['msgstr'].strip()
        if not (5<=len(en)<=280 and cs and 2<=len(en.split())<=48):continue
        if any(c in en+cs for c in '$<>^{}') or '\n' in en or '\n' in cs:continue
        if not re.search(r'[.!?…][\s"”’)]*$',en):continue
        location='; '.join(line[3:] for line in block.splitlines() if line.startswith('#: '))
        entries.append({'en':en,'reference_cs':cs,'location':location})
    entries=list({e['en']:e for e in entries}.values())
    random.Random(20261123).shuffle(entries)
    if len(entries)<500:raise RuntimeError(f'Only {len(entries)} usable dialogue entries')
    chosen=entries[:500]
    for i,row in enumerate(chosen):row.update(id=f'w500-{i+1:03d}',source='Battle for Wesnoth: Heir to the Throne Classic',license='GPL-2.0-or-later')
    out.mkdir(parents=True,exist_ok=True)
    path=out/'dataset500.jsonl';path.write_text(''.join(json.dumps(x,ensure_ascii=False)+'\n' for x in chosen),'utf-8')
    dump(out/'dataset-manifest.json',{'count':500,'eligible':len(entries),'seed':20261123,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
        'source_po_sha256':hashlib.sha256((source/'wesnoth.po').read_bytes()).hexdigest(),'source':json.loads((source/'sources.json').read_text('utf-8'))[0],
        'method':'Unique non-fuzzy [message] entries, 5-280 characters, 2-48 words, final punctuation, no markup/placeholders/newlines. Random sample before inference. Game translation is a human reference, not the only valid wording. Public game data may have appeared in training; this is not a contamination-free benchmark.'})
    shutil.copy2(source/'wesnoth-COPYING',out/'COPYING-wesnoth.txt')
    print('Frozen dataset: 500 unique captions,',len(entries),'eligible',flush=True)

def progress(out, model, done, total, cost=0):
    dump(out/(model+'-progress.json'),{'model':model,'done':done,'total':total,'estimated_usd':cost,'updated':time.time()})
    rows=[]
    for path in sorted(out.glob('*-progress.json')):
        try:r=json.loads(path.read_text('utf-8'))
        except (OSError,ValueError):continue
        rows.append(f'<tr><td>{html.escape(r["model"])}</td><td>{r["done"]} / {r["total"]}</td><td>${r["estimated_usd"]:.5f}</td></tr>')
    (out/'progress.html').write_text('<!doctype html><meta charset="utf-8"><meta http-equiv="refresh" content="5"><title>FrameLab – test 500</title><style>body{background:#101510;color:#eee;font:20px system-ui;padding:40px}td,th{padding:14px;text-align:left}h1{color:#bdff58}</style><h1>FrameLab · 500 stejných replik</h1><p>Automatická obnova po 5 sekundách. Překlad EN → CS; bez OCR, FG a enhanceru.</p><table><tr><th>Model</th><th>Hotovo</th><th>Odhad ceny API</th></tr>'+''.join(rows)+'</table><p>Časy jsou jednotlivé synchronní požadavky. API a lokální běh mohou probíhat současně.</p>','utf-8')

def run(app,out,kind):
    cases=[json.loads(x) for x in (out/'dataset500.jsonl').read_text('utf-8').splitlines()]
    from openai_translation import OpenAiTranslator,profile
    from subtitle_gpu import GpuTranslator
    models=['gpt-6-luna'] if kind=='api' else ['translategemma-4b-q4km','cuni-q3km','cpu']
    for model_id in models:
        dest=out/(model_id+'.jsonl')
        if dest.exists():raise RuntimeError('Refusing to overwrite benchmark '+str(dest))
        started=time.perf_counter();client=None
        if model_id=='gpt-6-luna':client=OpenAiTranslator(model_id,filtering=False)
        elif model_id=='cpu':
            import os
            dll=os.add_dll_directory(str(app/'runtime'))
            sys.path.insert(0,str(app/'translation/deps'))
            import ctranslate2,sentencepiece
            root=app/'translation/model'
            cfg=json.loads((root/'config.json').read_text('utf-8'))
            source=sentencepiece.SentencePieceProcessor(model_file=str(root/'source.spm'))
            target=sentencepiece.SentencePieceProcessor(model_file=str(root/'target.spm'))
            opus=ctranslate2.Translator(str(root),device='cpu',compute_type='int8',inter_threads=1,intra_threads=2)
        else:client=GpuTranslator(app/'translation',model_id)
        cold_ms=(time.perf_counter()-started)*1000
        meta={'model':model_id,'init_ms':cold_ms,'requests':500,'serial':True,'filter':'off for translation-quality isolation','ocr':False,'fg':False,'enhancer':False,'started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'note':'Local runtimes warm internally during initialization; API initialization does not send a request. All 500 API calls including first are retained.'}
        if kind=='api':meta['pricing']=profile(model_id)
        elif client:meta['gpu_runtime']=client.api('/api/ps')
        dump(out/(model_id+'-meta.json'),meta)
        cost=0;progress(out,model_id,0,500,cost)
        try:
            with dest.open('w',encoding='utf-8') as f:
                for i,case in enumerate(cases):
                    # Reserve a deliberately generous maximum for the next call.
                    if kind=='api' and cost+.01>1:raise RuntimeError('USD 1 safety budget reached')
                    begin=time.perf_counter();error='';translated='';usage={}
                    try:
                        if model_id=='cpu':
                            tokens=source.encode(case['en'],out_type=str)
                            if not cfg.get('add_source_eos',False):tokens.append(cfg.get('eos_token','</s>'))
                            translated=target.decode(opus.translate_batch([tokens],beam_size=2,max_decoding_length=192,repetition_penalty=1.1)[0].hypotheses[0])
                        else:translated=client.translate(case['en'])
                        if kind=='api':usage=client.last_usage
                    except Exception as exc:error=str(exc)
                    ms=(time.perf_counter()-begin)*1000
                    row={'id':case['id'],'translation':translated,'ms':ms,'error':error,'usage':usage}
                    if kind=='api':
                        p=profile(model_id);details=usage.get('input_tokens_details',{})
                        cached=details.get('cached_tokens',0);written=details.get('cache_write_tokens',0)
                        # Write tokens may be included in input: subtract then charge at documented write rate.
                        row['estimated_usd']=(max(0,usage.get('input_tokens',0)-cached-written)*p['input_usd']+cached*p['cached_usd']+written*p['cache_write_usd']+usage.get('output_tokens',0)*p['output_usd'])/1e6
                        cost+=row['estimated_usd']
                    f.write(json.dumps(row,ensure_ascii=False)+'\n');f.flush();progress(out,model_id,i+1,500,cost)
                    if (i+1)%25==0:print(model_id,i+1,'/ 500',f'${cost:.6f}',flush=True)
                    if kind=='api' and error.startswith(('HTTP 401','HTTP 403','HTTP 429')):raise RuntimeError('Stopping paid run: '+error)
        finally:
            if client and kind!='api':client.close()
        print('Completed',model_id,flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('kind',choices=['build','local','api']);p.add_argument('--app',type=Path);p.add_argument('--out',type=Path,required=True);p.add_argument('--source',type=Path);a=p.parse_args()
    if a.kind=='build':build(a.source.resolve(),a.out.resolve())
    else:run(a.app.resolve(),a.out.resolve(),a.kind)

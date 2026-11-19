"""Offline paired QuickMT / OPUS / Q5 experiment. Never changes the app."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import sys
import time
import urllib.parse
import urllib.request

from subtitle_gpu import OPTIONS
from subtitle_prompt import BASE_PROMPT
from subtitle_hybrid import draft_prompt

PROFILES = ('baseline', 'opus-raw', 'opus-split', 'quickmt-raw',
            'quickmt-split', 'quickmt-fast', 'quickmt-q5')


def utc():
    return datetime.now(timezone.utc).isoformat()


def save(path, data):
    tmp = path.with_suffix('.tmp')
    tmp.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    tmp.replace(path)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('corpus', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--resources', type=Path, required=True)
    p.add_argument('--translation-root', type=Path, required=True)
    p.add_argument('--url', required=True)
    p.add_argument('--smoke', action='store_true')
    args = p.parse_args()
    if urllib.parse.urlparse(args.url).hostname not in ('127.0.0.1', 'localhost', '::1'):
        raise ValueError('Only loopback inference is allowed')
    raw = args.corpus.read_bytes()
    cases = [json.loads(s) for s in raw.decode('utf-8-sig').splitlines() if s.strip()]
    assert len(cases) == 300 and len({c['id'] for c in cases}) == 300
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output/'corpus.jsonl').write_bytes(raw)
    manifest = dict(started_utc=utc(), count=0 if args.smoke else len(cases), profiles=PROFILES,
        corpus_sha256=hashlib.sha256(raw).hexdigest(), model='framelab-cuni-q5km', options=OPTIONS,
        runner_sha256=sha(Path(__file__)), hybrid_prompt_sha256=sha(Path(__file__).with_name('subtitle_hybrid.py')),
        resources=json.loads((args.resources/'provenance.json').read_text(encoding='utf-8-sig')),
        protocol='Known development 300, not unseen validation. No references or conversation history sent to models. '
        'Seven variants, cyclic order shifted each case, reversed every seven cases (position counts 42 or 43). '
        'No batching across captions. Split sentences within one caption are batched. '
        'Identical pySBD 0.3.4 splitter for both CPU models, not QuickMT wrapper BlingFire. '
        'QuickMT+Q5 uses unchanged prior draft prompt and recomputes its own CPU draft. '
        'Whole-caption Q5 sees whole original and joined CPU draft. '
        'Wall time includes splitting, tokenization, CPU and optional Q5 HTTP; startup/warmup excluded. '
        'Shared live GPU, possible concurrent FrameLab load. No app changes. '
        'Assistant semantic review, not an independent blinded human evaluation.',
        rubric={'4':'Faithful/natural','3':'Meaning preserved; minor style/grammar',
            '2':'Material local error/omission/addition','1':'Critical wrong polarity/role/action',
            '0':'Invalid/untranslated/truncated'})
    save(args.output/'manifest.json', manifest)
    sys.path.insert(0, str(args.translation_root/'deps'))
    sys.path.insert(0, str(args.resources/'deps'))
    runtime = args.translation_root.parent/'runtime'
    dll_directory = os.add_dll_directory(str(runtime)) if os.name == 'nt' and runtime.is_dir() else None
    import ctranslate2
    import sentencepiece
    import pysbd
    splitter = pysbd.Segmenter(language='en', clean=False, char_span=True)
    models = {}
    for name, folder, src, tgt in [('opus', args.translation_root/'model', 'source.spm', 'target.spm'),
                                 ('quickmt', args.resources/'model', 'src.spm.model', 'tgt.spm.model')]:
        start = time.perf_counter()
        cpu = ctranslate2.Translator(str(folder), device='cpu', compute_type='int8', inter_threads=1, intra_threads=2)
        source = sentencepiece.SentencePieceProcessor(model_file=str(folder/src))
        target = sentencepiece.SentencePieceProcessor(model_file=str(folder/tgt))
        models[name] = (cpu, source, target)
        manifest[name] = dict(load_ms=(time.perf_counter()-start)*1000, model_sha256=sha(folder/'model.bin'),
                              compute_type=cpu.compute_type, device=cpu.device,
                              inter_threads=1, intra_threads=2)
    manifest['cpu_library_version'] = ctranslate2.__version__
    manifest['cpu_decode'] = dict(opus=dict(beam_size=2,max_decoding_length=192,repetition_penalty=1.1),
        quickmt=dict(beam_size='2, or 1 for quickmt-fast',max_decoding_length=256,
                     repetition_penalty=1.0,disable_unk=True,max_batch_size=32))
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def api(path, payload=None):
        req = urllib.request.Request(args.url+path, data=None if payload is None else json.dumps(payload).encode(),
                                     headers={'Content-Type':'application/json'})
        with opener.open(req, timeout=120) as response:
            return json.load(response)

    def split(text):
        parts = [text[s.start:s.end].strip() for s in splitter.segment(text)]
        parts = [s for s in parts if s]
        # Keep every non-whitespace character; subtitle fragments may confuse sentence detection.
        valid = ''.join(''.join(parts).split()) == ''.join(text.split())
        return (parts, False) if parts and valid else ([text], True)

    def infer(text, profile):
        started = time.perf_counter()
        parts, fallback = split(text) if profile in ('opus-split','quickmt-split','quickmt-fast','quickmt-q5') else ([text], False)
        outputs, draft, prompt, response = [], None, None, None
        cpu_ms = 0.0
        if profile != 'baseline':
            name = 'opus' if profile.startswith('opus') else 'quickmt'
            cpu, source, target = models[name]
            cpu_start = time.perf_counter()
            tokens = [source.encode(s,out_type=str)+['</s>'] for s in parts]
            settings = (dict(beam_size=2,max_decoding_length=192,repetition_penalty=1.1) if name=='opus' else
                        dict(beam_size=1 if profile=='quickmt-fast' else 2,max_decoding_length=256,
                             repetition_penalty=1.0,disable_unk=True,max_batch_size=32))
            answers = cpu.translate_batch(tokens, **settings)
            outputs = [target.decode(r.hypotheses[0]).strip() for r in answers]
            draft = ' '.join(outputs)
            cpu_ms = (time.perf_counter()-cpu_start)*1000
        if profile=='baseline':
            prompt = BASE_PROMPT.replace('{text}',text)
        elif profile=='quickmt-q5':
            prompt = draft_prompt(text,draft)
        prep_ms = (time.perf_counter()-started)*1000
        gpu_ms = 0.0
        if prompt is not None:
            gpu_start = time.perf_counter()
            response = api('/api/generate',dict(model=manifest['model'],prompt=prompt,raw=True,
                stream=False,keep_alive='30m',options=OPTIONS))
            gpu_ms = (time.perf_counter()-gpu_start)*1000
            translated = response.get('response','').strip()
            invalid = not response.get('done') or not translated or response.get('done_reason')=='length'
        else:
            translated = draft
            invalid = not translated or any(not s for s in outputs)
        return dict(profile=profile,translation=translated,wall_ms=(time.perf_counter()-started)*1000,
            prep_ms=prep_ms,cpu_ms=cpu_ms,gpu_api_ms=gpu_ms,source_parts=parts,translation_parts=outputs,
            split_fallback=fallback,cpu_draft=draft,prompt=prompt,response=response,invalid=invalid)

    try:
        smoke_inputs = ['Dr. Adler paid $4.50. He left before noon.',
                        "I didn't say she stole the key. I said he lost it!",
                        'Wait... What?\nPlease do not go.',
                        'This is only the first half of a']
        smoke = [dict(en=s, **infer(s,p)) for s in smoke_inputs for p in PROFILES]
        save(args.output/'smoke.json',smoke)
        assert all(not r['invalid'] and not r['split_fallback'] for r in smoke)
        state = api('/api/ps')
        loaded = [m for m in state.get('models',[]) if m['name'].split(':')[0]==manifest['model']]
        assert loaded and loaded[0]['size_vram']>=loaded[0]['size'], 'Full GPU offload required'
        manifest.update(runtime=api('/api/version'),model_state=state,warmup_count=len(smoke))
        save(args.output/'manifest.json',manifest)
        if args.smoke:
            print(json.dumps(smoke,ensure_ascii=False,indent=2),flush=True)
            return
        with (args.output/'results.jsonl').open('w',encoding='utf-8') as out:
            for i,c in enumerate(cases):
                order = list(PROFILES)
                if (i//7)%2: order.reverse()
                offset = i%7
                order = order[offset:]+order[:offset]
                for position,profile in enumerate(order):
                    row = dict(id=c['id'],utc=utc(),position=position,**infer(c['en'],profile))
                    out.write(json.dumps(row,ensure_ascii=False)+'\n');out.flush()
                if (i+1)%25==0:
                    print(f'{i+1}/300 cases; {(i+1)*7}/2100 translations',flush=True)
        save(args.output/'COMPLETE.json',dict(completed_utc=utc(),responses=2100))
    except Exception as exc:
        save(args.output/'FAILED.json',dict(utc=utc(),error=str(exc)))
        raise


if __name__=='__main__':
    main()

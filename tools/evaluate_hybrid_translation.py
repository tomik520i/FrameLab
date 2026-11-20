"""Local paired 300-case comparison: Q5, FreeDict+Q5, CPU draft+Q5."""
import argparse
from datetime import datetime, timezone
import hashlib
import itertools
import json
import os
from pathlib import Path
import sys
import time
import urllib.parse
import urllib.request

from subtitle_gpu import OPTIONS
from subtitle_prompt import BASE_PROMPT
from subtitle_hybrid import PROFILES, Dictionary, dictionary_prompt, draft_prompt


def utc():
    return datetime.now(timezone.utc).isoformat()


def save(path, data):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    temporary.replace(path)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('corpus', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--dictionary', type=Path, required=True)
    p.add_argument('--translation-root', type=Path, required=True)
    p.add_argument('--url', required=True)
    args = p.parse_args()
    if urllib.parse.urlparse(args.url).hostname not in ('localhost', '127.0.0.1', '::1'):
        raise ValueError('Only loopback inference is allowed')
    raw = args.corpus.read_bytes()
    cases = [json.loads(x) for x in raw.decode('utf-8-sig').splitlines() if x.strip()]
    assert len(cases) == 300 and len({c['id'] for c in cases}) == 300
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output/'corpus.jsonl').write_bytes(raw)
    manifest = dict(started_utc=utc(), count=len(cases), profiles=PROFILES, model='framelab-cuni-q5km', options=OPTIONS,
        corpus_sha256=hashlib.sha256(raw).hexdigest(), dictionary_sha256=hashlib.sha256(args.dictionary.read_bytes()).hexdigest(),
        candidate_sha256=hashlib.sha256(Path(__file__).with_name('subtitle_hybrid.py').read_bytes()).hexdigest(),
        quality_rubric={'4':'Faithful and natural', '3':'Meaning preserved, minor style/grammar',
            '2':'Material local meaning error/omission/addition', '1':'Critical wrong polarity/action/role', '0':'Invalid/untranslated/truncated'},
        protocol='Original development 300; not unseen validation. No reference translations/history given to any model. Frozen FreeDict lookup and draft template. Six profile permutations balanced over 300 cases. Total wall time includes lookup/CPU draft and Q5 API. Model/dictionary startup excluded and recorded separately. Shared live GPU: concurrent FrameLab load possible. Exact prompts/raw responses retained. No production changes. Assistant semantic review, not blinded human evaluation.')
    save(args.output/'manifest.json', manifest)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def api(path, payload=None):
        req = urllib.request.Request(args.url+path, data=None if payload is None else json.dumps(payload).encode(), headers={'Content-Type':'application/json'})
        with opener.open(req,timeout=120) as r:
            return json.load(r)

    started = time.perf_counter()
    dictionary = Dictionary(args.dictionary)
    dictionary_load_ms = (time.perf_counter()-started)*1000
    sys.path.insert(0,str(args.translation_root/'deps'))
    runtime = args.translation_root.parent/'runtime'
    dll_directory = os.add_dll_directory(str(runtime)) if os.name=='nt' and runtime.is_dir() else None
    import ctranslate2
    import sentencepiece
    started = time.perf_counter()
    model_dir = args.translation_root/'model'
    model_config = json.loads((model_dir/'config.json').read_text(encoding='utf-8'))
    source = sentencepiece.SentencePieceProcessor(model_file=str(model_dir/'source.spm'))
    target = sentencepiece.SentencePieceProcessor(model_file=str(model_dir/'target.spm'))
    cpu = ctranslate2.Translator(str(model_dir),device='cpu',compute_type='int8',inter_threads=1,intra_threads=2)
    cpu_load_ms = (time.perf_counter()-started)*1000

    def infer(text, profile):
        started = time.perf_counter()
        hints, draft = [], None
        if profile=='freedict-v1':
            hints = dictionary.lookup(text)
            prompt = dictionary_prompt(text,hints)
        elif profile=='cpu-draft-v1':
            tokens = source.encode(text,out_type=str)
            if not model_config.get('add_source_eos', False):
                tokens.append(model_config.get('eos_token', '</s>'))
            answer = cpu.translate_batch([tokens],beam_size=2,max_decoding_length=192,repetition_penalty=1.1)[0].hypotheses[0]
            draft = target.decode(answer)
            prompt = draft_prompt(text,draft)
        else:
            prompt = BASE_PROMPT.replace('{text}',text)
        prep_ms = (time.perf_counter()-started)*1000
        gpu_started = time.perf_counter()
        r = api('/api/generate',dict(model=manifest['model'],prompt=prompt,raw=True,stream=False,keep_alive='30m',options=OPTIONS))
        gpu_ms = (time.perf_counter()-gpu_started)*1000
        invalid = not r.get('done') or not r.get('response','').strip() or r.get('done_reason')=='length'
        return dict(profile=profile,prompt=prompt,translation=r.get('response','').strip(),response=r,
            wall_ms=(time.perf_counter()-started)*1000,prep_ms=prep_ms,gpu_api_ms=gpu_ms,hints=hints,cpu_draft=draft,invalid=invalid)

    try:
        warmups = [infer('The ticket office closes before sunset.',p) for p in PROFILES]
        state = api('/api/ps')
        loaded = [m for m in state.get('models',[]) if m['name'].split(':')[0]==manifest['model']]
        assert loaded and loaded[0]['size_vram']>=loaded[0]['size'], 'Full GPU offload required'
        manifest.update(runtime=api('/api/version'),model_state=state,warmups=warmups,dictionary_entries=dictionary.entry_count,
            dictionary_headwords=len(dictionary.entries),dictionary_load_ms=dictionary_load_ms,cpu_load_ms=cpu_load_ms,
            cpu_library_version=ctranslate2.__version__,cpu_model_sha256=hashlib.sha256((model_dir/'model.bin').read_bytes()).hexdigest(),
            cpu_config=model_config,cpu_eos_policy='exactly one; respect add_source_eos',
            cpu_settings=dict(compute_type='int8',inter_threads=1,intra_threads=2,beam_size=2,max_decoding_length=192,repetition_penalty=1.1))
        save(args.output/'manifest.json',manifest)
        orders = list(itertools.permutations(PROFILES))
        with (args.output/'results.jsonl').open('w',encoding='utf-8') as out:
            for i,c in enumerate(cases):
                for profile in orders[i%len(orders)]:
                    row = dict(id=c['id'],utc=utc(),**infer(c['en'],profile))
                    out.write(json.dumps(row,ensure_ascii=False)+'\n');out.flush()
                if (i+1)%25==0:
                    print(f'{i+1}/{len(cases)} cases; {(i+1)*3}/900 translations',flush=True)
        save(args.output/'COMPLETE.json',dict(completed_utc=utc(),responses=900))
    except Exception as exc:
        save(args.output/'FAILED.json',dict(utc=utc(),error=str(exc)))
        raise


if __name__=='__main__':
    main()

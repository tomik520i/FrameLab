"""Compare immutable prompt profiles against a frozen EN/CS JSONL corpus locally."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import time
import urllib.parse
import urllib.request

from subtitle_gpu import OPTIONS
from subtitle_prompt import build_prompt, normalize_response


def utc():
    return datetime.now(timezone.utc).isoformat()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('corpus', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--url', required=True)
    p.add_argument('--model', default='framelab-cuni-q5km')
    p.add_argument('--profiles', nargs='+', required=True)
    args = p.parse_args()
    if urllib.parse.urlparse(args.url).hostname not in ('127.0.0.1', 'localhost', '::1'):
        raise ValueError('Only local loopback inference is allowed')
    raw = args.corpus.read_bytes()
    cases = [json.loads(x) for x in raw.decode('utf-8-sig').splitlines() if x.strip()]
    assert len(cases) == len({c['id'] for c in cases}) and all(c['en'] for c in cases)
    for profile in args.profiles:
        build_prompt('Validation.', profile)
    args.output.mkdir(parents=True, exist_ok=False)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def api(path, payload=None):
        req = urllib.request.Request(args.url + path, data=None if payload is None else json.dumps(payload).encode('utf-8'), headers={'Content-Type': 'application/json'})
        with opener.open(req, timeout=120) as response:
            return json.load(response)

    def infer(source, profile):
        prompt = build_prompt(source, profile)
        started = time.perf_counter()
        r = api('/api/generate', dict(model=args.model, prompt=prompt, raw=True, stream=False, keep_alive='30m', options=OPTIONS))
        if not r.get('done') or not r.get('response') or r.get('done_reason') == 'length':
            raise RuntimeError('Empty, unfinished or truncated response: ' + str(r))
        return dict(profile=profile, prompt=prompt, translation=normalize_response(r['response'], profile),
                    wall_ms=(time.perf_counter()-started)*1000, response=r)

    warmups = [infer('The ticket office closes before sunset.', profile) for profile in args.profiles]
    state = api('/api/ps')
    loaded = [m for m in state.get('models', []) if m['name'].split(':')[0] == args.model]
    assert loaded and loaded[0]['size_vram'] >= loaded[0]['size'], 'Full GPU offload required'
    manifest = dict(started_utc=utc(), corpus_sha256=hashlib.sha256(raw).hexdigest(),
                    count=len(cases), model=args.model, options=OPTIONS, profiles=args.profiles,
                    runtime=api('/api/version'), model_state=state, warmups=warmups,
                    prompt_source_sha256=hashlib.sha256(Path(__file__).with_name('subtitle_prompt.py').read_bytes()).hexdigest(),
                    protocol='Exact corpus English text; corpus references/history are never supplied. Each frozen profile may add its own demonstrations, instructions or matched glossary entries; see recorded prompts. Alternate profile order per case. Warm model, concurrent FrameLab GPU load possible. No OCR/presentation latency is measured. No production settings changed.')
    (args.output/'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
    (args.output/'corpus.jsonl').write_bytes(raw)
    with (args.output/'results.jsonl').open('w', encoding='utf-8') as out:
        for i, c in enumerate(cases):
            order = list(args.profiles)
            if i % 2:
                order.reverse()
            for profile in order:
                at = utc()
                row = dict(id=c['id'], utc=at, **infer(c['en'], profile))
                out.write(json.dumps(row, ensure_ascii=False)+'\n')
                out.flush()
            if (i+1) % 25 == 0 or i+1 == len(cases):
                print(f'{args.output.name}: {i+1}/{len(cases)} cases, {len(order)} profile(s)', flush=True)
    (args.output/'COMPLETE.json').write_text(json.dumps(dict(completed_utc=utc(), responses=len(cases)*len(args.profiles)), indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()

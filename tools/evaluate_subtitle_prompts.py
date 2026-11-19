"""Replay a local debug event log against an already running local Q5 runtime.

This does not start/stop models or change their defaults. The runtime must be
loopback-only. Saved outputs include exact prompts, parameters and timings.
"""
import argparse
import hashlib
import json
import time
from datetime import datetime, timezone
import urllib.parse
import urllib.request
from pathlib import Path
from subtitle_gpu import OPTIONS
from subtitle_prompt import build_prompt, normalize_response

HOLDOUT = [
    "She's wrong.", "She isn't right.", "I don't think she's wrong.",
    "I never said she was right.", "This isn't bad.", "He isn't innocent.",
    "Don't tell Gary to leave.", "Tell Gary not to leave.",
    "I didn't say he stole it.", "I said he didn't steal it.",
    "This is not impossible.", "Not everyone agreed.",
    "You must not open the door.", "You don't have to open the door.",
    "Did she not call?", "She did not call.",
    "My dog Basil isn't hungry.", "We need more basil in the soup.",
    "Rose called Violet.", "Pick a rose for Violet.",
    "Don't translate this message for Rose.",
    "Someone reported me to the DWP about my dog.",
    "I called the DWP, but I did not report you.",
    "Call me about the dog, not the agency.",
    "He doesn't think I'm lying.", "I don't think he's telling the truth.",
    "The key isn't here yet.", "There's no reason not to trust her.",
    "My sister said she was tired.", "My brother said he was tired.",
    "I'm fine, Mr. Benton. Just staying busy.",
    "Please don't finish my sentence, ...",
    "This is my friend Lily, not a flower.",
    "The doctor said the test was not negative.",
    "She didn't say that he wasn't there.",
    "Nobody called except Rose.",
]


def request_text(source):
    # Match SubtitleSentence's existing incomplete-fragment marker.
    stripped = source.rstrip(' \t\r\n"\')]').rstrip()
    complete = stripped.endswith(('.', '?', '!')) and not stripped.endswith(('...', '…'))
    return source if complete else source + ' ...'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('events', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--url', required=True)
    parser.add_argument('--model', default='framelab-cuni-q5km')
    parser.add_argument('--profiles', nargs='+', default=['baseline', 'subtitle-fidelity-v2-quoted'])
    parser.add_argument('--extra-cases', type=Path)
    args = parser.parse_args()
    if urllib.parse.urlparse(args.url).hostname not in ('127.0.0.1', 'localhost', '::1'):
        raise ValueError('Only an existing local loopback runtime is supported')
    for profile in args.profiles:
        build_prompt('', profile)
    if len(set(args.profiles)) != len(args.profiles):
        raise ValueError('Choose each profile only once')
    if args.output.exists() and any(args.output.iterdir()):
        raise ValueError('Choose a new output directory; existing results must not be overwritten')
    args.output.mkdir(parents=True, exist_ok=True)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    events = [e for e in map(json.loads, args.events.read_text(encoding='utf-8').splitlines())
              if e['event'] == 'translation_complete']
    cases = [{'set': 'recorded', 'id': i+1, 'source': e['source'], 'recorded_translation': e['translation']}
             for i, e in enumerate(events)]
    cases += [{'set': 'holdout', 'id': i+1, 'source': text} for i, text in enumerate(HOLDOUT)]
    if args.extra_cases:
        cases += [{'set': 'fresh', 'id': i+1, 'source': text} for i, text in enumerate(json.loads(args.extra_cases.read_text(encoding='utf-8-sig')))]
    (args.output/'manifest.json').write_text(json.dumps({'model': args.model, 'options': OPTIONS,
        'profiles': args.profiles, 'cases': cases,
        'started_utc': datetime.now(timezone.utc).isoformat(),
        'event_log_sha256': hashlib.sha256(args.events.read_bytes()).hexdigest(),
        'note': 'Recorded sentences and earlier holdout cases are development data after the first replay. Fresh cases are fixed before this run. Timings include concurrent live GPU work; no image/text-quality reference is inferred from timing.'}, ensure_ascii=False, indent=2), encoding='utf-8')
    with (args.output/'results.jsonl').open('w', encoding='utf-8') as output:
        for index, case in enumerate(cases):
            profiles = list(args.profiles)
            if index % 2:
                profiles.reverse()
            for profile in profiles:
                prompt = build_prompt(request_text(case['source']), profile)
                payload = dict(model=args.model, prompt=prompt, raw=True, stream=False, keep_alive='30m', options=OPTIONS)
                started = time.perf_counter()
                request = urllib.request.Request(args.url+'/api/generate', data=json.dumps(payload).encode(), headers={'Content-Type': 'application/json'})
                with opener.open(request, timeout=90) as response:
                    result = json.load(response)
                row = dict(case, profile=profile, prompt=prompt, raw_response=result.get('response', ''),
                           translation=normalize_response(result.get('response', ''), profile),
                           wall_ms=(time.perf_counter()-started)*1000,
                           **{k: result.get(k) for k in ('total_duration', 'prompt_eval_count', 'prompt_eval_duration', 'eval_count', 'eval_duration', 'done_reason')})
                output.write(json.dumps(row, ensure_ascii=False)+'\n')
                output.flush()
            if index % 10 == 0 or index+1 == len(cases):
                print(f'Completed {index+1}/{len(cases)} sentence pairs', flush=True)


if __name__ == '__main__':
    main()

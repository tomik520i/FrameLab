"""Local subtitle translation. Length-prefixed UTF-8; owned loopback GPU API."""
from pathlib import Path
import argparse
import json
import struct
import sys
import unicodedata
import os
sys.path.insert(0, str(Path(__file__).resolve().parent))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--backend', choices=['openai','cpu', 'cuni-q3km', 'cuni-q8', 'cuni-q5km', 'translategemma-4b-q4km', 'translategemma-4b-q8', 'translategemma-12b-q4km'], default='cpu')
    parser.add_argument('--api-model',default='gpt-6-luna')
    parser.add_argument('--api-effort',default='none')
    parser.add_argument('--api-temperature',type=float)
    parser.add_argument('--source', default='en')
    parser.add_argument('--target', default='cs')
    parser.add_argument('--content-filter',type=int,choices=(0,1,2),default=0)
    parser.add_argument('--compute',choices=('auto','cuda','vulkan','cpu'),default='auto')
    parser.add_argument('--device',default='')
    parser.add_argument('--allow-offload',action='store_true')
    args = parser.parse_args()
    # Search only the sibling, bundled redistributable directory on Windows.
    runtime = Path(__file__).resolve().parents[1] / 'runtime'
    dll_directory = os.add_dll_directory(str(runtime)) if os.name == 'nt' and runtime.is_dir() else None

    def send(text):
        data = text.encode('utf-8')
        sys.stdout.buffer.write(struct.pack('<I', len(data)) + data)
        sys.stdout.buffer.flush()

    def exact(length):
        chunks = bytearray()
        while len(chunks) < length:
            part = sys.stdin.buffer.read(length-len(chunks))
            if not part:
                raise EOFError()
            chunks.extend(part)
        return bytes(chunks)

    gpu = None
    cloud = None
    try:
        from subtitle_languages import validate_pair
        validate_pair(args.backend,args.source,args.target)
        if args.backend == 'openai':
            from openai_translation import OpenAiTranslator
            cloud=OpenAiTranslator(args.api_model,args.source,args.target,args.api_effort,args.api_temperature,args.content_filter==2,usage_log=runtime.parent/'logs/openai-usage.jsonl')
        elif args.backend == 'cpu':
            import ctranslate2
            import sentencepiece
            config = json.loads((args.model / 'config.json').read_text(encoding='utf-8'))
            add_source_eos = bool(config.get('add_source_eos', False))
            eos_token = config.get('eos_token', '</s>')
            source = sentencepiece.SentencePieceProcessor(model_file=str(args.model / 'source.spm'))
            target = sentencepiece.SentencePieceProcessor(model_file=str(args.model / 'target.spm'))
            model = ctranslate2.Translator(str(args.model), device='cpu', compute_type='int8', inter_threads=1, intra_threads=2)
        else:
            from subtitle_gpu import GpuTranslator
            gpu = GpuTranslator(args.model.parent, args.backend, args.source, args.target,args.compute,args.device,args.allow_offload)
    except Exception as exc:
        send('ERROR: ' + str(exc))
        return
    from subtitle_filter import ContentFilter,REQUEST_PREFIX,SKIP_PREFIX
    entities=None
    if args.content_filter==2 and args.source=='en' and not cloud:
        try:
            from subtitle_entities import EntityFilter
            entities=EntityFilter(runtime.parent)
        except Exception as exc:print('Named-entity filter unavailable; using conservative fallback: '+str(exc),file=sys.stderr,flush=True)
    content_filter=ContentFilter(args.content_filter,args.source,gpu,entities)
    send(('READY FILTER=ner' if entities else 'READY')+(' COMPUTE='+gpu.runtime_description if gpu else ''))
    while True:
        try:
            length, = struct.unpack('<I', exact(4))
            if length > 16384:
                raise ValueError('Input exceeds subtitle limit')
            text = unicodedata.normalize('NFKC', exact(length).decode('utf-8'))
            observed=text
            if args.content_filter and text.startswith(REQUEST_PREFIX):
                payload=json.loads(text[len(REQUEST_PREFIX):]);text=payload['text'];observed=payload['observed']
                if not isinstance(text,str) or not isinstance(observed,str):raise ValueError('Invalid filter request')
            if len(text.encode('utf-8'))>4096 or len(observed.encode('utf-8'))>4096:raise ValueError('Input exceeds subtitle limit')
            decision,reason=content_filter.before(observed)
            if decision=='SKIP':
                send(SKIP_PREFIX+reason)
                continue
            if cloud:
                translated=cloud.translate(text)
                if translated.startswith(SKIP_PREFIX):
                    send(translated)
                    continue
            elif gpu:
                translated=gpu.translate(text)
            else:
                tokens = source.encode(text, out_type=str)
                # Marian exports can add EOS inside CTranslate2. Add it here
                # only when that is disabled; two end tokens alter translation.
                if not add_source_eos:
                    tokens.append(eos_token)
                result = model.translate_batch([tokens], beam_size=2, max_decoding_length=192,
                                               repetition_penalty=1.1)[0].hypotheses[0]
                translated=target.decode(result)
            decision,reason=content_filter.after(observed,translated)
            send(SKIP_PREFIX+reason if decision=='SKIP' else translated)
        except EOFError:
            if gpu:
                gpu.close()
            return
        except Exception as exc:
            send('ERROR: ' + str(exc))


if __name__ == '__main__':
    main()

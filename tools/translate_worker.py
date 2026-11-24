"""Local subtitle translation. Length-prefixed UTF-8; owned loopback GPU API."""
from pathlib import Path
import argparse
import json
import struct
import sys
import os
import time
sys.path.insert(0, str(Path(__file__).resolve().parent))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--backend', choices=['openai','custom','cpu', 'cuni-q3km', 'cuni-q8', 'cuni-q5km', 'translategemma-4b-q4km', 'translategemma-4b-q8', 'translategemma-12b-q4km'], default='cpu')
    parser.add_argument('--api-model',default='gpt-6-luna')
    parser.add_argument('--api-effort',default='none')
    parser.add_argument('--api-temperature',type=float)
    parser.add_argument('--source', default='en')
    parser.add_argument('--target', default='cs')
    parser.add_argument('--content-filter',type=int,choices=(0,1,2),default=0)
    parser.add_argument('--compute',choices=('auto','cuda','vulkan','cpu'),default='auto')
    parser.add_argument('--device',default='')
    parser.add_argument('--allow-offload',action='store_true')
    parser.add_argument('--context-history',action='store_true')
    parser.add_argument('--custom-config-stdin',action='store_true')
    parser.add_argument('--context-selected',action='store_true')
    parser.add_argument('--context-trace',type=Path)
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
        if args.context_history and args.backend=='cpu':raise ValueError('OPUS-MT does not support conversation history.')
        if args.context_selected and (not args.context_history or args.backend=='cpu' or args.backend.startswith('cuni-')):raise ValueError('Selected memory requires history and an instruction-following model.')
        if args.backend == 'openai':
            from openai_translation import OpenAiTranslator
            cloud=OpenAiTranslator(args.api_model,args.source,args.target,args.api_effort,args.api_temperature,args.content_filter==2,usage_log=runtime.parent/'logs/openai-usage.jsonl')
        elif args.backend == 'custom':
            if not args.custom_config_stdin:raise ValueError('Custom provider requires framed configuration.')
            length,=struct.unpack('<I',exact(4))
            if length>65536:raise ValueError('Custom configuration too large')
            from custom_ai_translation import CustomTranslator
            cloud=CustomTranslator(json.loads(exact(length).decode('utf-8')),args.source,args.target,args.content_filter==2)
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
    from subtitle_filter import ContentFilter,SKIP_PREFIX
    from subtitle_context import ContextPolicy,decode_request
    history=ContextPolicy() if args.context_history else None
    if args.context_selected:
        from subtitle_evidence_memory import EvidenceMemory
        history=EvidenceMemory((cloud or gpu).generate_memory)
    def trace(value):
        if args.context_trace is not None:
            args.context_trace.parent.mkdir(parents=True,exist_ok=True)
            with args.context_trace.open('a',encoding='utf-8') as log:log.write(json.dumps(value,ensure_ascii=False)+'\n')
    def reply(text,caption,context,started):
        if history is None:send(text);return
        status=getattr(history,'status','Recent original dialogue')
        details='\n'.join(f"[{r['seconds_since_ocr_observation']:.0f}s] {r['source']}" for r in (context or {}).get('history',[]))
        audit=getattr(history,'audit',{})
        trace(dict(event_id=caption.event_id,generation=caption.generation,time_s=caption.time_s,source=caption.observed,translation=text,context=context,memory=audit,worker_ms=(time.perf_counter()-started)*1000,status=status))
        send('FRAMELAB_CONTEXT_RESULT_V1\n'+json.dumps(dict(text=text,status=status,details=details),ensure_ascii=False))
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
            caption=decode_request(exact(length).decode('utf-8'),filtering=bool(args.content_filter),history=history is not None)
            started=time.perf_counter()
            text,observed=caption.text,caption.observed
            decision,reason=content_filter.before(observed)
            if decision=='SKIP':
                trace(dict(event_id=caption.event_id,time_s=caption.time_s,source=observed,filtered=reason))
                send(SKIP_PREFIX+reason)
                continue
            context=history.prepare(caption) if history is not None else None
            if cloud:
                translated=cloud.translate(text,context=context)
                if translated.startswith(SKIP_PREFIX):
                    reply(translated,caption,context,started)
                    continue
            elif gpu:
                translated=gpu.translate(text,context=context)
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
            if history is not None and decision!='SKIP':history.remember(caption)
            if history is None:
                trace(dict(event_id=caption.event_id,time_s=caption.time_s,source=observed,translation=translated,context=None,worker_ms=(time.perf_counter()-started)*1000))
            reply(SKIP_PREFIX+reason if decision=='SKIP' else translated,caption,context,started)
        except EOFError:
            if args.context_selected:history.close()
            if gpu:
                gpu.close()
            return
        except Exception as exc:
            send('ERROR: ' + str(exc))


if __name__ == '__main__':
    main()

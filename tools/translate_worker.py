"""Local EN -> CS translation. Length-prefixed UTF-8; GPU uses owned loopback API."""
from pathlib import Path
import argparse
import struct
import sys
import unicodedata
import os
sys.path.insert(0, str(Path(__file__).resolve().parent))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--backend', choices=['cpu', 'cuni-q3km', 'cuni-q8', 'cuni-q5km'], default='cpu')
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
    try:
        if args.backend == 'cpu':
            import ctranslate2
            import sentencepiece
            source = sentencepiece.SentencePieceProcessor(model_file=str(args.model / 'source.spm'))
            target = sentencepiece.SentencePieceProcessor(model_file=str(args.model / 'target.spm'))
            model = ctranslate2.Translator(str(args.model), device='cpu', compute_type='int8', inter_threads=1, intra_threads=2)
        else:
            from subtitle_gpu import GpuTranslator
            gpu = GpuTranslator(args.model.parent, args.backend)
    except Exception as exc:
        send('ERROR: ' + str(exc))
        return
    send('READY')
    while True:
        try:
            length, = struct.unpack('<I', exact(4))
            if length > 4096:
                raise ValueError('Input exceeds subtitle limit')
            text = unicodedata.normalize('NFKC', exact(length).decode('utf-8'))
            if gpu:
                send(gpu.translate(text))
            else:
                tokens = source.encode(text, out_type=str) + ['</s>']
                result = model.translate_batch([tokens], beam_size=2, max_decoding_length=192,
                                               repetition_penalty=1.1)[0].hypotheses[0]
                send(target.decode(result))
        except EOFError:
            if gpu:
                gpu.close()
            return
        except Exception as exc:
            send('ERROR: ' + str(exc))


if __name__ == '__main__':
    main()

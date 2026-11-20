"""Exercise the real CPU worker with and without CTranslate2 automatic EOS."""
import argparse
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--app', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = a.app / 'translation'
sys.path.insert(0, str(root / 'deps'))
dlls = os.add_dll_directory(str(a.app / 'runtime'))
import ctranslate2
import sentencepiece

inputs = [
    'Do not open the red door.',
    'You must not leave without Anna.',
    'We have five minutes before the next class starts.',
    'Can you turn it off?',
    'Follow the road to the station. Wait for me there.',
    'The train leaves at eight, not nine.',
    'I did not say that Peter stole the car.',
    'I will help you, but you must wait here.',
]
source = sentencepiece.SentencePieceProcessor(model_file=str(root / 'model/source.spm'))
target = sentencepiece.SentencePieceProcessor(model_file=str(root / 'model/target.spm'))
config = json.loads((root / 'model/config.json').read_text())
assert config['add_source_eos'] is True
model = ctranslate2.Translator(str(root / 'model'), device='cpu', compute_type='int8', inter_threads=1, intra_threads=2)
expected = [target.decode(model.translate_batch([source.encode(t, out_type=str)], beam_size=2,
            max_decoding_length=192, repetition_penalty=1.1)[0].hypotheses[0]) for t in inputs]
del model
payload = b''.join(struct.pack('<I', len(t.encode())) + t.encode() for t in inputs)

def run(model_path):
    process = subprocess.run([str(root / 'python/python.exe'), '-B', '-I', str(root / 'translate_worker.py'),
            '--model', str(model_path), '--backend', 'cpu'], input=payload, capture_output=True,
            timeout=120, creationflags=subprocess.CREATE_NO_WINDOW, check=True)
    output, pos, messages = process.stdout, 0, []
    while pos < len(output):
        length, = struct.unpack_from('<I', output, pos)
        messages.append(output[pos + 4:pos + 4 + length].decode())
        pos += 4 + length
    assert messages[0] == 'READY', messages
    assert messages[1:] == expected, messages
    return messages[1:]

automatic = run(root / 'model')
with tempfile.TemporaryDirectory(prefix='framelab-eos-') as temp:
    alternate = Path(temp) / 'model'
    shutil.copytree(root / 'model', alternate)
    config['add_source_eos'] = False
    (alternate / 'config.json').write_text(json.dumps(config))
    explicit = run(alternate)
a.output.write_text(json.dumps(dict(passed=True, ctranslate2=ctranslate2.__version__,
    scope='Worker IPC/tokenization regression; not a translation-quality benchmark',
    rows=[dict(en=t, reference=e, automatic=x, explicit=y) for t, e, x, y in zip(inputs, expected, automatic, explicit)]),
    ensure_ascii=False, indent=2), encoding='utf-8')
print('PASS: 16 real CPU worker translations; exactly one EOS with automatic and manual configurations')

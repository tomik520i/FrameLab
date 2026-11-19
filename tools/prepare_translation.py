"""Developer setup for the fully bundled offline EN-CS translator. Never publishes."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def fetch(url, path, expected):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_file() and digest(path) == expected:
        return
    temporary = path.with_suffix(path.suffix + '.download')
    urllib.request.urlretrieve(url, temporary)
    if digest(temporary) != expected:
        raise RuntimeError('Publisher checksum mismatch: ' + path.name)
    temporary.replace(path)


def extract(archive, directory):
    directory = directory.resolve()
    with zipfile.ZipFile(archive) as source:
        for info in source.infolist():
            path = (directory / info.filename).resolve()
            if not path.is_relative_to(directory):
                raise RuntimeError('Unsafe archive path')
            if info.is_dir():
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(source.read(info))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'app/translation')
    parser.add_argument('--cache', type=Path, default=ROOT / 'downloads/translation')
    args = parser.parse_args()
    pins = json.loads((ROOT / 'docs/translation-components.json').read_text(encoding='utf-8'))
    out, cache = args.output.resolve(), args.cache.resolve()
    out.mkdir(parents=True, exist_ok=True)
    wheels = cache / 'wheels'
    for item in pins['packages']:
        path = wheels / item['url'].rsplit('/', 1)[-1]
        fetch(item['url'], path, item['sha256'])
    subprocess.run([sys.executable, '-m', 'pip', 'install', '--no-index', '--find-links', str(wheels),
                    '--target', str(out / 'deps'), 'ctranslate2==4.9.0',
                    'sentencepiece==0.2.3', 'numpy==2.3.5', 'pyyaml==6.0.3'], check=True)
    python_zip = cache / 'python-3.11.9-embed-amd64.zip'
    fetch(pins['python']['url'], python_zip, pins['python']['sha256'])
    extract(python_zip, out / 'python')
    (out / 'python/python311._pth').write_text('python311.zip\n.\n../deps\n', encoding='utf-8')
    model_zip = cache / 'opus-2019-12-18.zip'
    fetch(pins['model']['source'], model_zip, pins['model']['archive_sha256'])
    original = cache / 'original'
    extract(model_zip, original)
    sys.path.insert(0, str(out / 'deps'))
    import ctranslate2
    model = out / 'model'
    ctranslate2.converters.MarianConverter(
        str(original / 'opus.spm32k-spm32k.transformer-align.model1.npz.best-perplexity.npz'),
        [str(original / 'opus.spm32k-spm32k.vocab.yml')]
    ).convert(str(model), quantization='int8', force=True)
    for name in ['source.spm', 'target.spm', 'LICENSE', 'README.md']:
        shutil.copy2(original / name, model / name)
    for file in ['translate_worker.py', 'subtitle_models.py', 'subtitle_model_manager.py', 'subtitle_gpu.py']:
        shutil.copy2(ROOT / 'tools' / file, out / file)
    shutil.copy2(ROOT / 'docs/subtitle-models.json', out / 'subtitle-models.json')
    pins['model']['files'] = {p.name: digest(p) for p in model.iterdir() if p.is_file()}
    (out / 'provenance.json').write_text(json.dumps(pins, indent=2), encoding='utf-8')
    (out / 'README-LICENSES.txt').write_text(
        'Helsinki-NLP / OPUS-MT English-Czech opus-2019-12-18. License: CC BY 4.0, '
        'model/LICENSE. Source: ' + pins['model']['source'] + '\n'
        'FrameLab modification: converted Marian NPZ to CTranslate2 4.9.0 int8, no retraining. '
        'Original SentencePiece files retained. No publisher endorsement. '
        'CTranslate2 MIT; SentencePiece Apache-2.0; NumPy and PyYAML notices in deps; '
        'Python license in python/LICENSE.txt. Windows OCR is supplied by Windows.\n', encoding='utf-8')
    print('Offline translation component prepared:', out)


if __name__ == '__main__':
    main()

"""Developer packaging step: fetch hash-pinned OCR once, bundle for offline use."""
from pathlib import Path
import argparse
import json
import shutil
import subprocess
import sys
from prepare_translation import fetch, digest

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cache', type=Path, default=ROOT / 'downloads/ocr')
    args = parser.parse_args()
    pins = json.loads((ROOT / 'docs/ocr-components.json').read_text(encoding='utf-8'))
    out, cache = args.output.resolve(), args.cache.resolve()
    out.mkdir(parents=True, exist_ok=True)
    for package in pins['packages']:
        fetch(package['url'], cache / package['url'].rsplit('/', 1)[-1], package['sha256'])
    subprocess.run([sys.executable, '-m', 'pip', 'install', '--no-index', '--no-deps',
                    '--find-links', str(cache), '--target', str(out / 'deps')]
                   + [p['name']+'=='+p['version'] for p in pins['packages']], check=True)
    (out / 'models').mkdir(exist_ok=True)
    for model in pins['models']:
        target = out / 'models' / model['name']
        if 'url' in model:
            original = cache / model['name']
            fetch(model['url'], original, model['sha256'])
        else:
            original = out / 'deps' / model['source']
        assert digest(original) == model['sha256'], model['name']
        shutil.copy2(original, target)
    shutil.copy2(ROOT / 'tools/ocr_worker.py', out / 'ocr_worker.py')
    shutil.copy2(ROOT / 'tools/ocr_style.py', out / 'ocr_style.py')
    shutil.copy2(ROOT / 'tools/subtitle_dialogue.py', out / 'subtitle_dialogue.py')
    shutil.copy2(ROOT / 'tools/subtitle_languages.py', out / 'subtitle_languages.py')
    shutil.copy2(ROOT / 'docs/subtitle-languages.json', out / 'subtitle-languages.json')
    for name in ['RapidOCR-LICENSE.txt','PaddleOCR-LICENSE.txt']:
        shutil.copy2(ROOT / 'docs' / name, out / name)
    (out / 'provenance.json').write_text(json.dumps(pins, indent=2)+'\n', encoding='utf-8')
    (out / 'README-LICENSES.txt').write_text(
        'RapidOCR https://github.com/RapidAI/RapidOCR, Apache-2.0. '
        'PP-OCRv4 detection + PP-OCRv5 English recognition from RapidAI / PaddleOCR, '
        'Apache-2.0: https://github.com/PaddlePaddle/PaddleOCR . '
        'Unmodified ONNX weights; FrameLab tunes detection scale, threads and subtitle timing. '
        'All pinned sources and SHA-256 are in provenance.json. '
        'Dependencies retain their licenses in deps/*.dist-info; ONNX Runtime MIT, '
        'OpenCV Apache-2.0, NumPy/Pillow BSD-style, Shapely BSD, PyYAML MIT, '
        'pyclipper MIT, six MIT, tqdm MPL-2.0/MIT, flatbuffers Apache-2.0, '
        'protobuf BSD, packaging Apache-2.0/BSD, colorama BSD. '
        'Uses the bundled Python in ../translation/python; Python license there. '
        'No runtime downloads, no Windows OCR capability needed.\n', encoding='utf-8')
    print('Bundled OCR ready:', out)


if __name__ == '__main__':
    main()

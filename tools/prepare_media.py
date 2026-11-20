"""Bundle a hash-pinned, official portable LibVLC runtime; no system installation."""
import argparse
import hashlib
import json
import shutil
import subprocess
import os
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cache', type=Path, default=ROOT / 'downloads/vlc')
    args = parser.parse_args()
    pin = json.loads((ROOT / 'docs/media-runtime.json').read_text(encoding='utf-8'))
    args.cache.mkdir(parents=True, exist_ok=True)
    archive = args.cache / pin['url'].rsplit('/', 1)[-1]
    if not archive.exists():
        part = archive.with_suffix('.part')
        with urllib.request.urlopen(pin['url'], timeout=60) as source, part.open('wb') as out:
            shutil.copyfileobj(source, out)
        part.replace(archive)
    with archive.open('rb') as f:
        if hashlib.file_digest(f, 'sha256').hexdigest() != pin['sha256']:
            raise RuntimeError('LibVLC archive checksum mismatch')
    target = args.output.resolve()
    target.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as z:
        prefix = 'vlc-' + pin['version'] + '/'
        for info in z.infolist():
            if not info.filename.startswith(prefix) or info.is_dir():
                continue
            rel = info.filename[len(prefix):]
            if rel not in ['libvlc.dll', 'libvlccore.dll', 'vlc-cache-gen.exe', 'COPYING.txt', 'AUTHORS.txt', 'README.txt'] and not rel.startswith('plugins/'):
                continue
            path = (target / rel).resolve()
            if not path.is_relative_to(target) or (info.external_attr >> 16) & 0o170000 == 0o120000:
                raise RuntimeError('Unsafe archive entry')
            path.parent.mkdir(parents=True, exist_ok=True)
            with z.open(info) as src, path.open('wb') as dst:
                shutil.copyfileobj(src, dst)
    if os.name == 'nt':
        subprocess.run([str(target / 'vlc-cache-gen.exe'), str(target / 'plugins')], check=True, creationflags=subprocess.CREATE_NO_WINDOW)
    (target / 'provenance.json').write_text(json.dumps(pin, indent=2) + '\n', encoding='utf-8')
    (target / 'SOURCE-AND-LICENSE.txt').write_text(
        'Unmodified VideoLAN VLC ' + pin['version'] + ' Windows x64 libraries and plugins.\n'
        'LibVLC library: LGPL 2.1 or later; VLC plugins include GPL components.\n'
        'The complete upstream license is included in COPYING.txt.\n'
        'Corresponding upstream source: ' + pin['source'] + '\n'
        'API reference: https://videolan.videolan.me/vlc-3.0/\n'
        'FrameLab loads the shared libraries dynamically. No VLC system installation is required.\n', encoding='utf-8')
    print('Portable media runtime ready:', target)

if __name__ == '__main__':
    main()

"""Prepare the public SDK; optionally stage an existing local runtime archive.

NR runtimes are never downloaded automatically. Local staging records provenance,
not a license grant; see docs/NR-LICENSING-STATUS.txt.
"""
from pathlib import Path
import argparse
import hashlib
import json
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
REVISION = '9981c6350dc58c0ce21f024fd65e31e851c31689'
ARCHIVE = 'Veyra-2.1.9-NVIDIA-win64-portable.zip'
URL = 'https://github.com/Likely7/Veyra-NRVideo/releases/download/v2.1.9/' + ARCHIVE
SHA256 = '4225ecb11a8e96c2826b4b0b798feb811d7ae237f3edf0b627f3b6895c79f362'

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path,
                        help='Optional existing local archive; no NR download or license grant')
    parser.add_argument('--app-dir', type=Path, default=ROOT / 'app')
    args = parser.parse_args()
    if args.archive is not None:
        if not args.archive.is_file():
            parser.error('--archive must be an existing local file; NR automatic download is disabled')
        if digest(args.archive) != SHA256:
            raise RuntimeError('Veyra archive checksum mismatch; extraction refused')

    sdk_files = ['include/nvsdk_ngx.h', 'include/nvsdk_ngx_defs.h',
                 'include/nvsdk_ngx_params.h', 'include/nvsdk_ngx_defs_dlssd.h',
                 'include/nvsdk_ngx_defs_dlssg.h',
                 'lib/Windows_x86_64/x64/nvsdk_ngx_s.lib', 'LICENSE.txt']
    for name in sdk_files:
        target = ROOT / 'deps' / 'dlss' / name
        target.parent.mkdir(parents=True, exist_ok=True)
        urllib.request.urlretrieve(f'https://raw.githubusercontent.com/NVIDIA/DLSS/{REVISION}/{name}', target)

    if args.archive is None:
        print('Public SDK prepared. NR runtimes were not downloaded or staged.')
        return

    selected = {
        'runtime/experimental/nr-ampere/nvngx_dlssnr.dll': 'runtime/nr-sf/nvngx_dlssnr.dll',
        'runtime/experimental/nr-original/nvngx_dlssnr.dll': 'runtime/nr-original/nvngx_dlssnr.dll',
    }
    vfg = {'cudart64_12.dll', 'NVCVImage.dll', 'nvngxruntime.dll',
           'nvVFXVideoFrameGeneration.dll', 'NVVideoEffects.dll'}
    app = args.app_dir.resolve()
    manifest = {'source_url': URL, 'archive_sha256': SHA256, 'sdk_revision': REVISION,
                'nr_redistribution_rights': 'not established; see docs/NR-LICENSING-STATUS.txt',
                'files': []}
    with zipfile.ZipFile(args.archive) as archive:
        for member in archive.infolist():
            if member.is_dir():
                continue
            name = member.filename.removeprefix('Veyra-2.1.9-NVIDIA-win64-portable/')
            destination = selected.get(name)
            if name.startswith('runtime/nvidia-vfg/') and Path(name).name in vfg:
                destination = 'runtime/' + Path(name).name
            if name.startswith('licenses/') and ('NVIDIA' in name.upper() or 'NV' in Path(name).name.upper()):
                destination = name
            if not destination:
                continue
            target = (app / destination).resolve()
            if not target.is_relative_to(app):
                raise RuntimeError('Unsafe archive path')
            data = archive.read(member)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            manifest['files'].append({'file': destination, 'source_entry': member.filename,
                                      'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    (app / 'runtime-provenance.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('External runtimes prepared. Build the local host separately; see docs/experimental-nr.md.')

if __name__ == '__main__':
    main()

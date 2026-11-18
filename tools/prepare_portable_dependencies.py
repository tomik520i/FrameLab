"""Fetch pinned, unmodified portable components from their publishers.

Outputs stay under ignored deps/ and downloads/. No system installation or PATH change.
"""
from pathlib import Path
import hashlib, json, shutil, subprocess, urllib.request, zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'deps/portable'
DOWNLOADS = ROOT / 'downloads'


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def fetch(url, name, expected=None):
    path = DOWNLOADS / name
    if not path.exists():
        partial = path.with_suffix(path.suffix + '.partial')
        with urllib.request.urlopen(url, timeout=60) as response, partial.open('wb') as stream:
            shutil.copyfileobj(response, stream, 4 * 1024 * 1024)
        if expected and sha(partial) != expected:
            raise ValueError(f'Publisher hash mismatch: {name}')
        partial.replace(path)
    if expected and sha(path) != expected:
        raise ValueError(f'Cached hash mismatch: {name}')
    return path


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    DOWNLOADS.mkdir(exist_ok=True)
    components = []
    name = 'python-3.11.9-embed-amd64.zip'
    url = 'https://www.python.org/ftp/python/3.11.9/' + name
    archive = fetch(url, name)
    python = OUT / 'python'
    with zipfile.ZipFile(archive) as z:
        z.extractall(python)
    # Isolated embedded interpreter. Export scripts explicitly insert deps/python.
    components.append({'component': 'CPython 3.11.9 Windows x64 embedded', 'url': url,
                       'sha256': sha(archive), 'license': 'PSF',
                       'source': 'https://www.python.org/ftp/python/3.11.9/Python-3.11.9.tar.xz'})
    for package, version, filename, expected in [
        ('pillow', '13.0.0', 'pillow-13.0.0-cp311-cp311-win_amd64.whl',
         'd7710eb676e9bb68965cb045be0d333b4c0d7f9dfbaa7fbf121385b1588683d3'),
        ('imageio-ffmpeg', '0.6.0', 'imageio_ffmpeg-0.6.0-py3-none-win_amd64.whl',
         '02fa47c83703c37df6bfe4896aab339013f62bf02c5ebf2dce6da56af04ffc0a'),
    ]:
        with urllib.request.urlopen(f'https://pypi.org/pypi/{package}/{version}/json', timeout=30) as r:
            metadata = json.load(r)
        wheel = next(f for f in metadata['urls'] if f['filename'] == filename)
        assert wheel['digests']['sha256'] == expected
        archive = fetch(wheel['url'], filename, expected)
        with zipfile.ZipFile(archive) as z:
            z.extractall(OUT / 'packages')
        components.append({'component': f'{package} {version}', 'url': wheel['url'],
                           'sha256': expected, 'license': 'See included wheel dist-info licenses',
                           'source': f'https://pypi.org/project/{package}/{version}/#files'})
    wheel_name = 'nvidia_vfx-0.2.0.0-cp311-cp311-win_amd64.whl'
    url = 'https://pypi.nvidia.com/nvidia-vfx/' + wheel_name
    expected = '5aaf6a42bc6b6dbbf52fcb714194c994a6893cbbf7ada38bc2165a1f83e4a6fc'
    wheel = fetch(url, wheel_name, expected)
    existing = json.loads((ROOT / 'docs/nvidia-runtime-manifest.json').read_text())
    selected = {f['name']: f['sha256'] for f in existing['files']}
    runtime = OUT / 'nvidia-runtime'
    runtime.mkdir(exist_ok=True)
    with zipfile.ZipFile(wheel) as z:
        for name, wanted in selected.items():
            matches = [n for n in z.namelist() if n.rsplit('/', 1)[-1] == name]
            assert len(matches) == 1, (name, matches)
            data = z.read(matches[0])
            assert hashlib.sha256(data).hexdigest() == wanted, name
            (runtime / name).write_bytes(data)
        licenses = [n for n in z.namelist() if '/licenses/' in n.lower() and not n.endswith('/')]
        assert licenses, 'Official NVIDIA wheel must include license files'
        for member in licenses:
            target = OUT / 'licenses/nvidia-vfg' / Path(member).name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(z.read(member))
    manifest = {'runtime': 'NVIDIA VFX SDK 1.3 / nvidia-vfx 0.2.0.0', 'publisher_url': url,
                'publisher_wheel_sha256': expected, 'modified': False,
                'windows_gpu_architectures': ['Ada (RTX 40)', 'Blackwell (RTX 50)'],
                'hardware_tested': 'RTX 5070 Ti only', 'python_required_for_vfg': False,
                'license_directory': '../../licenses/nvidia-vfg',
                'distribution_context': 'Optional NVIDIA component of the FrameLab application, under separate NVIDIA terms; not GPL.',
                'files': [{'name': name, 'sha256': value, 'bytes': (runtime/name).stat().st_size}
                          for name, value in selected.items()]}
    (runtime / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    components.append({'component': manifest['runtime'], 'url': url, 'sha256': expected,
                       'license': 'NVIDIA Software License Agreement + AI Product-Specific Terms'})
    (OUT / 'components.json').write_text(json.dumps(components, indent=2), encoding='utf-8')
    ffmpeg = next((OUT/'packages/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
    (OUT/'licenses/ffmpeg-build.txt').write_bytes(subprocess.check_output([str(ffmpeg),'-version']))
    (OUT/'licenses/ffmpeg-license.txt').write_bytes(subprocess.check_output([str(ffmpeg),'-L'],stderr=subprocess.STDOUT))
    shutil.copy2(ROOT/'LICENSE',OUT/'licenses/FFmpeg-GPL-3.0.txt')
    print(json.dumps({'directory': str(OUT), 'components': components}, indent=2), flush=True)


if __name__ == '__main__':
    main()

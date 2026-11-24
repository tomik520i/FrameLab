"""Pinned local subtitle assets. No installer, cloud inference or system service.

Downloads are resumable and streamed, then verified in full before activation.
This module is also used by the portable helper and its offline tests.
"""
from pathlib import Path
import hashlib
import json
import os
import shutil
import time
import urllib.request
import zipfile


def atomic_json(path, value):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    os.replace(temporary, path)


def fingerprint(path):
    stat = path.stat()
    return {'bytes': stat.st_size, 'mtime_ns': stat.st_mtime_ns}


def verified(path, asset):
    try:
        marker = json.loads(path.with_suffix(path.suffix + '.verified.json').read_text())
        return (marker['sha256'] == asset['sha256'] and marker['bytes'] == asset['bytes']
                and marker['file'] == fingerprint(path))
    except (OSError, ValueError, KeyError):
        return False


def verify(path, asset, progress=lambda *args: None, cancelled=lambda: False):
    if path.stat().st_size != asset['bytes']:
        raise ValueError('Unexpected asset size: ' + path.name)
    sha = hashlib.sha256()
    checked = 0
    with path.open('rb') as stream:
        while block := stream.read(8 * 1024 * 1024):
            if cancelled():
                raise InterruptedError('Paused; partial download retained')
            sha.update(block)
            checked += len(block)
            progress('verifying', checked, asset['bytes'], path.name)
    if sha.hexdigest() != asset['sha256']:
        raise ValueError('SHA-256 mismatch: ' + path.name + '. Delete the partial file and retry.')
    atomic_json(path.with_suffix(path.suffix + '.verified.json'),
                {'sha256': sha.hexdigest(), 'bytes': checked, 'file': fingerprint(path)})


def download(asset, folder, progress=lambda *args: None, cancelled=lambda: False):
    folder.mkdir(parents=True, exist_ok=True)
    target = folder / asset['file']
    if verified(target, asset):
        return target
    if target.exists():
        verify(target, asset, progress, cancelled)
        return target
    partial = target.with_suffix(target.suffix + '.partial')
    current = partial.stat().st_size if partial.exists() else 0
    if current > asset['bytes']:
        raise ValueError('Oversized partial file: ' + str(partial))
    if shutil.disk_usage(folder).free < asset['bytes'] - current + 256 * 1024 ** 2:
        raise OSError('Not enough disk space for ' + target.name)
    for attempt in range(4):
        try:
            if cancelled():
                raise InterruptedError('Paused; partial download retained')
            if current == asset['bytes']:
                break
            headers = {'User-Agent': 'FrameLab-subtitles/1', 'Accept-Encoding': 'identity'}
            if current:
                headers['Range'] = f'bytes={current}-'
            with urllib.request.urlopen(urllib.request.Request(asset['url'], headers=headers), timeout=20) as response:
                if current and response.status == 200:
                    current = 0
                elif current and not response.headers.get('Content-Range', '').startswith(f'bytes {current}-'):
                    raise ValueError('Server returned an inconsistent resume range')
                with partial.open('ab' if current else 'wb') as stream:
                    while block := response.read(1024 * 1024):
                        if cancelled():
                            raise InterruptedError('Paused; partial download retained')
                        if current + len(block) > asset['bytes']:
                            raise ValueError('Download exceeds pinned asset size')
                        stream.write(block)
                        current += len(block)
                        progress('downloading', current, asset['bytes'], target.name)
            break
        except (InterruptedError, ValueError):
            raise
        except OSError:
            if attempt == 3:
                raise
            current = partial.stat().st_size if partial.exists() else 0
            for _ in range(10):
                if cancelled():
                    raise InterruptedError('Paused; partial download retained')
                time.sleep(.1)
    verify(partial, asset, progress, cancelled)
    os.replace(partial, target)
    os.replace(partial.with_suffix(partial.suffix + '.verified.json'), target.with_suffix(target.suffix + '.verified.json'))
    return target


def install_runtime(archive, destination, progress=lambda *args: None, cancelled=lambda: False):
    """Extract a verified, pinned archive. Never execute before extraction completes."""
    staging = destination.with_name(destination.name + '.extracting')
    staging.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive) as z:
        entries = z.infolist()
        total = sum(x.file_size for x in entries)
        if shutil.disk_usage(staging).free < total + 256 * 1024 ** 2:
            raise OSError('Not enough space to extract local inference runtime')
        done = 0
        for item in entries:
            if cancelled():
                raise InterruptedError('Paused; runtime will be extracted on resume')
            file = (staging / item.filename).resolve()
            if not file.is_relative_to(staging.resolve()) or (item.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('Unsafe runtime archive entry')
            if item.is_dir():
                file.mkdir(parents=True, exist_ok=True)
                continue
            file.parent.mkdir(parents=True, exist_ok=True)
            with z.open(item) as source, file.open('wb') as target:
                while block := source.read(4 * 1024 * 1024):
                    if cancelled():
                        raise InterruptedError('Paused; runtime will be extracted on resume')
                    target.write(block)
                    done += len(block)
                    progress('extracting', done, total, item.filename)
    if not (staging / 'ollama.exe').is_file():
        raise ValueError('Missing portable runtime executable')
    # An interrupted or modified installation can be repaired without deleting
    # user files. All directory moves stay inside this managed runtime's parent.
    backup = destination.with_name(destination.name + '.previous-' + str(time.time_ns()))
    parent = destination.parent.resolve()
    if any(p.is_symlink() or p.resolve().parent != parent for p in [staging, destination, backup]):
        raise ValueError('Runtime repair paths must stay in the managed directory')
    had_previous = destination.exists()
    if had_previous:
        os.replace(destination, backup)
    try:
        os.replace(staging, destination)
    except OSError:
        if had_previous:
            os.replace(backup, destination)
        raise


def catalog(root):
    return json.loads((root / 'subtitle-models.json').read_text(encoding='utf-8'))

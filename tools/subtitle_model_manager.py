"""Owned background download helper; communicates only through its private session."""
from pathlib import Path
import argparse
import json
import os
import threading
import time
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from subtitle_models import catalog, download, install_runtime, verified, atomic_json, fingerprint


def runtime_ready(root, data):
    exe = root / 'gpu-runtime' / 'ollama.exe'
    try:
        marker = json.loads((root / 'gpu-runtime' / 'verified.json').read_text())
        return marker['archive_sha256'] == data['runtime']['sha256'] and marker['executable'] == fingerprint(exe) and all((root/'gpu-runtime'/p).is_file() for p in ['lib/ollama/llama-server.exe','lib/ollama/vulkan/ggml-vulkan.dll'])
    except (OSError, KeyError, ValueError):
        return False


def model_status(root, model, data, previous):
    path = root / 'models' / model['file']
    if verified(path, model):
        phase = 'ready' if runtime_ready(root, data) else 'downloaded'
        return dict(phase=phase, done=model['bytes'], total=model['bytes'], detail='')
    # Keep actionable failure/pause details until the next explicit retry.
    if previous['phase'] in ('error', 'paused'):
        return previous
    if path.exists():
        return dict(phase='needs verification', done=0, total=model['bytes'],
                    detail='Use Download / resume to verify the local file; no download is needed if its SHA-256 matches.')
    return dict(phase='missing', done=0, total=model['bytes'], detail='')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--session', type=Path, required=True)
    args = p.parse_args()
    root, session = args.root, args.session
    data = catalog(root)
    state = {m['id']: {'phase': 'missing', 'done': 0, 'total': m['bytes'], 'detail': ''} for m in data['models']}
    cancelled = threading.Event()
    lock = threading.Lock()
    active = None
    last_command = ''
    last_refresh = 0.

    def refresh_devices():
        from subtitle_compute import discover
        def quoted(value):return '"'+str(value).replace('\\','\\\\').replace('"','\\"').replace('\n',' ').replace('\r',' ')+'"'
        rows=discover()
        temporary=session/'devices.tmp'
        temporary.write_text('\n'.join(' '.join([quoted(d['backend']),quoted(d['id']),quoted(d['name']),str(int(d['integrated'])),str(d['free']/1024**3 if d.get('free') is not None else -1)]) for d in rows),'utf-8')
        os.replace(temporary,session/'devices.txt')

    def update(key, phase, done, total, detail):
        with lock:
            state[key] = dict(phase=phase, done=done, total=total, detail=detail)

    def refresh():
        for m in data['models']:
            with lock:
                previous = state[m['id']].copy()
            current = model_status(root, m, data, previous)
            update(m['id'], current['phase'], current['done'], current['total'], current['detail'])

    def fetch(key):
        try:
            m = next(m for m in data['models'] if m['id'] == key)
            progress = lambda *values: update(key, *values)
            download(m, root / 'models', progress, cancelled.is_set)
            if not runtime_ready(root, data):
                archive = download(data['runtime'], root / 'downloads', progress, cancelled.is_set)
                install_runtime(archive, root / 'gpu-runtime', progress, cancelled.is_set)
                atomic_json(root / 'gpu-runtime' / 'verified.json', dict(archive_sha256=data['runtime']['sha256'], executable=fingerprint(root / 'gpu-runtime' / 'ollama.exe')))
            refresh()
        except InterruptedError as error:
            update(key, 'paused', 0, 0, str(error))
        except Exception as error:
            update(key, 'error', 0, 0, str(error))

    refresh_devices()
    refresh()
    while True:
        if (active is None or not active.is_alive()) and time.monotonic()-last_refresh > 2:
            refresh()
            last_refresh = time.monotonic()
        try:
            command = (session / 'command.txt').read_text(encoding='utf-8')
        except OSError:
            command = ''
        if command and command != last_command:
            last_command = command
            parts = command.split()
            if len(parts) >= 2:
                if parts[1] == 'quit':
                    cancelled.set()
                    return
                if parts[1] == 'cancel':
                    cancelled.set()
                elif parts[1] == 'devices':
                    refresh_devices()
                elif parts[1] == 'download' and len(parts) == 3 and parts[2] in state and (active is None or not active.is_alive()):
                    cancelled.clear()
                    active = threading.Thread(target=fetch, args=(parts[2],), daemon=True)
                    active.start()
                elif parts[1] == 'download' and len(parts) == 3 and parts[2] in state:
                    update(parts[2], 'error', 0, 0, 'Another download is running. Finish or pause it first.')
        def quoted(value):
            return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"').replace('\n', ' ').replace('\r', ' ') + '"'
        with lock:
            lines = [f'{quoted(key)} {quoted(s["phase"])} {s["done"]} {s["total"]} {quoted(s["detail"])}' for key, s in state.items()]
        temp = session / 'status.tmp'
        temp.write_text('\n'.join(lines), encoding='utf-8')
        try:
            os.replace(temp, session / 'status.txt')
        except PermissionError:
            pass  # Windows may briefly have a reader open; retry next tick.
        time.sleep(.2)


if __name__ == '__main__':
    main()

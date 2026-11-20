"""FrameLab-owned, loopback-only CUNI inference with explicit full GPU offload."""
from pathlib import Path
import json
import os
import shutil
import socket
import subprocess
import time
import urllib.request
import ctypes
from subtitle_models import catalog, verified, verify
from subtitle_model_manager import runtime_ready

PROMPT = '### Instruction:\nTranslate Input from English to Czech\n### Glossary:\n\n### Previous text:\n\n### Input:\n{text}\n### Response:\n'
OPTIONS = dict(temperature=0, num_ctx=2048, num_predict=256, num_gpu=99, num_thread=4,
               repeat_penalty=1.0, seed=20261119, top_k=20, top_p=.6, stop=['\n'])


def available_vram():
    """NVML physical free bytes on the primary NVIDIA GPU, or None if unavailable."""
    library = None
    initialized = False
    try:
        library = ctypes.WinDLL(str(Path(os.environ['SystemRoot']) / 'System32/nvml.dll'))
        if library.nvmlInit_v2() != 0:
            return None
        initialized = True
        device = ctypes.c_void_p()
        if library.nvmlDeviceGetHandleByIndex_v2(0, ctypes.byref(device)) != 0:
            return None
        class Memory(ctypes.Structure):
            _fields_ = [('total', ctypes.c_ulonglong), ('free', ctypes.c_ulonglong), ('used', ctypes.c_ulonglong)]
        memory = Memory()
        if library.nvmlDeviceGetMemoryInfo(device, ctypes.byref(memory)) != 0:
            return None
        return memory.free
    except (OSError, AttributeError, KeyError):
        return None
    finally:
        if initialized:
            library.nvmlShutdown()


class GpuTranslator:
    def __init__(self, root, model_id):
        self.process = None
        self.log = None
        self.opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        self.data = catalog(root)
        self.asset = next(m for m in self.data['models'] if m['id'] == model_id)
        self.name = 'framelab-' + model_id
        if not verified(root / 'models' / self.asset['file'], self.asset) or not runtime_ready(root, self.data):
            raise RuntimeError('Download and verify this model and runtime in Subtitles first.')
        free = available_vram()
        minimum = self.asset['bytes'] + int(.8 * 1024 ** 3)  # 2048-token KV, compute buffers and safety margin
        if free is not None and free < minimum:
            raise RuntimeError(f'Only {free/1024**3:.2f} GiB GPU memory is free; this model needs about {minimum/1024**3:.2f} GiB free. Choose Q3/CPU or reduce FG/enhancer resolution.')
        with socket.socket() as probe:
            probe.bind(('127.0.0.1', 0))
            port = probe.getsockname()[1]
        self.url = f'http://127.0.0.1:{port}'
        models = root / 'models' / 'ollama'
        blobs = models / 'blobs'
        blobs.mkdir(parents=True, exist_ok=True)
        blob = blobs / ('sha256-' + self.asset['sha256'])
        if not blob.exists():
            try:
                os.link(root / 'models' / self.asset['file'], blob)
            except OSError:
                if shutil.disk_usage(blobs).free < self.asset['bytes'] + 256 * 1024 ** 2:
                    raise RuntimeError('This drive needs extra space for the inference model copy. NTFS avoids it.')
                temporary = blob.with_suffix('.copying')
                shutil.copyfile(root / 'models' / self.asset['file'], temporary)
                os.replace(temporary, blob)
        env = os.environ.copy()
        env.update(OLLAMA_HOST=f'127.0.0.1:{port}', OLLAMA_MODELS=str(models), OLLAMA_NUM_PARALLEL='1',
                   OLLAMA_MAX_LOADED_MODELS='1', OLLAMA_FLASH_ATTENTION='1', OLLAMA_CONTEXT_LENGTH='2048',
                   OLLAMA_NO_CLOUD='1', OLLAMA_DEBUG='0')
        log_path = root.parent / 'logs' / 'subtitle-gpu-runtime.log'
        if log_path.exists() and log_path.stat().st_size > 8 * 1024 ** 2:
            os.replace(log_path, log_path.with_suffix('.previous.log'))
        self.log = log_path.open('a', encoding='utf-8')
        self.log.write('\n--- FrameLab GPU runtime start ' + time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()) + ' ---\n')
        self.log.flush()
        self.process = subprocess.Popen([str(root / 'gpu-runtime' / 'ollama.exe'), 'serve'], env=env,
                                        stdout=self.log, stderr=subprocess.STDOUT,
                                        creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            for _ in range(120):
                if self.process.poll() is not None:
                    raise RuntimeError('Local GPU runtime exited; see subtitle-gpu-runtime.log')
                try:
                    self.api('/api/version')
                    break
                except OSError:
                    time.sleep(.25)
            else:
                raise RuntimeError('Local GPU runtime did not start within 30 seconds')
            source = root / 'models' / self.asset['file']
            stamp = source.stat()
            try:
                self.api('/api/create', dict(model=self.name, files={self.asset['file']: 'sha256:' + self.asset['sha256']},
                         template='{{ .Prompt }}{{ .Response }}', parameters={'stop': ['\n']}, stream=False))
            finally:
                # Ollama touches blob mtime during import. With an NTFS hardlink
                # that also touches our verified download; preserve its metadata.
                os.utime(source, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
            verify(source, self.asset)
            self.translate('We should get moving before sunrise.', timeout=90)
            loaded = self.api('/api/ps')['models']
            if len(loaded) != 1 or loaded[0]['size_vram'] < loaded[0]['size']:
                raise RuntimeError('Model did not run fully on the GPU. Check the GPU driver/runtime and free VRAM. Choose Q3_K_M or CPU, or reduce FG/enhancer load.')
        except Exception:
            self.close()
            raise

    def api(self, path, payload=None, timeout=90):
        request = urllib.request.Request(self.url + path, data=None if payload is None else json.dumps(payload).encode('utf-8'), headers={'Content-Type': 'application/json'})
        with self.opener.open(request, timeout=timeout) as response:
            return json.load(response)

    def translate(self, text, timeout=25):
        result = self.api('/api/generate', dict(model=self.name, prompt=PROMPT.replace('{text}', text),
                         raw=True, stream=False, keep_alive=-1, options=OPTIONS), timeout=timeout)
        if result.get('done_reason') == 'length':
            raise RuntimeError('Translation exceeded the token limit; shorten the OCR region.')
        return result.get('response', '').strip()

    def close(self):
        if self.process:
            try:
                self.api('/api/generate', dict(model=self.name, keep_alive=0), timeout=5)
            except Exception:
                pass
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
            self.process = None
        if self.log:
            self.log.close()
            self.log = None

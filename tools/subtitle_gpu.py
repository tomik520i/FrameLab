"""FrameLab-owned CUNI / TranslateGemma on CUDA, Vulkan or CPU."""
from pathlib import Path
import json
import os
import shutil
import socket
import subprocess
import time
import urllib.request
from subtitle_models import catalog, verified, verify
from subtitle_model_manager import runtime_ready
from subtitle_languages import gemma_prompt, validate_pair
from subtitle_compute import choose, runtime_environment, label

PROMPT = '### Instruction:\nTranslate Input from English to Czech\n### Glossary:\n\n### Previous text:\n\n### Input:\n{text}\n### Response:\n'
OPTIONS = dict(temperature=0, num_ctx=2048, num_predict=256, num_gpu=99, num_thread=4,
               repeat_penalty=1.0, seed=20261119, top_k=20, top_p=.6, stop=['\n'])


class GpuTranslator:
    def __init__(self, root, model_id, source='en', target='cs', compute='auto', device='', allow_offload=False):
        # Ollama requires an absolute model store, including from CLI evaluators.
        root = Path(root).resolve()
        validate_pair(model_id, source, target)
        self.source, self.target = source, target
        self.gemma = model_id.startswith('translategemma-')
        self.device = choose(compute,device)
        self.compute = self.device['backend']
        self.allow_offload = bool(allow_offload)
        self.runtime_description = label(self.device)
        self.last_metrics = {}
        self.process = None
        self.log = None
        self.opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        self.data = catalog(root)
        self.asset = next(m for m in self.data['models'] if m['id'] == model_id)
        self.name = 'framelab-' + model_id
        if not verified(root / 'models' / self.asset['file'], self.asset) or not runtime_ready(root, self.data):
            raise RuntimeError('Download and verify this model and runtime in Subtitles first.')
        free = self.device.get('free') if self.compute=='cuda' and not allow_offload else None
        minimum = int(self.asset.get('estimated_vram_gib', self.asset['bytes']/1024**3+.8) * 1024 ** 3)
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
        if self.compute=='vulkan' and not (root/'gpu-runtime/lib/ollama/vulkan/ggml-vulkan.dll').is_file():
            raise RuntimeError('Vulkan runtime files are missing. Repair the local model runtime in Subtitles.')
        env = runtime_environment(os.environ,self.device)
        env.update(OLLAMA_HOST=f'127.0.0.1:{port}', OLLAMA_MODELS=str(models), OLLAMA_NUM_PARALLEL='1',
                   OLLAMA_MAX_LOADED_MODELS='1', OLLAMA_FLASH_ATTENTION='1' if self.compute=='cuda' else '0', OLLAMA_CONTEXT_LENGTH='2048',
                   OLLAMA_NO_CLOUD='1', OLLAMA_DEBUG='0', OLLAMA_LOAD_TIMEOUT='3m', OLLAMA_DEBUG_LOG_REQUESTS='0')
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
            # Startup cleanup removes blobs not yet referenced by a manifest.
            # Stage our verified model only after that cleanup and server startup.
            if not blob.exists():
                try:
                    os.link(source, blob)
                except OSError:
                    if shutil.disk_usage(blobs).free < self.asset['bytes'] + 256 * 1024 ** 2:
                        raise RuntimeError('This drive needs extra space for the inference model copy. NTFS avoids it.')
                    temporary = blob.with_suffix('.copying')
                    shutil.copyfile(source, temporary)
                    os.replace(temporary, blob)
            stamp = source.stat()
            try:
                self.api('/api/create', dict(model=self.name, files={self.asset['file']: 'sha256:' + self.asset['sha256']},
                         template='{{ .Prompt }}{{ .Response }}', parameters={'stop': ['<end_of_turn>', '<eos>'] if self.gemma else ['\n']}, stream=False))
            finally:
                # Ollama touches blob mtime during import. With an NTFS hardlink
                # that also touches our verified download; preserve its metadata.
                os.utime(source, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
            verify(source, self.asset)
            self.translate('We should get moving before sunrise.', timeout=180)
            loaded = self.api('/api/ps')['models']
            if len(loaded)!=1:raise RuntimeError('Unexpected local model state after loading.')
            self.loaded_info=loaded[0]
            gpu_bytes=loaded[0].get('size_vram',0);model_bytes=loaded[0].get('size',0)
            if self.compute=='cpu':
                if gpu_bytes:raise RuntimeError('CPU selection was not respected by the runtime.')
            elif not gpu_bytes or (not allow_offload and gpu_bytes<model_bytes):
                raise RuntimeError('Selected GPU could not load the model as requested. Choose a smaller model, allow GPU + RAM offload, or select CPU. No silent CPU fallback.')
            if self.compute!='cpu':self.runtime_description+=' / '+('GPU + RAM' if gpu_bytes<model_bytes else 'full GPU')
            from subtitle_models import atomic_json
            atomic_json(root.parent/'logs'/('subtitle-compute-'+str(os.getpid())+'.json'),dict(device=self.device,requested=compute,allow_offload=allow_offload,description=self.runtime_description,loaded=loaded[0]))
        except Exception:
            self.close()
            raise

    def api(self, path, payload=None, timeout=90):
        request = urllib.request.Request(self.url + path, data=None if payload is None else json.dumps(payload).encode('utf-8'), headers={'Content-Type': 'application/json'})
        with self.opener.open(request, timeout=timeout) as response:
            return json.load(response)

    def options(self):
        options=dict(OPTIONS)
        compute=getattr(self,'compute','cuda')
        options['num_gpu']=0 if compute=='cpu' else -1 if getattr(self,'allow_offload',False) else 99
        return options

    def translate(self, text, timeout=None, context=None):
        if timeout is None:timeout=25 if getattr(self,'compute','cuda')=='cuda' else 120
        gemma = getattr(self, 'gemma', False)
        options = self.options()
        if gemma:
            options.update(stop=['<end_of_turn>', '<eos>'], num_predict=384, top_k=1, top_p=1.0)
        prompt = gemma_prompt(text,self.source,self.target) if gemma else PROMPT.replace('{text}', text)
        if context is not None:
            from translation_timeline import temporal_prompt
            prompt=temporal_prompt('translategemma' if gemma else 'cuni',text,context,self.source,self.target)
        result = self.api('/api/generate', dict(model=self.name, prompt=prompt,
                         raw=True, stream=False, keep_alive=-1, options=options), timeout=timeout)
        self.last_metrics={k:result.get(k) for k in ('total_duration','load_duration','prompt_eval_count','prompt_eval_duration','eval_count','eval_duration','done_reason')}
        if result.get('done_reason') == 'length':
            raise RuntimeError('Translation exceeded the token limit; shorten the OCR region.')
        return result.get('response', '').strip()

    def generate_memory(self,instruction,data):
        if not self.gemma:raise ValueError('This specialized model cannot build scene memory.')
        # Translation-specific Gemma models are experimental here. Their own
        # model is used, with normal turn delimiters and no external summarizer.
        content=instruction+'\n'+json.dumps(data,ensure_ascii=False)
        for token in ('<start_of_turn>','<end_of_turn>','<bos>','<eos>'):content=content.replace(token,token.replace('<','‹').replace('>','›'))
        prompt='<start_of_turn>user\n'+content+'<end_of_turn>\n<start_of_turn>model\n'
        options=dict(self.options(),num_predict=256,top_k=1,top_p=1.,stop=['<end_of_turn>','<eos>'])
        result=self.api('/api/generate',dict(model=self.name,prompt=prompt,raw=True,stream=False,keep_alive=-1,options=options),timeout=90)
        if result.get('done_reason')=='length':raise RuntimeError('Memory exceeded its token limit')
        return result.get('response','').strip()

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

    def classify_content(self, text, timeout=None):
        if timeout is None:timeout=5 if self.compute=='cuda' else 30
        from subtitle_filter import classification_prompt
        if not self.gemma:
            return 'UNKNOWN'
        options=dict(self.options(),num_predict=12,top_k=1,top_p=1.0,stop=['<end_of_turn>','<eos>','\n'])
        result=self.api('/api/generate',dict(model=self.name,prompt=classification_prompt(text),raw=True,stream=False,keep_alive=-1,options=options),timeout=timeout)
        value=result.get('response','').strip()
        return value if result.get('done_reason')!='length' and value in ('KEEP','SKIP') else 'UNKNOWN'

"""Desktop continuation. Wait for import/download, own new processes and new logs."""
from pathlib import Path
import ctypes, datetime, hashlib, json, os, subprocess, sys, time, urllib.request
BASE=Path('H:/FrameLab-desktop');OUT=BASE/'benchmark';OLD=Path('H:/FrameLab-work/workspace/work/gpu-translation-eval')
APP=BASE/'baseline-preview12/app';URL='http://127.0.0.1:18783'
def write(name,data):
 (OUT/name).write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding='utf-8')
def api(path,payload=None):
 req=urllib.request.Request(URL+path,data=None if payload is None else json.dumps(payload).encode(),headers={'Content-Type':'application/json'})
 with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(req,timeout=300) as r:return json.load(r)
while json.loads(Path('H:/FrameLab-work/IMPORT-PROGRESS.json').read_text())['phase']!='complete':time.sleep(5)
assert (BASE/'checks/CPU-OCR-PASSED.json').is_file(), 'Verify actual CPU OCR before measuring coexistence'
assert json.loads((BASE/'models/CUNI-MH-Q8_0-progress.json').read_text())['phase']=='verified'
assert not (OUT/'RUNNING.json').exists(),'Never start a duplicate benchmark'
assert hashlib.sha256(Path('H:/FrameLab-work/workspace/work/translation-eval-300/corpus.jsonl').read_bytes()).hexdigest()=='addef2490cdb7429f7f3cfe2330a1a4178f2ee59a00b7c66afc561ae82c8f5cc'
try:
 api('/api/version')
 raise RuntimeError('Evaluation port already in use; will not use or stop another server')
except urllib.error.URLError:pass
write('RUNNING.json',{'pid':os.getpid(),'utc':datetime.datetime.now(datetime.timezone.utc).isoformat()})
env=os.environ.copy();env.update(OLLAMA_HOST='127.0.0.1:18783',OLLAMA_MODELS=str(OUT/'models'),OLLAMA_NUM_PARALLEL='1',OLLAMA_MAX_LOADED_MODELS='1',OLLAMA_FLASH_ATTENTION='1',OLLAMA_CONTEXT_LENGTH='2048',OLLAMA_NO_CLOUD='1',OLLAMA_DEBUG='1')
serverlog=(OUT/'ollama.log').open('w',encoding='utf-8')
server=subprocess.Popen([str(OLD/'ollama/ollama.exe'),'serve'],env=env,stdout=serverlog,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
app=None;window=[]
try:
 for _ in range(120):
  if server.poll() is not None:raise RuntimeError('Server exited')
  try:api('/api/version');break
  except OSError:time.sleep(.5)
 write('runtime-version.json',api('/api/version'))
 for quant,file,sha in [('cuni-q3km',OLD/'CUNI-MH.Q3_K_M.gguf','4e2c3440516a5340e261f96b664994f79be8835523fd496c1821b31d3a02893f'),('cuni-q8',BASE/'models/CUNI-MH.Q8_0.gguf','461b6c6e20b0fd5162fc916878b680590b8ee10799fd83c1eae6e7ab87d0f18c')]:
  blob=OUT/'models/blobs'/('sha256-'+sha);blob.parent.mkdir(parents=True,exist_ok=True)
  if not blob.exists():os.link(file,blob)
  model='framelab-cuni-mh-'+('q8' if quant=='cuni-q8' else 'q3km')
  stamp=file.stat()
  try:api('/api/create',dict(model=model,files={file.name:'sha256:'+sha},template='{{ .Prompt }}{{ .Response }}',parameters={'stop':['\n']},stream=False))
  finally:os.utime(file,ns=(stamp.st_atime_ns,stamp.st_mtime_ns))
  write(quant+'-model-info.json',api('/api/show',dict(model=model)))
 start=time.time()
 app=subprocess.Popen([str(APP/'FrameLab-preview12.exe'),'--sample','--fg','--medium','--nr','--nr-height','720','--subtitles','--seconds','4800'],cwd=APP)
 write('processes.json',dict(app_pid=app.pid,server_pid=server.pid,app=str(APP/'FrameLab-preview12.exe'),arguments='--sample --fg --medium --nr --nr-height 720 --subtitles --seconds 4800',nr_settings=(APP/'nr-settings.txt').read_text(),subtitle_settings=(APP/'subtitle-settings.txt').read_text()))
 # Exercise the application's F11 path without capturing the desktop.
 user32=ctypes.windll.user32;WNDPROC=ctypes.WINFUNCTYPE(ctypes.c_bool,ctypes.c_void_p,ctypes.c_void_p);window=[]
 @WNDPROC
 def find(hwnd,extra):
  pid=ctypes.c_ulong();user32.GetWindowThreadProcessId(ctypes.c_void_p(hwnd),ctypes.byref(pid))
  if pid.value==app.pid and user32.IsWindowVisible(ctypes.c_void_p(hwnd)):window.append(hwnd)
  return True
 for _ in range(120):
  if app.poll() is not None:raise RuntimeError('FrameLab exited during startup')
  user32.EnumWindows(find,0)
  if window:break
  time.sleep(.5)
 assert window,'No FrameLab window'
 user32.PostMessageW(ctypes.c_void_p(window[0]),0x100,0x7A,0)
 time.sleep(15)
 logs=[p for p in (APP/'logs').glob('*.csv') if p.stat().st_mtime>start]
 assert len(logs)==1,logs
 log=logs[0]
 write('app-log.json',{'path':str(log),'started':start,'fullscreen_requested':True,'no_screen_capture':True})
 for quant in ['cuni-q3km','cuni-q8']:
  path=OUT/(quant+'-console.log')
  with path.open('w',encoding='utf-8') as console:
   command=[sys.executable,str(OUT/'evaluate_cuni.py'),'--quant',quant,'--app-log',str(log)]
   run=subprocess.run(command,stdout=console,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
   if run.returncode:raise RuntimeError(quant+' failed; inspect console log')
 write('DESKTOP-COMPLETE.json',dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),models=['cuni-q3km','cuni-q8']))
finally:
 if app and app.poll() is None:
  if window:user32.PostMessageW(ctypes.c_void_p(window[0]),0x10,0,0)
  try:app.wait(timeout=20)
  except subprocess.TimeoutExpired:app.terminate()
 server.terminate()
 try:server.wait(timeout=20)
 except subprocess.TimeoutExpired:server.kill()
 serverlog.close()

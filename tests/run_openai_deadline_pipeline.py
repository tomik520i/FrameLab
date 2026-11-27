"""Fault injection through real workers and C++ queues; synthetic OCR, localhost HTTP only."""
from pathlib import Path
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import json, shutil, subprocess, threading, time, tempfile

import argparse
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--base-app',type=Path,required=True,help='Clean bundled app directory (Python and language/profile manifests)')
p.add_argument('--binary',type=Path,required=True,help='Built openai_deadline_pipeline_test.exe')
p.add_argument('--output',type=Path,required=True)
args=p.parse_args()
SOURCE=Path(__file__).resolve().parents[1]
ROOT=args.output.resolve();ROOT.mkdir(parents=True,exist_ok=True)
APP=Path(tempfile.mkdtemp(prefix='FrameLab-openai-test-',dir=ROOT))
LIVE=args.base_app.resolve()
(APP/'translation').mkdir();(APP/'ocr').mkdir()
for source in (SOURCE/'tools').glob('*.py'):shutil.copy2(source,APP/'translation'/source.name)
shutil.copy2(LIVE/'translation/subtitle-languages.json',APP/'translation/subtitle-languages.json')
shutil.copytree(LIVE/'translation/python',APP/'translation/python')
(APP/'ocr/ocr_worker.py').write_text('''import json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent
def exact(n):
 data=sys.stdin.buffer.read(n)
 if len(data)!=n:raise EOFError()
 return data
def send(text):
 b=text.encode('utf8');sys.stdout.buffer.write(struct.pack('<I',len(b))+b);sys.stdout.buffer.flush()
send('READY')
while True:
 try:
  w,h,n=struct.unpack('<III',exact(12));exact(n)
  p=root/'test-source.txt';text=p.read_text() if p.exists() else ''
  send('STYLE4 1 0 0 0 0 0 0\\n'+json.dumps(text)+'\\n')
 except EOFError:break
''',encoding='utf8')
calls=[]
class Server(BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_POST(self):
        p=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        content=p['input'][-1]['content']
        try:text=json.loads(content)['current_text']
        except (ValueError,KeyError,TypeError):text=content
        start=time.monotonic()
        context=json.loads(content).get('context') if content.startswith('{') else None
        calls.append(dict(run_id=p['model'],text=text,max_tokens=p['max_output_tokens'],context=context,received=start))
        if 'Slow subtitle' in text:time.sleep(4)
        answer=('INCOMPLETE SLOW ANSWER' if 'Slow subtitle' in text else 'Jason: Před čekáním.' if 'Before the delay' in text else 'Jason: Nejnovější titulek.' if 'Latest subtitle' in text else 'Lucia: Překlad pokračuje.')
        data=json.dumps(dict(status='completed',output=[dict(type='message',content=[dict(type='output_text',text=json.dumps(dict(translate=True,text=answer)))])],usage=dict(output_tokens=10)),ensure_ascii=False).encode()
        try:
            self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
        except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError):pass
server=ThreadingHTTPServer(('127.0.0.1',0),Server)
# Isolated fault injection only: redirect the API and substitute a dummy credential.
shutil.copy2(LIVE/'translation/openai-model-profiles.json',APP/'translation/openai-model-profiles.json')
with (APP/'translation/openai_translation.py').open('a',encoding='utf8') as fixture:
    fixture.write("\nAPI="+repr(f'http://127.0.0.1:{server.server_port}/v1/')+"\nread_key=lambda: 'fixture-not-a-real-key'\n_original_profile=profile\nprofile=lambda model: _original_profile('gpt-6-luna')\n")
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
try:
    run=subprocess.run([str(args.binary.resolve()),str(APP),str(server.server_port)],capture_output=True,text=True,encoding='utf8',timeout=65)
finally:
    server.shutdown();server.server_close();thread.join(2)
if (APP/'logs').is_dir():
    shutil.copytree(APP/'logs',ROOT/'pipeline-logs',dirs_exist_ok=True)
(ROOT/'pipeline-test.log').write_text(run.stdout+'\n'+run.stderr,encoding='utf8')
run_id=next(x[7:] for x in run.stdout.splitlines() if x.startswith('RUN_ID='))
external_calls=calls;calls=[c for c in calls if c['run_id']==run_id]
data=dict(native_run_id=run_id,all_observed_requests=external_calls,assertions_passed=False,returncode=run.returncode,calls=calls,synthetic_ocr=True,real_cpp_pipeline=True,real_python_translator=True,remote_requests=0,test_app=str(APP))
(ROOT/'pipeline-results.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf8')
print(run.stdout,run.stderr);print(json.dumps(data,ensure_ascii=False,indent=2))
assert run.returncode==0
assert sum('Slow subtitle' in c['text'] for c in calls)==1
assert not any('Old pending' in c['text'] for c in calls), 'Old queued captions were sent after timeout'
latest=next(c for c in calls if 'Latest subtitle' in c['text'])
slow=next(c for c in calls if 'Slow subtitle' in c['text'])
assert 1.8<latest['received']-slow['received']<2.7, 'Next caption did not start near two-second deadline'
assert 'Before the delay' in json.dumps(latest['context']), 'Recent history lost after timeout'
assert all(c['max_tokens']==1000 for c in calls)
events=[json.loads(line) for line in (APP/'logs/subtitle-lifecycle.jsonl').read_text().splitlines()]
assert sum(e['event']=='model_loading' for e in events)==1, 'Ordinary timeout restarted worker'
assert sum(e['event']=='translation_timeout' for e in events)==1
assert not any(e['event']=='translation_error' for e in events)
print('All two-second deadline, latest-only queue and history assertions passed.')

data.update(assertions_passed=True,next_request_after_timeout_s=latest['received']-slow['received'])
(ROOT/'pipeline-results.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf8')

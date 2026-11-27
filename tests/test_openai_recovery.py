import json,sys,time,threading,tempfile,unittest
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import openai_translation as api
from custom_ai_translation import CustomAiResponseError,CustomAiTimeoutError

def answer(text='Hotovo.'):
 return dict(status='completed',output=[dict(type='message',content=[dict(type='output_text',text=json.dumps(dict(translate=True,text=text)))])])
class Server(BaseHTTPRequestHandler):
 calls=[]
 def log_message(self,*args):pass
 def do_POST(self):
  p=json.loads(self.rfile.read(int(self.headers['Content-Length'])));self.calls.append(p)
  text=p['input'][-1]['content'];status=500 if text=='serverfail' else 401 if text=='authfail' else 429 if text=='quota' else 200
  data=answer()
  if text=='slow':time.sleep(2)
  if text.startswith('block'):time.sleep(.65)
  if text=='invalid':data=dict(status='incomplete',output=[])
  if text=='disconnect':self.connection.close();return
  if text=='memory':data=dict(status='completed',output=[dict(type='message',content=[dict(type='output_text',text='{"facts":[]}')])])
  raw=json.dumps(data).encode()
  try:
   self.send_response(status);self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
  except OSError:pass
class RecoveryTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.server=ThreadingHTTPServer(('127.0.0.1',0),Server);cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
  cls.replace=patch.object(api,'API',f'http://127.0.0.1:{cls.server.server_port}/v1/');cls.replace.start()
 @classmethod
 def tearDownClass(cls):cls.replace.stop();cls.server.shutdown();cls.server.server_close();cls.thread.join(2)
 def client(self,**kw):return api.OpenAiTranslator(key='fixture',timeout_s=1,max_tokens=1000,**kw)
 def test_transient_and_invalid_answer_continue(self):
  c=self.client()
  for text in ('serverfail','disconnect','invalid'):
   with self.assertRaises(CustomAiResponseError):c.translate(text)
   self.assertEqual(c.translate('next'),'Hotovo.')
 def test_credentials_and_quota_do_not_loop(self):
  for text in ('authfail','quota'):
   with self.assertRaises(api.ApiError):self.client().translate(text)
 def test_absolute_deadline_and_next_caption(self):
  c=self.client();start=time.monotonic()
  with self.assertRaises(CustomAiTimeoutError):c.translate('slow')
  self.assertLess(time.monotonic()-start,1.4);self.assertEqual(c.translate('next'),'Hotovo.')
 def test_all_speaker_blocks_share_budget(self):
  c=self.client();c.begin_caption();start=time.monotonic()
  try:
   self.assertEqual(c.translate('block one'),'Hotovo.')
   with self.assertRaises(CustomAiTimeoutError):c.translate('block two')
   self.assertLess(time.monotonic()-start,1.4)
  finally:c.end_caption()
  self.assertEqual(c.translate('next'),'Hotovo.')
 def test_budget_and_usage_log(self):
  with tempfile.TemporaryDirectory() as d:
   log=Path(d)/'usage.jsonl';c=self.client(usage_log=log);self.assertEqual(c.translate('fast'),'Hotovo.')
   self.assertEqual(Server.calls[-1]['max_output_tokens'],1000);self.assertTrue(log.is_file())
   with patch.object(api,'request',return_value=dict(status='completed',output=[dict(type='message',content=[dict(type='output_text',text='{"facts":[]}')])])) as send:
    self.assertEqual(c.generate_memory('Choose evidence',{}),'{"facts":[]}')
    self.assertEqual(send.call_args.args[2]['max_output_tokens'],1000)
    self.assertIsNotNone(send.call_args.kwargs['deadline'])
if __name__=='__main__':unittest.main()

import json,sys,time,threading,unittest
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from custom_ai_translation import CustomTranslator,CustomAiTimeoutError,request

class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_POST(self):
        data=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        text=data['messages'][-1]['content']
        body=json.dumps(dict(choices=[dict(finish_reason='stop',message=dict(content='Hotovo.'))])).encode()
        try:
            if 'headers' in text:time.sleep(2)
            if 'block' in text:time.sleep(.65)
            self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers()
            if 'drip' in text:
                for byte in body:self.wfile.write(bytes([byte]));self.wfile.flush();time.sleep(.07)
            else:self.wfile.write(body)
        except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError):pass

class DeadlineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
        cls.url=f'http://127.0.0.1:{cls.server.server_port}/v1'
    @classmethod
    def tearDownClass(cls):cls.server.shutdown();cls.server.server_close();cls.thread.join(2)
    def client(self):return CustomTranslator(dict(url=self.url,model='test',timeout_s=1,max_tokens=1000),read_credential=False)
    def test_delayed_headers_then_next_request(self):
        client=self.client();start=time.monotonic()
        with self.assertRaises(CustomAiTimeoutError):client.translate('headers')
        self.assertLess(time.monotonic()-start,1.35)
        self.assertEqual(client.translate('fast'),'Hotovo.')
    def test_dribbling_body_does_not_extend_deadline(self):
        start=time.monotonic()
        with self.assertRaises(CustomAiTimeoutError):self.client().translate('drip')
        self.assertLess(time.monotonic()-start,1.35)
    def test_all_speaker_blocks_share_one_budget(self):
        client=self.client();client.begin_caption();start=time.monotonic()
        self.assertEqual(client.translate('block one'),'Hotovo.')
        with self.assertRaises(CustomAiTimeoutError):client.translate('block two')
        self.assertLess(time.monotonic()-start,1.35)
        client.end_caption();self.assertEqual(client.translate('fast'),'Hotovo.')
    def test_expired_budget_sends_no_request(self):
        with self.assertRaises(CustomAiTimeoutError):request(self.url,'/chat/completions',payload={},deadline=time.monotonic()-1)

if __name__=='__main__':unittest.main()

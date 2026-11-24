"""Exercise the real framed worker loop without loading models or charging APIs."""
import io,json,runpy,struct,sys,types,unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from subtitle_context import CONTEXT_PREFIX
from translation_timeline import temporal_prompt
from openai_translation import build_request

class ContextIntegrationTests(unittest.TestCase):
    def test_worker_keeps_sources_only_filters_noise_and_resets(self):
        calls=[]
        class FakeGpu:
            runtime_description='fixture';gemma=False
            def __init__(self,*args):pass
            def translate(self,text,context=None):
                calls.append((text,context));return 'Překlad '+str(len(calls))
            def close(self):pass
        frames=[]
        for text,now,generation in [('The boat is stuck.',1,1),('AB 123 CD',2,1),('Meet me at the bank.',3,1),('He said ＂Go＂.',4,2)]:
            data=(CONTEXT_PREFIX+json.dumps(dict(text=text,observed=text,time_s=now,generation=generation))).encode()
            frames.append(struct.pack('<I',len(data))+data)
        stdin=types.SimpleNamespace(buffer=io.BytesIO(b''.join(frames)));output=io.BytesIO();stdout=types.SimpleNamespace(buffer=output)
        with patch.dict(sys.modules,{'subtitle_gpu':types.SimpleNamespace(GpuTranslator=FakeGpu)}),patch.object(sys,'stdin',stdin),patch.object(sys,'stdout',stdout),patch.object(sys,'argv',['translate_worker.py','--model','unused','--backend','cuni-q3km','--content-filter','1','--context-history']):
            runpy.run_path(str(ROOT/'tools/translate_worker.py'),run_name='__main__')
        self.assertEqual(len(calls),3)
        self.assertIsNone(calls[0][1]);self.assertIsNone(calls[2][1])
        self.assertEqual([r['source'] for r in calls[1][1]['history']],['The boat is stuck.'])
        self.assertEqual(calls[1][1]['history'][0]['seconds_since_ocr_observation'],2)
        self.assertNotIn('age_s',calls[1][1]['history'][0])
        self.assertNotIn('Překlad',json.dumps(calls[1][1],ensure_ascii=False))
        self.assertEqual(calls[2][0],'He said "Go".')
        output.seek(0);messages=[]
        while prefix:=output.read(4):messages.append(output.read(struct.unpack('<I',prefix)[0]).decode())
        self.assertTrue(messages[0].startswith('READY'))
        self.assertTrue(messages[2].startswith('FRAMELAB_SKIP_V1:'))

    def test_provider_prompts_preserve_current_text_and_target_language(self):
        text='A'*650+' THE END'
        context={'current_speaker':None,'verified_profiles':[],'history':[{'speaker':None,'source':'Earlier line.','age_s':2,'segment':1}]}
        for name,source,target in [('cuni','en','cs'),('translategemma','cs','ja')]:
            prompt=temporal_prompt(name,text,context,source,target)
            self.assertIn(text,prompt);self.assertIn('Earlier line.',prompt)
            if target=='ja':self.assertIn('Japanese',prompt);self.assertIn('Czech',prompt)
        body=build_request('gpt-6-luna','cs','ja',text,context=context)
        self.assertEqual(json.loads(body['input'][1]['content'])['current_text'],text)
        self.assertEqual(body['text']['format']['type'],'json_schema')
        plain=build_request('gpt-6-luna','cs','ja',text)
        self.assertEqual(plain['input'][1]['content'],text)

if __name__=='__main__':unittest.main()

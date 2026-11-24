import sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from custom_ai_translation import *

class CustomTests(unittest.TestCase):
    def config(self,**kw):return dict(url='http://127.0.0.1:8080/v1',model='/server/model.gguf',**kw)
    def test_endpoint_credentials_and_url_validation(self):
        self.assertEqual(credential_target('http://localhost:8080/'),'FrameLab/CustomAI/http://localhost:8080/v1')
        for url in ['file:///key','https://u:p@host/v1','http://host/v1?key=x','http://host/#part','http://host:99999']:
            with self.assertRaises(CustomAiError):base_url(url)
    def test_parameters_cannot_replace_input_or_add_tools(self):
        for params in [dict(messages=[]),dict(tools=[]),dict(options=dict(num_predict=99)),dict(temperature=float('nan'))]:
            with self.assertRaises(CustomAiError):validate_config(self.config(parameters=params))
    def test_memory_uses_same_server_model_and_parameters(self):
        c=CustomTranslator(self.config(parameters={'chat_template_kwargs':{'enable_thinking':False}},temperature=0),read_credential=False)
        result=dict(choices=[dict(finish_reason='stop',message=dict(content='{"facts":[]}'))])
        with patch('custom_ai_translation.request',return_value=result) as send:
            self.assertEqual(c.generate_memory('Select original evidence',{'observations':[]}),' {"facts":[]}'.strip())
        a,k=send.call_args
        self.assertEqual(a,('http://127.0.0.1:8080/v1','/chat/completions'))
        self.assertEqual(k['payload']['model'],'/server/model.gguf');self.assertIsNone(k['key'])
        self.assertEqual(k['payload']['chat_template_kwargs'],{'enable_thinking':False})
    def test_incomplete_reasoning_tools_and_wrong_shapes_not_displayed(self):
        for message,finish in [(dict(content='<think>unfinished'),'stop'),(dict(content='x',tool_calls=[{}]),'stop'),(dict(content='partial'),'length'),(dict(content=['text']),'stop')]:
            with self.assertRaises(CustomAiError):parse_response(dict(choices=[dict(finish_reason=finish,message=message)]))
        self.assertEqual(parse_response(dict(choices=[dict(finish_reason='stop',message=dict(content='<think>hidden</think> Ahoj.'))])),'Ahoj.')
    def test_multilingual_context_and_ollama(self):
        c=self.config(protocol='ollama');p=build_request(c,'cs','ja','Pozor!',context={'history':[]})
        self.assertIn('Japanese',p['messages'][0]['content']);self.assertIn('Czech',p['messages'][0]['content'])
        self.assertEqual(p['options']['num_predict'],256);self.assertFalse(p['stream'])

if __name__=='__main__':unittest.main()

import json, sys, unittest, importlib.util
from pathlib import Path
from unittest.mock import patch
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from custom_ai_translation import *

def response(text):return {'choices':[{'finish_reason':'stop','message':{'content':text}}]}
class PromptTests(unittest.TestCase):
    def config(self,**kwargs):return dict(url='http://127.0.0.1:8080/v1',model='fixture',**kwargs)
    def test_disabled_request_exactly_matches_installed_baseline(self):
        fixture=json.loads((root/'tests/fixtures/custom-default-requests.json').read_text('utf-8'))
        for case in fixture['cases']:
            new=dict(case['config'],prompt_enabled=False,instruction='Do not use this draft',response_format='translation_tags')
            self.assertEqual(case['request'],build_request(new,'en','cs','Wait!',filtering=case['filtering'],context=case['context']))
    def test_legacy_instruction_still_works_and_can_be_disabled(self):
        config=self.config(instruction='Translate to {target_language}.')
        self.assertTrue(validate_config(config)['prompt_enabled'])
        self.assertTrue(build_request(config,'en','cs','x')['messages'][0]['content'].startswith('Translate to Czech.'))
        self.assertNotIn('{target_language}',build_request(config,'en','cs','x')['messages'][0]['content'])
    def test_both_protocols_extract_multiline_and_keep_names(self):
        for protocol in ('openai','ollama'):
            c=CustomTranslator(self.config(protocol=protocol,prompt_enabled=True,response_format='translation_tags'),read_credential=False)
            text='Alex: Nenechávej mě tady!\nSam: Ani náhodou.'
            r=response('<translation>'+text+'</translation>') if protocol=='openai' else dict(done=True,message=dict(content='<translation>'+text+'</translation>'))
            with patch('custom_ai_translation.request',return_value=r) as send:
                self.assertEqual(c.translate('Alex: Do not leave!\nSam: No way.'),text)
            self.assertIn('<translation>',send.call_args.kwargs['payload']['messages'][0]['content'])
    def test_malformed_wrapper_is_recoverable_next_caption_succeeds(self):
        c=CustomTranslator(self.config(prompt_enabled=True,response_format='translation_tags'),read_credential=False)
        for bad in ['outside<translation>Ahoj</translation>','<translation>partial','<translation></translation>','<translation>A</translation><translation>B</translation>','<translation><translation>A</translation></translation>','plain']:
            with patch('custom_ai_translation.request',return_value=response(bad)):
                with self.assertRaises(CustomAiResponseError):c.translate('Hello')
        with patch('custom_ai_translation.request',return_value=response('<translation>Ahoj</translation>')):
            self.assertEqual(c.translate('Hello again'),'Ahoj')
    def test_filter_marker_and_memory_json_are_not_broken(self):
        c=CustomTranslator(self.config(prompt_enabled=True,response_format='translation_tags'),read_credential=False)
        for text in ('FRAMELAB_SKIP_V1:custom_non_dialogue','<translation>FRAMELAB_SKIP_V1:custom_non_dialogue</translation>'):
            with patch('custom_ai_translation.request',return_value=response(text)):
                self.assertEqual(c.translate('number plate'),'FRAMELAB_SKIP_V1:custom_non_dialogue')
        with patch('custom_ai_translation.request',return_value=response('{"facts":[]}')) as send:
            self.assertEqual(c.generate_memory('Select evidence',{'observations':[]}),' {"facts":[]}'.strip())
            self.assertEqual(send.call_args.kwargs['payload']['messages'][0]['content'],'Select evidence')
    def test_invalid_settings_rejected(self):
        for kw in [dict(prompt_enabled='true'),dict(response_format='json'),dict(instruction='x'*4097)]:
            with self.assertRaises(CustomAiError):validate_config(self.config(**kw))
    def test_cpp_default_matches_actual_backend(self):
        text=(root/'src/CustomPromptControls.hpp').read_text(encoding='utf-8')
        self.assertIn('defaultInstruction[]='+json.dumps(DEFAULT_INSTRUCTION)+';',text)

if __name__=='__main__':unittest.main()

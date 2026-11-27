import sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from custom_ai_translation import CustomTranslator,CustomAiError,CustomAiResponseError,build_request,parse_response,validate_config

class TokenLimits(unittest.TestCase):
    def config(self,**kwargs):return dict(url='http://localhost:8080/v1',model='test',**kwargs)
    def test_output_budget_has_no_old_hidden_cap(self):
        for protocol in ('openai','ollama'):
            for count in (1,256,1000,4096,32000,200000):
                p=build_request(self.config(protocol=protocol,max_tokens=count),'en','cs','Hello.')
                self.assertEqual(p.get('max_tokens') if protocol=='openai' else p['options']['num_predict'],count)
        for count in (0,-1,2147483648):
            with self.assertRaises(CustomAiError):validate_config(self.config(max_tokens=count))
    def test_memory_respects_user_budget_for_both_protocols(self):
        for protocol in ('openai','ollama'):
            c=CustomTranslator(self.config(protocol=protocol,max_tokens=4096),read_credential=False)
            response=dict(choices=[dict(finish_reason='stop',message=dict(content='{}'))]) if protocol=='openai' else dict(done=True,done_reason='stop',message=dict(content='{}'))
            with patch('custom_ai_translation.request',return_value=response) as request:
                self.assertEqual(c.generate_memory('Select original evidence',{}),'{}')
                p=request.call_args.kwargs['payload']
                self.assertEqual(p.get('max_tokens') if protocol=='openai' else p['options']['num_predict'],4096)
    def test_bad_response_carries_diagnostics_and_next_request_works(self):
        c=CustomTranslator(self.config(max_tokens=1000),read_credential=False)
        bad=dict(usage=dict(prompt_tokens=716,completion_tokens=1000),choices=[dict(finish_reason='length',message=dict(content='incomplete'))])
        good=dict(choices=[dict(finish_reason='stop',message=dict(content='Neubližuj mi!'))])
        with patch('custom_ai_translation.request',side_effect=[bad,good]) as request:
            with self.assertRaises(CustomAiResponseError) as caught:c.translate("Don't hurt me!")
            self.assertEqual(caught.exception.details['finish_reason'],'length')
            self.assertEqual(caught.exception.details['usage']['completion_tokens'],1000)
            self.assertEqual(caught.exception.details['max_tokens'],1000)
            self.assertEqual(c.translate("Don't hurt me!"),'Neubližuj mi!')
            self.assertEqual(request.call_count,2)
    def test_ollama_limit_and_tools_are_request_errors(self):
        for response in (dict(done=True,done_reason='length',eval_count=1000,message=dict(content='bad')),dict(done=True,done_reason='stop',message=dict(content='bad',tool_calls=[{}]))):
            with self.assertRaises(CustomAiResponseError):parse_response(response,'ollama',1000)

if __name__=='__main__':unittest.main()

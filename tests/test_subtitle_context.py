import json,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from subtitle_context import CaptionRequest,ContextPolicy,decode_request,CONTEXT_PREFIX,FILTER_PREFIX,observed_speaker

class ContextPolicyTests(unittest.TestCase):
    def test_fullwidth_quotes_are_normalized_after_json_parsing(self):
        body={'text':'He said ＂Go＂.','observed':'He said ＂Go＂.','time_s':1,'generation':1}
        result=decode_request(CONTEXT_PREFIX+json.dumps(body,ensure_ascii=False),history=True)
        self.assertEqual(result.text,'He said "Go".')
        self.assertEqual(decode_request(FILTER_PREFIX+json.dumps(body,ensure_ascii=False),filtering=True).text,result.text)

    def test_current_turn_is_not_its_own_past(self):
        p=ContextPolicy();a=CaptionRequest('The boat is stuck.','The boat is stuck.',1,1)
        self.assertIsNone(p.prepare(a));p.remember(a)
        b=CaptionRequest('Meet me at the bank.','Meet me at the bank.',2,1)
        context=p.prepare(b);self.assertEqual([x['source'] for x in context['history']],['The boat is stuck.'])

    def test_source_switch_and_clock_restart_clear_context(self):
        for now,generation in [(2,2),(.5,1)]:
            p=ContextPolicy();a=CaptionRequest('The boat is stuck.','The boat is stuck.',1,1);p.prepare(a);p.remember(a)
            self.assertIsNone(p.prepare(CaptionRequest('A bank loan.','A bank loan.',now,generation)))

    def test_old_speech_expires_despite_noise(self):
        p=ContextPolicy();a=CaptionRequest('The river.','The river.',0,1);p.prepare(a);p.remember(a)
        for i in range(1,70):
            r=CaptionRequest('J26 4B7','J26 4B7',i,1);p.prepare(r);p.remember(r)
        self.assertIsNone(p.prepare(CaptionRequest('The bank.','The bank.',70,1)))

    def test_long_current_input_is_not_truncated_to_make_room(self):
        p=ContextPolicy();a=CaptionRequest('Earlier line.','Earlier line.',0,1);p.prepare(a);p.remember(a)
        r=CaptionRequest('x'*1800+'END','x'*1800+'END',1,1)
        self.assertIsNone(p.prepare(r));self.assertTrue(r.text.endswith('END'));self.assertEqual(len(r.text),1803)

    def test_mentions_and_hud_are_not_speaker_labels(self):
        self.assertIsNone(observed_speaker('Jason is waiting.'));self.assertIsNone(observed_speaker('OBJECTIVE: Find Jason.'))
        self.assertIsNone(observed_speaker('In Italian: Hello.'));self.assertEqual(observed_speaker('Alex: I was late.'),'Alex')

    def test_unsafe_or_missing_clock_is_rejected(self):
        for time_s,generation in [(float('nan'),1),(1,True),(-1,1),(1,-1)]:
            raw=CONTEXT_PREFIX+json.dumps(dict(text='Hi',observed='Hi',time_s=time_s,generation=generation))
            with self.assertRaises(ValueError):decode_request(raw,history=True)
        with self.assertRaises(ValueError):decode_request('plain legacy request',history=True)

if __name__=='__main__':unittest.main()

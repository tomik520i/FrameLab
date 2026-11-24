import json,sys,unittest,threading,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from subtitle_context import CaptionRequest
from subtitle_evidence_memory import EvidenceMemory,parse_selection

class EvidenceMemoryTests(unittest.TestCase):
    def test_update_starts_after_accepting_second_caption_before_next_arrives(self):
        entered=threading.Event();seen=[]
        def generate(instruction,data):
            seen.append(data);entered.set();return '{"facts":[]}'
        p=EvidenceMemory(generate)
        try:
            first=CaptionRequest('First clue.','First clue.',1.,1,'a')
            second=CaptionRequest('Correction to the plan.','Correction to the plan.',6.,1,'b')
            p.prepare(first);p.remember(first);p.prepare(second)
            self.assertFalse(entered.is_set())
            p.remember(second)
            self.assertTrue(entered.wait(1))
            self.assertEqual([o['text'] for o in seen[0]['observations']],['First clue.','Correction to the plan.'])
            self.assertTrue(all(set(o)=={'id','text','speaker'} for o in seen[0]['observations']))
            self.assertEqual(p.pending['event_id'],'b')
        finally:p.close()

    def test_background_update_does_not_block_caption_or_leak_across_reset(self):
        entered=threading.Event();release=threading.Event();calls=[]
        def slow(*args):
            calls.append(1);entered.set();release.wait(2)
            return '{"facts":[{"text":"old scene","evidence":["o1"]}]}'
        p=EvidenceMemory(slow)
        try:
            for i in range(2):
                q=CaptionRequest(f'Old line {i}.',f'Old line {i}.',float(i),1,str(i));p.prepare(q)
                started=time.perf_counter();p.remember(q)
                self.assertLess(time.perf_counter()-started,.2)
            start=time.perf_counter();context=p.prepare(CaptionRequest('Now.','Now.',3.,1,'c'))
            self.assertLess(time.perf_counter()-start,.2);self.assertTrue(entered.wait(1));self.assertEqual(len(context['history']),2)
            p.prepare(CaptionRequest('New scene.','New scene.',4.,2,'d'))
            release.set();p.pending['future'].result(timeout=2)
            self.assertIsNone(p.prepare(CaptionRequest('Next.','Next.',5.,2,'e')))
            self.assertFalse(p.pins);self.assertEqual(len(calls),1);self.assertTrue(p.audit['discarded_stale_update'])
        finally:release.set();p.close()

    def test_original_evidence_survives_filler_and_never_leaks_generated_claims(self):
        calls=[]
        def generate(instruction,data):
            calls.append(data)
            return json.dumps({'facts':[{'text':'MODEL CLAIM NOT TO TRANSLATE','evidence':['o1']}]})
        policy=EvidenceMemory(generate,asynchronous=False)
        for i,text in enumerate(['We are digging the garden.','Bring the tool.']+[f'Filler line number {n}.' for n in range(22)]):
            q=CaptionRequest(text,text,float(i*2),1,str(i));policy.prepare(q);policy.remember(q)
        result=policy.prepare(CaptionRequest('Get that hoe.','Get that hoe.',50.,1,'target'))
        self.assertIn('We are digging the garden.',[r['source'] for r in result['history']])
        self.assertNotIn('MODEL CLAIM',json.dumps(result))
        self.assertLessEqual(len(result['history']),10)
        self.assertTrue(all('Get that hoe.' not in json.dumps(c) for c in calls))
        self.assertLessEqual(len(calls),7)
        self.assertEqual(result['history'][0]['seconds_since_ocr_observation'],50.)

    def test_invalid_evidence_falls_back_without_retry_and_generation_clears(self):
        calls=[]
        def bad(*args):calls.append(1);return '{"facts":[{"text":"fiction","evidence":["unknown"]}]}'
        p=EvidenceMemory(bad,asynchronous=False)
        for i in range(10):
            q=CaptionRequest(f'Line {i}.',f'Line {i}.',i*5.,1,str(i));p.prepare(q);p.remember(q)
        self.assertEqual(len(calls),1);self.assertIn('unknown',p.error);self.assertFalse(p.pins)
        self.assertIsNone(p.prepare(CaptionRequest('New scene.','New scene.',50.,2,'new')))
        self.assertEqual(p.error,'');self.assertFalse(p.recent)

    def test_idle_gap_and_backwards_clock_clear_all_memory(self):
        for end in [80.,0.]:
            p=EvidenceMemory(lambda *args:'{"facts":[]}')
            q=CaptionRequest('Old clue.','Old clue.',10.,1,'a');p.prepare(q);p.remember(q)
            self.assertIsNone(p.prepare(CaptionRequest('New line.','New line.',end,1,'b')))
            self.assertFalse(p.recent)

    def test_strict_schema_and_fenced_response(self):
        self.assertEqual(parse_selection('```json\n{"facts":[]}\n```',[]),{'facts':[]})
        for bad in ['Here is your memory: {"facts":[]}','{"facts":[],"extra":1}','{"facts":[{"text":"x","evidence":[4]}]}']:
            with self.assertRaises(ValueError):parse_selection(bad,[])

if __name__=='__main__':unittest.main()

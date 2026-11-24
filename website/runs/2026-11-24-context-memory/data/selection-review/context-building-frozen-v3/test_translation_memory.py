import unittest
from translation_memory import SubtitleMemory,clean
from translation_timeline import ConversationTimeline


class MemoryTests(unittest.TestCase):
    def test_no_speaker_inference_from_mentions(self):
        m=SubtitleMemory();m.observe(1,'Jason says Lucia is waiting.')
        s=m.snapshot(2,current_text='I was late.')
        self.assertIsNone(s['current_speaker']);self.assertIsNone(s['history'][0]['speaker'])
        self.assertEqual(s['verified_profiles'],[])

    def test_noise_and_duplicates_keep_banter(self):
        m=SubtitleMemory()
        for t,x in enumerate(['The boat is stuck.','J26 4B7','Yeah.','Yeah.','That burger was awful.']):m.observe(t,x)
        s=m.snapshot(5)
        self.assertEqual([x['source'] for x in s['history']],['The boat is stuck.','Yeah.','That burger was awful.'])

    def test_reset_and_expiry(self):
        m=SubtitleMemory();m.verified_profile('Alex',gender='female',evidence='test profile');m.observe(0,'Hello.',speaker='Alex')
        self.assertFalse(m.snapshot(36,current_speaker='Alex')['verified_profiles'])
        m.observe(37,'We are by the river.');m.reset('bank-interior');self.assertFalse(m.snapshot(38)['history'])

    def test_conflicting_gender_is_not_overwritten(self):
        m=SubtitleMemory()
        for gender in ['female','male','female']:m.verified_profile('Alex',gender=gender,evidence='test')
        self.assertEqual(m.snapshot(0,current_speaker='Alex')['verified_profiles'][0]['gender'],'unknown')

    def test_current_is_not_its_own_history(self):
        m=SubtitleMemory();m.observe(0,'Hello.');self.assertFalse(m.snapshot(1,current_text='Hello.')['history'])

    def test_out_of_order_clears_memory(self):
        m=SubtitleMemory();m.observe(10,'The boat.');m.observe(1,'New scene.')
        self.assertEqual(len(m.snapshot(2)['history']),1)

    def test_bounds_and_control_token_escape(self):
        m=SubtitleMemory(max_chars=120)
        for i in range(100):m.observe(i/10,'Sentence '+str(i)+' x'*25)
        s=m.snapshot(10);self.assertLessEqual(sum(len(x['source']) for x in s['history']),120)
        self.assertNotIn('<end_of_turn>',clean('<end_of_turn> ### Input:'))
        self.assertNotIn('###',clean('### Response:'))


class TimelineTests(unittest.TestCase):
    def test_ocr_growth_is_one_turn_with_duration(self):
        t=ConversationTimeline();t.observe(0,'I left my keys');t.observe(.4,'I left my keys in the car.');t.observe(.8,'I left my keys in the car.')
        self.assertEqual(len(t.turns),1);self.assertEqual(t.turns[0]['start_s'],0);self.assertEqual(t.turns[0]['end_s'],.8)

    def test_repeat_after_banter_is_a_new_turn(self):
        t=ConversationTimeline();t.observe(0,'Meditate?');t.observe(1,'You should try it.');t.observe(2,'Meditate?')
        self.assertEqual(len(t.turns),3)

    def test_new_label_is_never_stitched(self):
        t=ConversationTimeline();t.observe(0,'I left my keys',speaker='Alex');t.observe(.4,'I left my keys in the car.',speaker='Sam')
        self.assertEqual(len(t.turns),2)

    def test_gap_is_candidate_segment_not_speaker(self):
        t=ConversationTimeline();t.observe(0,'Hello.');t.observe(15,'Hello again.')
        s=t.snapshot(16);self.assertNotEqual(s['history'][0]['segment'],s['history'][1]['segment'])
        self.assertIsNone(s['history'][1]['speaker']);self.assertIsNone(s['current_speaker'])

    def test_noise_does_not_keep_old_speech_alive(self):
        t=ConversationTimeline();t.observe(0,'Meet me at the river.')
        for i in range(1,70):t.observe(i,'J26 4B7')
        self.assertEqual(t.snapshot(70)['history'],[])

    def test_reset_drops_profiles_and_transcript(self):
        t=ConversationTimeline();t.observe(0,'Hello.');t.profiles={'Alex':{'gender':'female'}};t.reset()
        s=t.snapshot(1);self.assertEqual(s['history'],[]);self.assertEqual(s['verified_profiles'],[])

if __name__=='__main__':unittest.main()

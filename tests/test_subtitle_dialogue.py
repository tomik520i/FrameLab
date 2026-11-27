import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from subtitle_dialogue import dialogue_text, DialogueTranslator
from subtitle_context import CaptionRequest, ContextPolicy
from subtitle_filter import ContentFilter, SKIP_PREFIX


def row(text, y, *, x=.2, width=.6, height=.05, rgb=(230,230,230), glyph=24, accepted=True):
    return dict(text=text, rect=[x,y,x+width,y+height], accepted=accepted,
                measured=dict(rgb=rgb,height=glyph))


class DialogueTests(unittest.TestCase):
    def test_two_speakers_and_soft_wraps(self):
        rows=[row('Jason: I told you',.10), row('not to shoot.',.16),
              row('Lucia: He has a gun!',.22)]
        self.assertEqual(dialogue_text(rows),'Jason: I told you not to shoot.\nLucia: He has a gun!')

    def test_gray_without_speaker_label(self):
        rows=[row('Do not open that door.',.1), row('Why not?',.17,rgb=(155,155,155),glyph=17)]
        self.assertEqual(dialogue_text(rows),'Do not open that door.\nWhy not?')

    def test_split_word_boxes_and_rejected_plate(self):
        rows=[row('Jason:',.1,x=.2,width=.1),row('Go!',.1,x=.31,width=.1),
              row('AB 123 CD',.2,accepted=False),row('Lucia: No!',.3)]
        self.assertEqual(dialogue_text(rows),'Jason: Go!\nLucia: No!')

    def test_dialogue_dash_and_gap(self):
        self.assertEqual(dialogue_text([row('- Go!',.1),row('- No!',.16)]),'- Go!\n- No!')
        self.assertEqual(dialogue_text([row('Wait here.',.1),row('Sure.',.3)]),'Wait here.\nSure.')

    def test_group_name_from_real_log(self):
        self.assertEqual(dialogue_text([row('Lucia: Fine, I got it.',.1),row("Kolt & Associates: Let's see it then!",.16)]),
                         "Lucia: Fine, I got it.\nKolt & Associates: Let's see it then!")

    def test_unchanged_main_cached_but_changed_reply_translated(self):
        d=DialogueTranslator();calls=[]
        def translate(text,context):calls.append(text);return 'CZ '+text
        def request(text,t=1,g=1):return CaptionRequest(text,text,t,g,'job')
        f=ContentFilter(1)
        d.translate(request('Jason: Wait!'),translate,f)
        value,_=d.translate(request('Jason: Wait!\nLucia: No!',2),translate,f)
        self.assertEqual(value,'CZ Jason: Wait!\nCZ Lucia: No!')
        self.assertEqual(calls,['Jason: Wait!','Lucia: No!'])
        value,_=d.translate(request('Jason: Wait!\nLucia: Go!',3),translate,f)
        self.assertEqual(calls[-1],'Lucia: Go!');self.assertEqual(len(calls),3)
        d.translate(request('Jason: Wait!\nLucia: Go!',4,2),translate,f)
        self.assertEqual(len(calls),5)  # Generation change never reuses stale text.
        d.translate(request('Jason: Wait!',20,2),translate,f)
        self.assertEqual(len(calls),6)

    def test_history_speaker_is_per_block(self):
        d=DialogueTranslator();history=ContextPolicy();calls=[]
        def translate(text,context):calls.append((text,context));return 'Překlad'
        c=CaptionRequest('Jason: Stop!\nLucia: No!','Jason: Stop!\nLucia: No!',1,1,'7')
        value,_=d.translate(c,translate,ContentFilter(1),history)
        self.assertEqual(value,'Překlad\nPřeklad')
        self.assertEqual(calls[1][1]['history'][0]['source'],'Jason: Stop!')
        self.assertEqual(calls[1][1]['history'][0]['speaker'],'Jason')
        self.assertNotIn('Lucia',calls[1][1]['history'][0]['source'])

    def test_filter_only_removes_noise_block(self):
        d=DialogueTranslator();calls=[]
        def translate(text,context):calls.append(text);return 'Ne!\nNechoď!'
        text='AB 123 CD\nLucia: No!'
        value,_=d.translate(CaptionRequest(text,text),translate,ContentFilter(1))
        self.assertEqual(value,'Ne! Nechoď!');self.assertEqual(calls,['Lucia: No!'])

    def test_model_skip_does_not_remove_other_speaker(self):
        d=DialogueTranslator();text='Jason: Go!\nLucia: No!'
        def translate(text,context):return SKIP_PREFIX+'label' if text.startswith('Jason') else 'Lucia: Ne!'
        value,_=d.translate(CaptionRequest(text,text),translate,ContentFilter(0))
        self.assertEqual(value,'Lucia: Ne!')

    def test_absent_block_removed(self):
        d=DialogueTranslator();calls=[]
        def translate(text,context):calls.append(text);return 'CZ'
        for t,text in enumerate(['Jason: Go!\nLucia: No!','Jason: Go!','Jason: Go!\nLucia: No!']):
            d.translate(CaptionRequest(text,text,t,1),translate,ContentFilter(0))
        self.assertEqual(calls,['Jason: Go!','Lucia: No!','Lucia: No!'])

    def test_failed_request_does_not_cache_partial_output(self):
        d=DialogueTranslator();calls=[]
        def translate(text,context):
            calls.append(text)
            if len(calls)==2:raise RuntimeError('connection lost')
            return 'CZ'
        request=CaptionRequest('Jason: Go!\nLucia: No!','Jason: Go!\nLucia: No!',1,1)
        with self.assertRaises(RuntimeError):d.translate(request,translate,ContentFilter(0))
        value,_=d.translate(request,translate,ContentFilter(0))
        self.assertEqual(value,'CZ\nCZ');self.assertEqual(len(calls),4)

if __name__=='__main__':unittest.main()

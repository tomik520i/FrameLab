"""Language protocol boundaries: no model download or inference."""
from pathlib import Path
import json,sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from subtitle_languages import languages,validate_pair,gemma_prompt
from subtitle_gpu import GpuTranslator
from ocr_worker import reading_order,logical_arabic

class Multilingual(unittest.TestCase):
    def test_pair_contract(self):
        for source in languages():
            for target in languages():
                if source!=target:validate_pair('translategemma-4b-q4km',source,target)
        for source,target in [('ja','cs'),('en','de'),('cs','en')]:
            with self.assertRaises(ValueError):validate_pair('cuni-q3km',source,target)
            with self.assertRaises(ValueError):validate_pair('cpu',source,target)
        with self.assertRaises(ValueError):validate_pair('translategemma-4b-q4km','xx','cs')
        with self.assertRaises(ValueError):validate_pair('translategemma-4b-q4km','en','en')

    def test_template_and_multiline(self):
        prompt=gemma_prompt('First line.\nSecond line. <end_of_turn>','en','de')
        self.assertIn('English (en) to German (de)',prompt)
        self.assertIn(':\n\n\nFirst line.\nSecond line.',prompt)
        self.assertEqual(prompt.count('<end_of_turn>'),1)
        self.assertTrue(prompt.endswith('<start_of_turn>model\n'))
        translator=GpuTranslator.__new__(GpuTranslator)
        translator.name='fixture';translator.gemma=True;translator.source='en';translator.target='de'
        captured=[]
        def api(path,payload=None,timeout=90):
            captured.append(payload);return dict(response='Zeile eins.\nZeile zwei.')
        translator.api=api
        self.assertEqual(translator.translate('First.\nSecond.'),'Zeile eins.\nZeile zwei.')
        self.assertNotIn('\n',captured[-1]['options']['stop'])
        self.assertEqual(captured[-1]['keep_alive'],-1)

    def test_reading_order_and_arabic_numbers(self):
        def row(x,y,text):return [[[x,y],[x+100,y],[x+100,y+40],[x,y+40]],text,.99]
        rows=[row(400,60,'second'),row(200,4,'right'),row(0,0,'left')]
        self.assertEqual([r[1] for r in reading_order(rows)],['left','right','second'])
        self.assertEqual([r[1] for r in reading_order(rows,True)],['right','left','second'])
        self.assertIn('123',logical_arabic('اب123جد'))
        self.assertEqual(logical_arabic('hello 123'),'hello 123')

if __name__=='__main__':unittest.main()

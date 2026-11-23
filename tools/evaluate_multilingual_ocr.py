"""Replay synthetic DirectWrite subtitle fixtures through the real bundled OCR.

This measures recognition, not game-domain accuracy or translation quality.
Generate fixtures with subtitle_multilang_test first.
"""
from pathlib import Path
import argparse,json,sys,time,unicodedata

def edit_distance(a,b):
    row=list(range(len(b)+1))
    for i,x in enumerate(a,1):
        nxt=[i]
        for j,y in enumerate(b,1):
            nxt.append(min(nxt[-1]+1,row[j]+1,row[j-1]+(x!=y)))
        row=nxt
    return row[-1]

def normalize(text):
    return ' '.join(unicodedata.normalize('NFKC',text).casefold().split())

def main():
    p=argparse.ArgumentParser();p.add_argument('--app',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    sys.path.insert(0,str(a.app/'ocr'));sys.path.insert(0,str(a.app/'ocr/deps'))
    import numpy as np
    from PIL import Image
    import ocr_worker
    from subtitle_languages import languages
    engines={};results=[]
    a.output.mkdir(parents=True,exist_ok=True)
    for line in (a.fixtures/'fixtures.tsv').read_text('utf-8').splitlines():
        code,size,name,missing,expected=line.split('\t')
        if code not in engines:
            engines.clear() # Avoid retaining every ONNX session at once.
            engines[code]=ocr_worker.create_engine(code)
            ocr_worker.recognize(engines[code],np.zeros((96,320,3),dtype=np.uint8))
        pixels=np.fromfile(a.fixtures/name,dtype=np.uint8).reshape(720,1280,4)
        Image.fromarray(pixels[:,:,[2,1,0]],'RGB').save(a.output/(name.removesuffix('.bgra')+'.png'))
        crop=np.ascontiguousarray(pixels[480:720,:,:3])
        start=time.perf_counter();text,confidence=ocr_worker.recognize(engines[code],crop);ms=1000*(time.perf_counter()-start)
        ref,hyp=normalize(expected),normalize(text)
        record=dict(language=code,size=int(size),expected=expected,ocr=text,confidence=confidence,ms=ms,cer=edit_distance(ref,hyp)/len(ref),missing_glyphs=int(missing))
        results.append(record)
        print(json.dumps(record,ensure_ascii=False),flush=True)
        (a.output/'ocr-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),'utf-8')
    # Ensure every advertised OCR family can initialize, including the broader
    # Cyrillic pack (the RU/UK fixtures above select the specialized eslav pack).
    families={x['ocr_model']:code for code,x in languages().items()}
    for model,code in families.items():
        engine=ocr_worker.create_engine(code);ocr_worker.recognize(engine,np.zeros((96,320,3),dtype=np.uint8));del engine
    print('PASS: all',len(families),'OCR families initialized',flush=True)

if __name__=='__main__':main()

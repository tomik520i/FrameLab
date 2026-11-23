"""Small English NER on CPU. Reject only a confident entity covering all text."""
from pathlib import Path
import sys,json,hashlib,os

class EntityFilter:
    def __init__(self,app):
        app=Path(app).resolve();folder=app/'translation/filter'
        model=folder/'model.onnx';tokenizer=folder/'tokenizer.json'
        if hashlib.sha256(model.read_bytes()).hexdigest()!='9419a876387ff2bbe5f21ab7429c7bef93eac86c50353390d4d8fca6e4a210d8':raise RuntimeError('Entity model checksum mismatch')
        sys.path.insert(0,str(folder/'deps'));sys.path.insert(0,str(app/'ocr/deps'))
        self.dll_dir=os.add_dll_directory(str(app/'runtime')) if os.name=='nt' else None
        import onnxruntime as ort
        from tokenizers import Tokenizer
        import numpy as np
        self.np=np;self.tokenizer=Tokenizer.from_file(str(tokenizer));self.tokenizer.enable_truncation(max_length=128)
        settings=ort.SessionOptions();settings.intra_op_num_threads=2;settings.inter_op_num_threads=1
        self.session=ort.InferenceSession(str(model),sess_options=settings,providers=['CPUExecutionProvider'])
        self.labels=json.loads((folder/'config.json').read_text('utf-8'))['id2label']
    def evidence(self,text):
        np=self.np;encoded=self.tokenizer.encode(text)
        if len(encoded.ids)>=128:return dict(score=0,kind='',coverage=0)
        inputs={'input_ids':np.array([encoded.ids],dtype=np.int64),'attention_mask':np.array([encoded.attention_mask],dtype=np.int64)}
        logits=self.session.run(None,inputs)[0][0];prob=np.exp(logits-logits.max(axis=1,keepdims=True));prob/=prob.sum(axis=1,keepdims=True)
        scores=[];kinds=set();covered=set();has_other=False
        for i,(start,end) in enumerate(encoded.offsets):
            if start==end or not any(c.isalnum() for c in text[start:end]):continue
            index=int(prob[i].argmax());label=self.labels[str(index)]
            if label=='O' or label[2:] not in ('PER','LOC','ORG'):has_other=True;continue
            scores.append(float(prob[i,index]));kinds.add(label[2:]);covered.update(j for j in range(start,end) if text[j].isalnum())
        length=sum(c.isalnum() for c in text);coverage=len(covered)/max(length,1)
        return dict(score=min(scores) if scores and not has_other and len(kinds)==1 and coverage==1 else 0,kind=next(iter(kinds)) if len(kinds)==1 else '',coverage=coverage)
    def rejects(self,text,threshold=.98):return self.evidence(text)['score']>=threshold

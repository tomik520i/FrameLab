"""Download a pinned, optional 66 MB English named-entity filter (Apache-2.0)."""
from pathlib import Path
import argparse,urllib.request,hashlib,json,io,zipfile

REVISION='3a19fe9404a4469d91aa3d551558a97f68872f67'
TOKENIZERS_URL='https://files.pythonhosted.org/packages/65/71/0670843133a43d43070abeb1949abfdef12a86d490bea9cd9e18e37c5ff7/tokenizers-0.22.2-cp39-abi3-win_amd64.whl'
TOKENIZERS_SHA256='c9ea31edff2968b44a88f97d784c2f16dc0729b8b143ed004699ebca91f05c48'
FILES=[('onnx/model_quantized.onnx','model.onnx',65772734,'sha256','9419a876387ff2bbe5f21ab7429c7bef93eac86c50353390d4d8fca6e4a210d8'),('config.json','config.json',965,'git','66a92793b02a539cba34b291e466ce4809bb23cf'),('tokenizer.json','tokenizer.json',669021,'git','de7ac9b6bda3ed337df237120512a771c1c3f519')]
def main():
 p=argparse.ArgumentParser();p.add_argument('--app',type=Path,required=True);a=p.parse_args();folder=a.app/'translation/filter';folder.mkdir(parents=True,exist_ok=True);manifest=[]
 for remote,filename,size,kind,expected in FILES:
  target=folder/filename
  if not target.exists():
   request=urllib.request.Request(f'https://huggingface.co/onnx-community/distilbert-NER-ONNX/resolve/{REVISION}/{remote}',headers={'User-Agent':'FrameLab-entity-filter/1'})
   temporary=target.with_suffix(target.suffix+'.partial');downloaded=0
   with urllib.request.urlopen(request,timeout=60) as response,temporary.open('wb') as out:
    while block:=response.read(1024*1024):out.write(block);downloaded+=len(block);print(filename,downloaded,'/',size,flush=True)
   temporary.replace(target)
  data=target.read_bytes();actual=hashlib.sha256(data).hexdigest() if kind=='sha256' else hashlib.sha1(f'blob {len(data)}\0'.encode()+data).hexdigest()
  if len(data)!=size or actual!=expected:raise RuntimeError('Entity filter integrity check failed: '+filename)
  manifest.append(dict(file=filename,bytes=size,sha256=hashlib.sha256(data).hexdigest()))
 # The standalone tokenizer API needs no hub client or network during playback.
 # Bundle its Windows/Python 3.9+ ABI wheel, including the original license.
 with urllib.request.urlopen(TOKENIZERS_URL,timeout=60) as response:wheel=response.read()
 if hashlib.sha256(wheel).hexdigest()!=TOKENIZERS_SHA256:raise RuntimeError('Tokenizer wheel checksum mismatch')
 with zipfile.ZipFile(io.BytesIO(wheel)) as archive:
  for entry in archive.infolist():
   relative=Path(entry.filename)
   if relative.is_absolute() or '..' in relative.parts or relative.parts[0] not in ('tokenizers','tokenizers-0.22.2.dist-info'):raise RuntimeError('Unexpected tokenizer wheel path')
   if entry.is_dir():continue
   target=folder/'deps'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(archive.read(entry))
   manifest.append(dict(file=target.relative_to(folder).as_posix(),bytes=target.stat().st_size,sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
 (folder/'provenance.json').write_text(json.dumps(dict(model='onnx-community/distilbert-NER-ONNX',revision=REVISION,original='https://huggingface.co/dslim/distilbert-NER',license='Apache-2.0',tokenizers=dict(url=TOKENIZERS_URL,sha256=TOKENIZERS_SHA256),files=manifest),indent=2),'utf-8')
 license_dir=folder/'licenses';license_dir.mkdir(exist_ok=True)
 with urllib.request.urlopen('https://www.apache.org/licenses/LICENSE-2.0.txt',timeout=30) as response:(license_dir/'Apache-2.0.txt').write_bytes(response.read())
 (license_dir/'NOTICE.txt').write_text('DistilBERT-NER: dslim/distilbert-NER, Apache-2.0. ONNX conversion: onnx-community/distilbert-NER-ONNX.\nOriginal: https://huggingface.co/dslim/distilbert-NER\nConversion: https://huggingface.co/onnx-community/distilbert-NER-ONNX/tree/'+REVISION+'\nTokenizer runtime: Hugging Face tokenizers 0.22.2, Apache-2.0 (license included in deps/tokenizers-0.22.2.dist-info/licenses).\n','utf-8')
 print('Verified entity model and license.',flush=True)
if __name__=='__main__':main()

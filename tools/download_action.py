import urllib.request,pathlib
p=pathlib.Path(__file__).resolve().parents[1]/'app/samples';p.mkdir(parents=True,exist_ok=True)
url='https://media.xiph.org/video/derf/twitch/y4m/CSGO.y4m'
with urllib.request.urlopen(urllib.request.Request(url,headers={'Range':'bytes=0-255'}),timeout=30) as r:
 header=r.read(256).split(b'\n')[0]+b'\n'
if header!=b'YUV4MPEG2 W1920 H1080 F60:1 Ip A0:0 C420jpeg\n':raise RuntimeError('Source format changed; refuse guessed byte offsets')
(p/'y4m-header.bin').write_bytes(header);size=1920*1080*3//2+6
start=len(header)+25*60*size;length=240*size
req=urllib.request.Request(url,headers={'Range':f'bytes={start}-{start+length-1}'})
with urllib.request.urlopen(req,timeout=30) as r:
 if r.status!=206 or not r.headers.get('Content-Range','').startswith(f'bytes {start}-{start+length-1}/'):raise RuntimeError('Server did not honor byte range')
 with (p/'csgo-action.y4m.partial').open('wb') as f:
  f.write(header);remaining=length
  while remaining:
   chunk=r.read(min(1048576,remaining))
   if not chunk:raise RuntimeError('Truncated range')
   f.write(chunk);remaining-=len(chunk)
(p/'csgo-action.y4m.partial').replace(p/'csgo-action.y4m')
print('Downloaded 240 original frames from seconds 25-29')

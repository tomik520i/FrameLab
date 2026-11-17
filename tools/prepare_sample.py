"""Prepare a repeatable public CS:GO clip; never synthesize reference frames."""
from pathlib import Path
import subprocess, json, hashlib
ROOT=Path(__file__).resolve().parents[1]
ffmpeg=next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
folder=ROOT/'app/samples';folder.mkdir(exist_ok=True)
source=folder/'csgo-reference.mkv'
url='https://media.xiph.org/video/derf/twitch/y4m/CSGO.y4m'
action=folder/'csgo-action.y4m'
if not action.exists() or action.stat().st_size!=746497485:
    raise RuntimeError('Run tools/download_action.py first; action sample missing or incomplete')
subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error','-i',str(action),'-an','-c:v','ffv1','-level','3','-y',str(source)],check=True)
subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error','-i',str(source),'-an','-pix_fmt','bgra','-f','rawvideo','-y',str(folder/'csgo-1080p60.bgra')],check=True)
subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error','-i',str(source),'-an','-c:v','libx264','-crf','16','-pix_fmt','yuv420p','-movflags','+faststart','-y',str(folder/'csgo-preview.mp4')],check=True)
(folder/'provenance.json').write_text(json.dumps({'source':url,'collection':'https://media.xiph.org/video/derf/','rights':'https://media.xiph.org/video/derf/twitch/copyright.txt','width':1920,'height':1080,'fps':60,'frames':(folder/'csgo-1080p60.bgra').stat().st_size//(1920*1080*4),'reference_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source_start_seconds':25 if action.exists() else 0,'notes':'Lossless FFV1 reference; decoded once to BGRA for all comparisons. Preview MP4 is not the benchmark reference.'},indent=2))
print((folder/'provenance.json').read_text())

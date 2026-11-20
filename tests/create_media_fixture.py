"""Generate only synthetic media; no screen recording or external content."""
import argparse
import subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--ffmpeg', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
for name, text in [('english', 'ENGLISH TRACK'), ('czech', 'CESKA STOPA')]:
    (a.output / (name + '.srt')).write_text('1\n00:00:00,000 --> 00:00:08,000\n' + text + '\n', encoding='utf-8')
subprocess.run([str(a.ffmpeg), '-hide_banner', '-loglevel', 'error', '-y',
    '-f', 'lavfi', '-i', 'color=c=red:s=320x180:r=24:d=8',
    '-f', 'lavfi', '-i', 'sine=frequency=440:duration=8',
    '-f', 'lavfi', '-i', 'sine=frequency=880:duration=8',
    '-i', str(a.output / 'english.srt'), '-i', str(a.output / 'czech.srt'),
    '-map', '0:v', '-map', '1:a', '-map', '2:a', '-map', '3:s', '-map', '4:s',
    '-vf', "drawbox=x=0:y=90:w=320:h=90:color=blue:t=fill,drawbox=x=280:y=0:w=40:h=40:color=0x00ff00:t=fill:enable='between(t,4,6)'",
    '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-c:a', 'aac', '-c:s', 'srt',
    '-metadata:s:a:0', 'language=eng', '-metadata:s:a:0', 'title=English 440 Hz',
    '-metadata:s:a:1', 'language=ces', '-metadata:s:a:1', 'title=Czech 880 Hz',
    '-metadata:s:s:0', 'language=eng', '-metadata:s:s:1', 'language=ces',
    str(a.output / 'multi-track.mkv')], check=True)
print('Created 8 s MKV with 2 audio tracks, 2 subtitles and a 4–6 s seek marker')

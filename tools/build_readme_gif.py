"""Build a small README GIF from the already verified source-aligned 2x movie.

Only select, resize, palette-quantize and retime existing frames. No interpolation.
The frame map identifies the exact source movie frame and recorded image IDs.
"""
from pathlib import Path
import csv, hashlib, json, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'deps/python'))
from PIL import Image, ImageDraw, ImageFont

def main():
    folder = ROOT/'release-assets/analysis-15s-82183953'
    source = folder/'analysis-15s-82183953_source-aligned_3s_4x-slow.mp4'
    source_map = list(csv.DictReader(source.with_suffix('.csv').open()))
    ffmpeg = next((ROOT/'deps/python/imageio_ffmpeg/binaries').glob('ffmpeg*.exe'))
    output = ROOT/'docs/images/framelab-2x-slow-comparison.gif'
    frame_map_path = output.with_name(output.stem+'-frames.csv')
    width, picture_h, header_h, footer_h = 768, 216, 48, 34
    height = picture_h + header_h + footer_h
    # Original source time 4.1..5.1 s = movie frames 264..503 (movie is already 4x slow).
    first, last, gif_fps, gif_frames = 264, 503, 40, 240
    # Retain every movie frame. Additional 1.5x slowdown makes one second last six.
    picked = [first+n for n in range(gif_frames)]
    decoder = subprocess.Popen([str(ffmpeg), '-v', 'error', '-threads', '2', '-i', str(source),
        '-vf', f'select=between(n\\,{first}\\,{last}),crop=1920:540:0:80,scale={width}:{picture_h}:flags=lanczos',
        '-fps_mode', 'passthrough', '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1'], stdout=subprocess.PIPE)
    encoder = subprocess.Popen([str(ffmpeg), '-v', 'error', '-filter_complex_threads', '1',
        '-f', 'rawvideo', '-pixel_format', 'rgb24', '-video_size', f'{width}x{height}',
        '-framerate', str(gif_fps), '-i', 'pipe:0',
        '-filter_complex', '[0:v]split[frames][palette];[palette]palettegen=max_colors=192:stats_mode=diff[p];[frames][p]paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle',
        '-loop', '0', '-final_delay', '2', '-gifflags', '+transdiff', '-y', str(output)], stdin=subprocess.PIPE)
    font = ImageFont.truetype('C:/Windows/Fonts/seguisb.ttf', 23)
    small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 18)
    canvas = Image.new('RGB', (width,height), '#0b1320')
    draw = ImageDraw.Draw(canvas)
    draw.text((14,9), 'RAW / ORIGINAL', font=font, fill='white')
    draw.text((width//2+14,9), 'FRAME GEN 2x', font=font, fill='#54dfbc')
    draw.text((14,height-28), '6x slow motion  |  same source time  |  NVIDIA VFG Medium', font=small, fill='#dce6f5')
    entries = []
    next_output = 0
    try:
        for movie_frame in range(first,last+1):
            data = decoder.stdout.read(width*picture_h*3)
            if len(data) != width*picture_h*3:
                raise RuntimeError(f'Incomplete decoded movie frame {movie_frame}')
            if next_output < gif_frames and movie_frame == picked[next_output]:
                canvas.paste(Image.frombytes('RGB',(width,picture_h),data),(0,header_h))
                encoder.stdin.write(canvas.tobytes())
                original = source_map[movie_frame]
                entries.append({'gif_frame':next_output,'gif_time_s':next_output/gif_fps,
                    'source_movie_frame':movie_frame,'capture_clock_s':original['capture_clock_s'],
                    'left_kind':original['left_kind'],'left_id':original['left_id'],
                    'right_kind':original['right_kind'],'right_id':original['right_id']})
                next_output += 1
        decoder.stdout.close()
        if decoder.wait() != 0: raise RuntimeError('Decoder failed')
        encoder.stdin.close()
        if encoder.wait() != 0: raise RuntimeError('GIF encoder failed')
    except BaseException:
        decoder.kill(); encoder.kill(); raise
    assert next_output == gif_frames
    with frame_map_path.open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=entries[0].keys());writer.writeheader();writer.writerows(entries)
    # GIF centisecond timing alternates 20/30 ms to represent 25 ms per movie frame.
    gif=Image.open(output)
    delays=[]
    for n in range(gif.n_frames):
        gif.seek(n);delays.append(gif.info['duration'])
    assert gif.n_frames == gif_frames and sum(delays)==6000 and set(delays)=={20,30}
    assert gif.size == (width,height) and gif.info['loop']==0
    source_keys={(r['right_kind'],r['right_id']) for r in source_map[first:last+1]}
    selected_keys={(r['right_kind'],r['right_id']) for r in entries}
    assert source_keys <= selected_keys, 'GIF sampling skipped a source-aligned output image ID'
    meta={'recording_id':'analysis-15s-82183953','settings_id':'analysis-15s-82183953_settings',
        'backend':'NVIDIA VFG','quality':'Medium','generation_multiplier':2,
        'input_dimensions':[2560,1440],'source_movie':source.name,
        'source_movie_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
        'source_interval_s':[4.1,5.1],'slowdown':6,'duration_s':6,'gif_fps':gif_fps,
        'gif_frames':gif_frames,'gif_delays_ms':sorted(set(delays)),'gif_size':[width,height],
        'gif_bytes':output.stat().st_size,'gif_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
        'frame_map':frame_map_path.name,'all_source_output_ids_retained':True,
        'selected_raw_ids_right':len({r['right_id'] for r in entries if r['right_kind']=='R'}),
        'selected_generated_ids_right':len({r['right_id'] for r in entries if r['right_kind']=='G'}),
        'transform':'Equal crop/resize of both recorded movie panels, slower playback, fixed labels outside images, GIF palette. No new interpolation.',
        'limits':['Source-aligned reconstruction removes scheduling delay and does not demonstrate input lag or physical scanout.',
                  'GIF reduces spatial and color detail; consult original PNGs/MP4s for artifact inspection.']}
    output.with_suffix('.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
    qa=ROOT/'analysis-output/readme-gif';qa.mkdir(parents=True,exist_ok=True)
    gif.seek(85);gif.convert('RGB').save(qa/'decoded-frame-85.png')
    print(json.dumps(meta,indent=2),flush=True)

if __name__=='__main__':
    main()

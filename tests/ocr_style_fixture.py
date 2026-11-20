"""Actual bundled OCR on mixed-style synthetic images; writes reviewable evidence."""
import argparse
import json
import sys
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--ocr', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
sys.path.insert(0, str(args.ocr / 'deps'))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import cv2
import ocr_worker
from ocr_style import learn_style, filter_rows

ocr_worker.ROOT = args.ocr
engine = ocr_worker.create_engine()
args.output.mkdir(parents=True, exist_ok=True)


def fixture(scale=1., color=(245, 245, 245), line_count=3):
    image = Image.new('RGB', (1280, 720))
    draw = ImageDraw.Draw(image)
    for y in range(720):
        draw.line((0, y, 1279, y), fill=(24 + y // 30, 38 + y // 25, 48 + y // 35))
    def word(text, xy, size, fill, stroke=1):
        font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', size)
        draw.text(xy, text, font=font, fill=fill, stroke_width=stroke, stroke_fill=(0, 0, 0), anchor='lt')
        return draw.textbbox(xy, text, font=font, anchor='lt')
    yellow = word('MISSION COMPLETE', (390, 80), 48, (255, 220, 35))
    word('SALE', (80, 280), 72, (245, 245, 245))
    word('DANGER AHEAD', (760, 340), 34, (245, 65, 55))
    word('SYSTEM MESSAGE', (60, 395), 34, (50, 160, 255))
    draw.rounded_rectangle((1030, 615, 1218, 656), radius=4, fill=(18, 18, 18))
    word('7AB 1234', (1050, 625), 18, (245, 245, 245), 0)
    expected = ['Follow the road to the station.', 'Do not open the red door.', 'Wait for me here.'][:line_count]
    sample = None
    for i, text in enumerate(expected):
        font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 34)
        x = int((1280 - draw.textlength(text, font=font)) / 2)
        word(text, (x, 490 + 46 * i), 34, color, 2)
        if i == 0:
            b = draw.textbbox((x, 490), 'Follow', font=font, anchor='lt')
            sample = [(b[0] - 3) / 1280, (b[1] - 3) / 720, (b[2] + 3) / 1280, (b[3] + 3) / 720]
    if scale != 1:
        image = image.resize((round(1280 * scale), round(720 * scale)), Image.Resampling.LANCZOS)
    return image, sample, expected, [v / (1280 if i % 2 == 0 else 720) for i, v in enumerate(yellow)]


image, sample, expected, yellow = fixture()
image.save(args.output / 'mixed-styles.png')
bgr = np.array(image)[:, :, ::-1].copy()
bgra = cv2.cvtColor(bgr, cv2.COLOR_BGR2BGRA)
bgra.tofile(args.output / 'mixed-styles.bgra')
(args.output / 'selection.txt').write_text(' '.join(str(v) for v in sample), encoding='utf-8')
profile = learn_style(bgr, sample, bgr.shape[0])
(args.output / 'profile.json').write_text(json.dumps(dict(profile=profile, sample=sample), indent=2), encoding='utf-8')
cases = []
for name, scale, color, lines in [('three-lines', 1., (245, 245, 245), 3), ('one-line', 1., (245, 245, 245), 1),
        ('two-lines-1080p', 1.5, (245, 245, 245), 2), ('slightly-dimmed', 1., (220, 220, 220), 3)]:
    image, _, expected, _ = fixture(scale, color, lines)
    bgr = np.array(image)[:, :, ::-1].copy()
    start = time.perf_counter()
    rows, _ = engine(bgr, use_cls=False)
    ocr_ms = (time.perf_counter() - start) * 1000
    start = time.perf_counter()
    result = filter_rows(bgr, rows, profile, bgr.shape[0], [0, .35, 1, .98])
    filter_ms = (time.perf_counter() - start) * 1000
    image.save(args.output / (name + '.png'))
    normalized = lambda text: ''.join(c.lower() for c in text if c.isalnum())
    success = normalized(result['text']) == normalized(' '.join(expected))
    cases.append(dict(name=name, success=success, expected=expected, ocr_ms=ocr_ms, filter_ms=filter_ms, **result))
    draw = ImageDraw.Draw(image)
    for row in result['rows']:
        rect = [v * (image.width if i % 2 == 0 else image.height) for i, v in enumerate(row['rect'])]
        draw.rectangle(rect, outline=(120, 255, 70) if row['accepted'] else (255, 105, 85), width=2)
    rect = [v * (image.width if i % 2 == 0 else image.height) for i, v in enumerate(sample)]
    draw.rectangle(rect, outline=(70, 200, 255), width=3)
    image.save(args.output / (name + '-decisions.png'))
    print(name, success, result['text'], f'OCR {ocr_ms:.1f} ms; filter {filter_ms:.1f} ms', flush=True)

image, _, _, yellow = fixture()
bgr = np.array(image)[:, :, ::-1].copy()
yellow_profile = learn_style(bgr, yellow, bgr.shape[0])
rows, _ = engine(bgr, use_cls=False)
result = filter_rows(bgr, rows, yellow_profile, bgr.shape[0])
cases.append(dict(name='select-yellow-instead', success=result['text'] == 'MISSION COMPLETE', **result))
print('select-yellow-instead', result['text'], flush=True)
(args.output / 'results.json').write_text(json.dumps(cases, ensure_ascii=False, indent=2), encoding='utf-8')
assert all(case['success'] for case in cases), 'Inspect results.json: selected style did not match expected subtitles'
print('PASS: actual OCR, sample word, 1/2/3 lines, normalized height at 1080p, dimmed colour, different selected style')

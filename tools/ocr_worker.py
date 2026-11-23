"""Offline subtitle OCR. Pixels arrive through bounded, process-local shared memory.

RapidOCR + PP-OCRv5 English run on two CPU threads; no downloads or network at runtime.
The GPU remains available to frame generation, enhancement and translation.
"""
from pathlib import Path
import argparse
import json
import mmap
import os
import struct
import sys
import re

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'deps'))
CAPACITY = 16 * 1024 * 1024


def create_engine(language='en'):
    import cv2
    from rapidocr_onnxruntime import RapidOCR
    from subtitle_languages import languages
    item=languages().get(language)
    if item is None:
        raise ValueError('Unknown OCR language: '+language)
    recognition=ROOT/'models'/item['ocr_model']
    if not recognition.is_file():
        raise FileNotFoundError('Missing OCR language pack: '+item['name']+'. Extract the complete multilingual package.')
    cv2.setNumThreads(2)
    engine = RapidOCR(
        intra_op_num_threads=2, inter_op_num_threads=1, use_cls=False,
        det_limit_type='min', det_limit_side_len=480,
        det_model_path=str(ROOT / 'models/ch_PP-OCRv4_det_infer.onnx'),
        rec_model_path=str(recognition),
        text_score=0.70,
    )
    if item.get('rtl'):
        # PaddleOCR's Arabic recognizers emit visual order. Restore logical
        # Unicode order, keeping Latin/number runs intact (PaddleOCR Apache-2.0).
        decoder = engine.text_rec.postprocess_op
        class ArabicDecoder:
            def __call__(self,*args,**kwargs):
                result=decoder(*args,**kwargs)
                return [(logical_arabic(row[0]),*row[1:]) for row in result]
        engine.text_rec.postprocess_op=ArabicDecoder()
    class LanguageOcr:
        def __call__(self,*args,**kwargs):
            rows,timing=engine(*args,**kwargs)
            if rows and kwargs.get('use_det',True):
                rows=reading_order(rows,item.get('rtl',False))
            return rows,timing
    return LanguageOcr()


def logical_arabic(text):
    pieces=re.findall(r'[a-zA-Z0-9 :*./%+\-]+|[^a-zA-Z0-9 :*./%+\-]',text)
    return ''.join(reversed(pieces))


def reading_order(rows,rtl=False):
    """Group horizontal subtitle rows by glyph scale, then reading direction."""
    lines=[]
    for row in sorted(rows,key=lambda row:sum(p[1] for p in row[0])/4):
        top=min(p[1] for p in row[0]);bottom=max(p[1] for p in row[0]);center=(top+bottom)/2
        if lines and abs(center-lines[-1][0])<.5*max(bottom-top,lines[-1][1]):
            lines[-1][2].append(row)
        else:
            lines.append([center,bottom-top,[row]])
    return [row for _,_,line in lines for row in sorted(line,key=lambda r:min(p[0] for p in r[0]),reverse=rtl)]


def recognize(engine, image):
    rows, _ = engine(image, use_cls=False)
    rows = rows or []
    text = ' '.join(row[1].strip() for row in rows if row[1].strip())
    # Confidence is the least confident line, not a probability of correctness.
    confidence = min((float(row[2]) for row in rows), default=0.0)
    return text, confidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mapping', required=True)
    parser.add_argument('--style-protocol', action='store_true')
    parser.add_argument('--language', default='en')
    args = parser.parse_args()
    runtime = ROOT.parent / 'runtime'
    dll_directory = os.add_dll_directory(str(runtime)) if runtime.is_dir() else None

    def send(text):
        data = text.encode('utf-8')
        sys.stdout.buffer.write(struct.pack('<I', len(data)) + data)
        sys.stdout.buffer.flush()

    def exact(size):
        data = bytearray()
        while len(data) < size:
            chunk = sys.stdin.buffer.read(size - len(data))
            if not chunk:
                raise EOFError
            data.extend(chunk)
        return data

    try:
        import numpy as np
        from ocr_style import learn_style, filter_rows, encode_result, rect_pixels
        engine = create_engine(args.language)
        memory = mmap.mmap(-1, CAPACITY, tagname=args.mapping, access=mmap.ACCESS_READ)
        recognize(engine, np.zeros((96, 320, 3), dtype=np.uint8))
        send('READY')
        while True:
            width, height = struct.unpack('<II', exact(8))
            config = {}
            if args.style_protocol:
                config_length, = struct.unpack('<I', exact(4))
                if config_length > 4096:
                    raise ValueError('Invalid OCR configuration length')
                config = json.loads(exact(config_length).decode('utf-8'))
            length = width * height * 4
            if not width or not height or length > CAPACITY:
                raise ValueError('Invalid OCR image dimensions')
            # The producer writes the next image only after this response.
            image = np.frombuffer(memory, dtype=np.uint8, count=length).reshape(height, width, 4)
            bgr = np.ascontiguousarray(image[:, :, :3])
            del image
            if args.style_protocol:
                try:
                    profile = config.get('profile')
                    if config.get('sample') is not None:
                        profile = learn_style(bgr, config['sample'], config.get('full_height', height))
                    rows, _ = engine(bgr, use_cls=False)
                    result = filter_rows(bgr, rows, profile, config.get('full_height', height), config.get('area', [0, 0, 1, 1]), inspect=bool(config.get('inspect') or config.get('sample')))
                    if config.get('sample') is not None:
                        x0, y0, x1, y1 = rect_pixels(config['sample'], width, height)
                        words, _ = engine(bgr[y0:y1, x0:x1], use_det=False, use_cls=False)
                        result['sample_text'] = ' '.join(str(row[0]) for row in (words or []) if float(row[1]) >= .5)
                    send(encode_result(result))
                except ValueError as exc:
                    # A poor user selection is recoverable; keep the worker alive.
                    send('ERROR: ' + str(exc))
            else:
                text, confidence = recognize(engine, bgr)
                send(f'{confidence:.6f}\t{text}')
    except EOFError:
        return
    except Exception as exc:
        send('ERROR: ' + str(exc))


if __name__ == '__main__':
    main()

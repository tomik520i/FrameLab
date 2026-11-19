"""Offline subtitle OCR. Pixels arrive through bounded, process-local shared memory.

RapidOCR + PP-OCRv5 English run on two CPU threads; no downloads or network at runtime.
The GPU remains available to frame generation, enhancement and translation.
"""
from pathlib import Path
import argparse
import mmap
import os
import struct
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'deps'))
CAPACITY = 16 * 1024 * 1024


def create_engine():
    import cv2
    from rapidocr_onnxruntime import RapidOCR
    cv2.setNumThreads(2)
    return RapidOCR(
        intra_op_num_threads=2, inter_op_num_threads=1, use_cls=False,
        det_limit_type='min', det_limit_side_len=480,
        det_model_path=str(ROOT / 'models/ch_PP-OCRv4_det_infer.onnx'),
        rec_model_path=str(ROOT / 'models/en_PP-OCRv5_rec_mobile.onnx'),
        text_score=0.70,
    )


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
        engine = create_engine()
        memory = mmap.mmap(-1, CAPACITY, tagname=args.mapping, access=mmap.ACCESS_READ)
        recognize(engine, np.zeros((96, 320, 3), dtype=np.uint8))
        send('READY')
        while True:
            width, height = struct.unpack('<II', exact(8))
            length = width * height * 4
            if not width or not height or length > CAPACITY:
                raise ValueError('Invalid OCR image dimensions')
            # The producer writes the next image only after this response.
            image = np.frombuffer(memory, dtype=np.uint8, count=length).reshape(height, width, 4)
            text, confidence = recognize(engine, np.ascontiguousarray(image[:, :, :3]))
            del image
            send(f'{confidence:.6f}\t{text}')
    except EOFError:
        return
    except Exception as exc:
        send('ERROR: ' + str(exc))


if __name__ == '__main__':
    main()

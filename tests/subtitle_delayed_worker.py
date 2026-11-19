"""Test fixture only: deterministic slow IPC response, never shipped in the app."""
from pathlib import Path
import struct
import sys
import time

root = Path(__file__).resolve().parent
def send(text):
    data = text.encode('utf-8')
    sys.stdout.buffer.write(struct.pack('<I', len(data)) + data)
    sys.stdout.buffer.flush()

send('READY')
request = 0
while header := sys.stdin.buffer.read(4):
    length, = struct.unpack('<I', header)
    text = sys.stdin.buffer.read(length).decode('utf-8')
    request += 1
    (root / f'started-{request}').write_text(text, encoding='utf-8')
    deadline = time.monotonic() + 15
    while not (root / f'release-{request}').exists():
        if time.monotonic() > deadline:
            raise TimeoutError('Test did not release delayed translation')
        time.sleep(.01)
    send(f'DELAYED RESULT {request}')
    (root / f'completed-{request}').touch()

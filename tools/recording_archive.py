"""Random access to FrameLab's lossless FLP1 blocks. No full-size raw scratch files."""
import csv, ctypes, struct
from collections import OrderedDict
from pathlib import Path

class Archive:
    def __init__(self, folder, decoder):
        self.folder = Path(folder)
        self.rows = list(csv.DictReader((self.folder / 'frames.csv').open()))
        for row in self.rows:
            for key in ('id', 'source_A', 'source_B', 'width', 'height', 'byte_offset', 'byte_count', 'stored_bytes'):
                row[key] = int(row[key])
            row['time_s'] = float(row['time_s'])
        self.lookup = {(r['kind'], r['id']): r for r in self.rows}
        self.files = {}
        self.cache = OrderedDict()
        self.dll = ctypes.CDLL(str(Path(decoder).resolve()))
        self.unpack = self.dll.unpack_recording
        self.unpack.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p]
        self.unpack.restype = ctypes.c_int

    def frame(self, key):
        key = tuple(key)
        if key in self.cache:
            self.cache.move_to_end(key)
            return self.cache[key]
        row = self.lookup[key]  # Never silently replace missing frames with good-looking substitutes.
        file = self.files.setdefault(key[0], None)
        if file is None:
            file = self.files[key[0]] = (self.folder / (key[0] + '.flp')).open('rb')
        file.seek(row['byte_offset'])
        header = file.read(20)
        if len(header) != 20:
            raise ValueError('Truncated FLP header')
        magic, raw, stored, flags, alpha = struct.unpack('<5I', header)
        if magic != 0x31504c46 or raw != row['byte_count'] or raw != row['width'] * row['height'] * 4 or stored + 20 != row['stored_bytes'] or stored > raw:
            raise ValueError('Invalid FLP block/index')
        packed = file.read(stored)
        if len(packed) != stored:
            raise ValueError('Truncated FLP pixels')
        buffer = ctypes.create_string_buffer(raw)
        if self.unpack(packed, stored, raw, flags, alpha, buffer) != 0:
            raise ValueError('Recording decompression failed')
        pixels = buffer.raw
        self.cache[key] = pixels
        while len(self.cache) > 8:
            self.cache.popitem(last=False)
        return pixels

    def close(self):
        for file in self.files.values():
            if file:
                file.close()
        self.cache.clear()

"""Offline tests: interruption/resume, integrity failure and zip traversal."""
from pathlib import Path
import hashlib
import http.server
import sys
import tempfile
import threading
import unittest
import zipfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from subtitle_models import download, install_runtime, verified, verify, atomic_json, fingerprint
from subtitle_model_manager import model_status

DATA = bytes(range(256)) * 16384


class Handler(http.server.BaseHTTPRequestHandler):
    ranges = []
    def log_message(self, *args):
        pass
    def do_GET(self):
        start = int(self.headers.get('Range', 'bytes=0-').split('=')[1].split('-')[0])
        self.ranges.append(start)
        self.send_response(206 if start else 200)
        self.send_header('Content-Length', str(len(DATA)-start))
        if start:
            self.send_header('Content-Range', f'bytes {start}-{len(DATA)-1}/{len(DATA)}')
        self.end_headers()
        try:
            self.wfile.write(DATA[start:])
        except (BrokenPipeError, ConnectionResetError):
            pass


class Assets(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()
    def asset(self):
        return dict(file='test.gguf', bytes=len(DATA), sha256=hashlib.sha256(DATA).hexdigest(), url=f'http://127.0.0.1:{self.server.server_port}/data')
    def test_resume_and_verify(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            stop = threading.Event()
            def progress(phase, done, total, detail):
                if phase == 'downloading' and done >= 1024*1024:
                    stop.set()
            with self.assertRaises(InterruptedError):
                download(self.asset(), root, progress, stop.is_set)
            self.assertFalse((root/'test.gguf').exists())
            count = (root/'test.gguf.partial').stat().st_size
            self.assertGreater(count, 0)
            target = download(self.asset(), root)
            self.assertIn(count, Handler.ranges)
            self.assertEqual(target.read_bytes(), DATA)
            self.assertTrue(verified(target, self.asset()))
            target.write_bytes(b'damaged')
            self.assertFalse(verified(target, self.asset()))
    def test_hash_mismatch_never_activates(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            asset = self.asset()
            asset['sha256'] = '0'*64
            with self.assertRaises(ValueError):
                download(asset, root)
            self.assertFalse((root/'test.gguf').exists())
    def test_runtime_traversal(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            with zipfile.ZipFile(root/'runtime.zip', 'w') as z:
                z.writestr('../escape.exe', b'not an executable')
            with self.assertRaises(ValueError):
                install_runtime(root/'runtime.zip', root/'runtime')
            self.assertFalse((root/'escape.exe').exists())
    def test_runtime_repair_preserves_previous(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root/'runtime').mkdir()
            (root/'runtime/old.txt').write_text('previous installation')
            with zipfile.ZipFile(root/'runtime.zip', 'w') as z:
                z.writestr('ollama.exe', b'test fixture, never executed')
            install_runtime(root/'runtime.zip', root/'runtime')
            self.assertTrue((root/'runtime/ollama.exe').exists())
            previous = list(root.glob('runtime.previous-*'))
            self.assertEqual(len(previous), 1)
            self.assertEqual((previous[0]/'old.txt').read_text(), 'previous installation')
    def test_ready_state_tracks_removed_or_modified_files(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root/'models').mkdir()
            (root/'gpu-runtime').mkdir()
            model = self.asset()
            path = root/'models'/model['file']
            path.write_bytes(DATA)
            verify(path, model)
            exe = root/'gpu-runtime/ollama.exe'
            exe.write_bytes(b'fixture, never executed')
            data = dict(runtime=dict(sha256='fixture archive hash'))
            atomic_json(root/'gpu-runtime/verified.json', dict(
                archive_sha256=data['runtime']['sha256'], executable=fingerprint(exe)))
            ready = model_status(root, model, data, dict(phase='missing'))
            self.assertEqual(ready['phase'], 'ready')
            exe.write_bytes(b'changed runtime')
            self.assertEqual(model_status(root, model, data, ready)['phase'], 'downloaded')
            path.write_bytes(b'changed model')
            self.assertEqual(model_status(root, model, data, ready)['phase'], 'needs verification')
            path.unlink()
            self.assertEqual(model_status(root, model, data, ready)['phase'], 'missing')


if __name__ == '__main__':
    unittest.main()

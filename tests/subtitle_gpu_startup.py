"""A fresh Ollama cache may prune unregistered blobs during server startup."""
from pathlib import Path
import hashlib
import os
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import subtitle_gpu


class FirstModelImport(unittest.TestCase):
    def test_first_import_survives_runtime_startup_cleanup(self):
        with tempfile.TemporaryDirectory(prefix='framelab-first-import-') as folder:
            app = Path(folder)
            root = app / 'translation'
            (root / 'models').mkdir(parents=True)
            (app / 'logs').mkdir()
            content = b'fixture model; inference API is simulated'
            asset = dict(id='cuni-q3km', file='fixture.gguf', bytes=len(content),
                         sha256=hashlib.sha256(content).hexdigest())
            (root / 'models' / asset['file']).write_bytes(content)
            blob = root / 'models/ollama/blobs' / ('sha256-' + asset['sha256'])
            process = Mock()
            process.poll.return_value = None

            def start_runtime(*args, **kwargs):
                self.assertTrue(Path(kwargs['env']['OLLAMA_MODELS']).is_absolute())
                # Observed with pinned Ollama 0.40.16 before /api/version responds.
                # On a first install no manifest references this staged blob yet.
                if blob.exists():
                    blob.unlink()
                return process

            def api(instance, path, payload=None, timeout=90):
                if path == '/api/create':
                    self.assertTrue(blob.is_file(), 'Runtime pruned the model before registration')
                    self.assertEqual(blob.read_bytes(), content)
                if path == '/api/ps':
                    return dict(models=[dict(size=len(content), size_vram=len(content))])
                return dict(response='Měli bychom vyrazit.', version='0.40.16')

            with patch.object(subtitle_gpu, 'catalog', return_value=dict(models=[asset])), \
                 patch.object(subtitle_gpu, 'verified', return_value=True), \
                 patch.object(subtitle_gpu, 'verify'), \
                 patch.object(subtitle_gpu, 'runtime_ready', return_value=True), \
                 patch.object(subtitle_gpu, 'available_vram', return_value=None), \
                 patch.object(subtitle_gpu.subprocess, 'Popen', side_effect=start_runtime), \
                 patch.object(subtitle_gpu.GpuTranslator, 'api', api):
                translator = subtitle_gpu.GpuTranslator(Path(os.path.relpath(root)), 'cuni-q3km')
                self.assertEqual(translator.translate('We should go.'), 'Měli bychom vyrazit.')
                translator.close()
            self.assertEqual((root / 'models' / asset['file']).read_bytes(), content)
            process.terminate.assert_called_once()


if __name__ == '__main__':
    unittest.main()

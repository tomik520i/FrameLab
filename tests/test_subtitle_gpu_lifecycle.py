"""Protocol-only test: no model loading, HTTP requests, or GPU use."""
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from subtitle_gpu import GpuTranslator

class Lifecycle(unittest.TestCase):
    def test_residency_timeout_and_explicit_unload(self):
        model = GpuTranslator.__new__(GpuTranslator)
        model.name = 'fixture-model'
        model.log = None
        calls = []
        def api(path, payload=None, timeout=90):
            calls.append((path, payload, timeout))
            return {'response': ' Pojď za mnou. '}
        model.api = api
        self.assertEqual(model.translate('Follow me.'), 'Pojď za mnou.')
        self.assertEqual(calls[-1][1]['keep_alive'], -1)
        self.assertLess(calls[-1][2], 30)  # Respond before the parent IPC deadline.
        model.translate('Warmup', timeout=90)
        self.assertEqual(calls[-1][2], 90)
        class Process:
            terminated = False
            def terminate(self): self.terminated = True
            def wait(self, timeout): pass
        process = Process()
        model.process = process
        model.close()
        self.assertEqual(calls[-1][1]['keep_alive'], 0)
        self.assertTrue(process.terminated)
        self.assertIsNone(model.process)

if __name__ == '__main__':
    unittest.main()

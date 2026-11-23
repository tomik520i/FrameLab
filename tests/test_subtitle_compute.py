from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from subtitle_compute import choose,runtime_environment
from subtitle_gpu import GpuTranslator
class ComputeSelection(unittest.TestCase):
    def setUp(self):
        self.devices=[dict(backend='vulkan',id='intel',index=0,name='Intel UHD',integrated=True,total=None),dict(backend='vulkan',id='amd',index=1,name='Radeon',integrated=False,total=None),dict(backend='cuda',id='GPU-nvidia',index=0,name='RTX',integrated=False,total=6*1024**3)]
    def test_auto_prefers_discrete_cuda_then_vulkan_then_cpu(self):
        self.assertEqual(choose('auto',devices=self.devices)['id'],'GPU-nvidia')
        self.assertEqual(choose('auto',devices=self.devices[:2])['id'],'amd')
        self.assertEqual(choose('auto',devices=self.devices[:1])['id'],'intel')
        self.assertEqual(choose('auto',devices=[])['backend'],'cpu')
    def test_explicit_missing_gpu_must_not_silently_change(self):
        with self.assertRaises(RuntimeError):choose('vulkan','missing',self.devices)
        with self.assertRaises(RuntimeError):choose('cuda',devices=[])
        with self.assertRaises(ValueError):choose('arbitrary backend',devices=[])
    def test_stable_uuid_resolved_after_index_reorder(self):
        changed=[dict(d,index=7-i) for i,d in enumerate(self.devices)]
        d=choose('vulkan','intel',changed)
        self.assertEqual(runtime_environment({},d)['GGML_VK_VISIBLE_DEVICES'],'7')
    def test_vulkan_isolates_nvidia_rocm_and_inherited_override(self):
        env=runtime_environment({'CUDA_VISIBLE_DEVICES':'0','OLLAMA_LLM_LIBRARY':'cuda_v12','GGML_BACKEND_PATH':'foreign.dll','KEEP':'yes'},self.devices[0])
        self.assertEqual(env['CUDA_VISIBLE_DEVICES'],'-1');self.assertEqual(env['HIP_VISIBLE_DEVICES'],'-1')
        self.assertEqual(env['GGML_VK_VISIBLE_DEVICES'],'0');self.assertEqual(env['OLLAMA_VULKAN'],'1')
        self.assertNotIn('OLLAMA_LLM_LIBRARY',env);self.assertNotIn('GGML_BACKEND_PATH',env);self.assertEqual(env['KEEP'],'yes')
    def test_cpu_disables_all_gpu_families(self):
        env=runtime_environment({},choose('cpu',devices=self.devices))
        for key in ['CUDA_VISIBLE_DEVICES','GGML_VK_VISIBLE_DEVICES','HIP_VISIBLE_DEVICES','ROCR_VISIBLE_DEVICES']:self.assertEqual(env[key],'-1')
        self.assertEqual(env['OLLAMA_VULKAN'],'0')
    def test_model_placement_is_explicit(self):
        model=GpuTranslator.__new__(GpuTranslator);model.compute='cpu';self.assertEqual(model.options()['num_gpu'],0)
        model.compute='vulkan';model.allow_offload=False;self.assertEqual(model.options()['num_gpu'],99)
        model.allow_offload=True;self.assertEqual(model.options()['num_gpu'],-1)
if __name__=='__main__':unittest.main()

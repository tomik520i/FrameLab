# Local translation: CUDA, Vulkan or CPU

Included in FrameLab preview.18. Select **Subtitles → Local / offline**, choose CUNI for EN → CS or TranslateGemma for supported language pairs, then **Translation compute**. Auto, CUDA, Vulkan and CPU are available. Choose the translation GPU independently from the image/frame-generation GPU. Changing backend disables translation; enable it again after reviewing the selection.

Use the normal model download/resume button. The same verified GGUF weights and portable Ollama 0.40.16 runtime serve all backends. No CUDA Toolkit or Vulkan SDK installation is required; a working graphics driver is required. GPU translation models/runtime remain separate downloads; OPUS-MT CPU works from the full ZIP immediately.

By default the app checks full GPU placement. **Allow GPU + RAM (slower)** permits partial offload. A selected GPU path silently loading everything on CPU is rejected; explicitly choose CPU to run the same GGUF model in RAM. CPU here does not substitute OPUS for CUNI/Gemma. Device choices use stable UUIDs and affect only FrameLab's own runtime process.

Auto prefers a discrete GPU and CUDA on NVIDIA, then Vulkan, then CPU when no usable GPU is found. This is device detection, not a speed benchmark. Integrated GPUs use shared memory and can be slower than CPU. AMD has not been physically tested. Translation backend support does not remove NVIDIA restrictions on VFG or NR enhancement.

## Measured on this laptop

Intel i5-13420H, RTX 4050 Laptop 6 GiB, Intel UHD, Windows 11. Four identical short entries per configuration: 32 translations total. Serial runs, ordinary desktop. No OCR, FG or NR in these timing samples. Model initialization/warmup is recorded separately. CUDA used Flash Attention; Vulkan/CPU did not. This is a compatibility and timing smoke test, not a new quality benchmark.

| Model | CUDA RTX | Vulkan RTX | Vulkan Intel UHD | CPU |
|---|---:|---:|---:|---:|
| CUNI-MH 7B Q3_K_M | 0.74 s | 5.26 s | 21.50 s | 4.28 s |
| TranslateGemma 4B Q4_K_M | 0.49 s | 2.52 s | 11.46 s | 3.53 s |

CUDA is preferable on this laptop. The tested Intel UHD path works but is too slow for typical live captions. These measurements do not predict AMD or Intel Arc performance. CUNI returned identical text on all four paths; two Gemma CPU outputs differed in phrasing. Backend arithmetic can affect output even at zero temperature.

Real image → OCR → filter → translator integration passed 6/6 cases for CUNI on Intel Vulkan and 6/6 for Gemma on RTX Vulkan. Forced partial placement of eight CUNI layers on RTX Vulkan also translated successfully (1,089,522,892 bytes on GPU of 4,021,697,902 total); automatic fitting under VRAM pressure was not separately stress-tested. Larger quantizations were not benchmarked here.

If inference is slow, a result stays eligible only while fresh OCR still confirms the same text. Source/model changes invalidate old results. Reported delay retains the original observation timestamp.

[All timings, outputs and original evidence](https://opicebot.cz/framelab/runs/2026-11-23-compute/) · [Separate 500-entry quality review](https://opicebot.cz/framelab/runs/2026-11-23-direct-review/) · [Ollama Vulkan documentation](https://docs.ollama.com/gpu#vulkan-gpu-support).

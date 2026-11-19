Open index.html for desktop coexistence results, quality/report.html for all 300 reviewed pairs.
All quality outputs from the concurrent runs match the separately reviewed preflight after stripping surrounding whitespace.
Exact-text inference ran beside preview.12 CPU OCR, VFG 2x Medium and experimental NR 65% / 720p on a 1080p30 sample loop. It did not replace the CPU worker during this controlled benchmark.
Separate native integration tests validate the new preview.13 OCR/CUNI pipeline.
Whole-device NVML includes other desktop processes. Application content submissions are not measured physical monitor FPS. Translation timings exclude OCR/stabilization.
Data includes original environment paths and reproduction scripts: adapt the paths before rerunning. No weights, executables or private NR DLLs.

# Contributing

Use C++20 / Windows x64. Keep interpolation behind `FrameGenerator`, capture behind `DirectShowCapture`, and presentation independent of compute completion. Add regression tests for pacing and ID association. Never report Present calls as physical display FPS or pixel difference as visual accuracy. Include GPU model, backend, resolution and identical source footage in performance reports. Keep recordings and proprietary runtime out of source commits.

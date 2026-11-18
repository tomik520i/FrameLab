# Interface rendering checks

The interface uses the website's original OFL fonts, embedded at build time. `src/UiTheme.hpp` defines the app palette and widget styles. Top-level windows deliberately have zero border thickness: ImGui's `NoDecoration` flag alone still draws and clips a one-pixel edge.

For layout review without desktop capture or a capture device, launch a separate executable with:

```text
FrameLab.exe --demo --ui-preview C:\Temp\layout.bmp --preview-width 1600 --preview-height 1000 --preview-tab 1 --preview-cs
```

This creates a hidden application window, saves only its own D3D11 render target after about 1.6 seconds and exits. It never uses a desktop-capture API. Tab indexes are Capture=0, Frame gen=1, Audio=2, Recording=3, Stats=4. Omit `--preview-cs` for English. `--preview-fullscreen` selects only the image layout. Requested preview dimensions are bounded to 640–3840 by 480–2160. Capture startup is disabled in this mode; `--demo`, `--sample` or the existing `--replay-recording` source can be used.

Preview counters reflect the chosen test source and hidden presentation; they are not live gameplay or physical-display benchmarks. Screenshots published with preview.9 use a short RAW replay from recording `113768562`. Existing public comparison recordings are unchanged.

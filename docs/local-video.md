# Local videos, MKV and tracks

Open **Capture → Open video…** and select a local video. The complete portable
package includes LibVLC 3.0.24, so a separate VLC installation or Windows codec
pack is unnecessary for supported formats. MKV, MP4, WebM and other common
containers are accepted; a particular file still needs a supported codec and
must not be DRM-protected.

Once the file opens, the same panel shows:

- **Audio track:** the audio streams in this file, with language/name where the
  file provides them. **Off** disables file audio. Volume, mute and the physical
  output device remain in **Audio**.
- **Subtitles in this file:** the embedded subtitle streams, or **Off**. These
  are rendered into the video source before FG, enhancement and optional OCR.
  Selecting Czech subtitles directly does not require AI translation.
- Pause, seek, looping, native dimensions and the player backend.

Audio/subtitle selections are preserved when changing the audio output device
within the same open file. They are not saved as global defaults across different
movies. Named **OCR appearance profiles** are separate: see [subtitle-ocr.md](subtitle-ocr.md).

To translate an embedded English subtitle track, select it here and enable Czech
translation in **Subtitles**. This version recognizes the rendered subtitles with
OCR; it does **not** extract their text/timing directly for translation. Select
an appropriate reading area and appearance profile. Burned-in captions cannot be
disabled with the subtitle-track menu because they are part of the picture.

## Developer/runtime notes

Prepare the portable decoder with Python 3.11+:

```powershell
python tools/prepare_media.py --output YOUR_APP/media/vlc
```

The official archive is pinned and SHA-256 checked against
`docs/media-runtime.json`. Libraries, decoder plugins, upstream notices and
source information travel together. Copy the whole `media/vlc` folder, not just
`libvlc.dll`. Without it, the previous Windows Media Foundation path remains
available, with Windows-dependent codec support and its original first-track
limitation. The player never downloads codecs while opening a file.

The integration uses LibVLC's CPU BGRA memory callback. This is not a zero-copy
GPU decoding path and can add CPU/memory bandwidth cost at high resolutions.
FG and enhancement still use the selected existing backends. Callback timestamps
are estimates from the player's clock, not exact per-picture timestamps; do not
use them as proof of end-to-end display latency. Playback is not a video exporter.

Local regression test `local_video_tracks_test` uses an 8-second 320×180 / 24 FPS
MKV containing two AAC and two SRT streams. Subtitle switching is checked in
decoded pixels; the test mutes audio and verifies selected audio IDs. It is not
an exhaustive codec, HDR, surround-audio or hardware-decoding qualification.

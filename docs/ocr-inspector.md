# OCR inspector — preview.16 development build

This feature is in current source and the local development build. The published
preview.15 application ZIP predates it.

Open **Subtitles → OCR inspector / saved sample…** (Czech: **Titulky → Inspektor
OCR / uložený vzor…**). The button is directly below the word-selection button.

- The saved word image and its recognized text show what was selected. The text
  is an example, not a phrase that subsequent subtitles must contain.
- A colour swatch, RGB/hex values, normalized letter height and tolerances describe
  the filter. Pixel height is calculated for the current source resolution.
- Live rows show their OCR text, score, measured colour, estimated letter height,
  OCR-box height and decision: accepted, outside the reading area, wrong size or
  wrong colour. Letter height and box height are different measurements. The
  font family and typographic point size are not identified.
- **Freeze these readings** holds the inspected values while video, audio, OCR
  and translation continue. The reading age remains visible. Source changes clear
  the held reading. Closing the inspector stops the extra analysis; fullscreen
  also hides the inspector.

For a new sample, select a clear word and press **Use this style**. The selection
dialog shows the original crop, its OCR text and measurements before applying it.
Save a named profile as usual. Sample metadata survives restart and switching
profiles; small word crops (maximum 512 × 128) stay under
`app/subtitle-style-samples/<numeric-id>.bmp`. Applying a style saves this one crop;
it does not start screen recording. Personal crops are excluded from public ZIPs.

Older settings and profiles remain readable. They have colour/height but no
historical word image or text: select one word again to add that evidence. Profile
format 2 and settings format 8 retain sample metadata; preview.15 cannot read the
new formats. Back up preferences before downgrading.

The inspector uses the existing RapidOCR readings. It adds a bounded CPU
appearance measurement for rejected/unfiltered rows only while open. It never
changes acceptance decisions and does not load another model. With Windows OCR,
appearance diagnostics are unavailable. Live rows require subtitle reading to be
enabled; the word-selection preview can be used independently.

If **Debug and measurement** is already recording, `ocr_style_line` events also
include measured RGB, normalized/source-pixel letter height and box height.
Recognized text follows the existing optional text-logging setting. Opening the
inspector does not start debug recording or upload anything.

Validation: bundled OCR on a mixed-style fixture with a sign, plate, three subtitle
lines, alternate colour, dimmer text and scaled 1080p input; inspection leaves
accepted text/decisions unchanged. Native IPC tests verify measurements, source
height, persistence, rejection without fallback and no pipeline reset on closing.
Offscreen ImGui input/rendering checks word selection, saved-crop reload and the
inspector without capturing the Windows desktop.

# Bundled sample videos

These silent, short looping test clips are bundled inside the full preview.10 release. They are not downloaded at startup. Exact source URLs, crops, encoding parameters and SHA-256 values accompany each file in app/samples/*.source.json.

| File | Content | Origin / transformation |
| --- | --- | --- |
| action.mp4 | CS:GO, 1920×1080, 60 FPS, 180 frames / 3 s | [Xiph/Twitch CSGO.y4m](https://media.xiph.org/video/derf/twitch/y4m/CSGO.y4m), seconds 25–28, original input frames, H.264 CRF18, no interpolation |
| combat.mp4 | Spider-Man, 960×540, 60 FPS, 3 s | This project's [previous public comparison](https://github.com/tomik520i/FrameLab/releases/download/v0.2.0-preview.7/FrameLab-113768562-comparison-example.zip); original left panel only, crop 960:540:0:80, restore 4× slowdown, H.264 CRF16; already downscaled/lossy |
| ocr-demo.mp4 | Artificial moving test pattern, 1280×720, 30 FPS, 8 s | Generated local test material with the sentence “The door is locked.”; tests OCR and rendering, not real-game translation quality |

Xiph's [recording notice](https://media.xiph.org/video/derf/twitch/copyright.txt) places the video game recordings in the public domain while expressly noting that copyrighted game content remains subject to fair use. The original notice is included as sample-xiph-copyright.txt. This does not put game assets in the public domain or grant an unrestricted game-content license.

Spider-Man footage is a short excerpt of the repository owner's already published recording used to demonstrate processing behavior. Game imagery, characters, trademarks and other underlying content retain their owners' rights. No new GPL or public-domain claim is made over them. Attribution is not a universal redistribution permission; applicable game-content policies and local law still apply.

The artificial OCR fixture is FrameLab-created test material. Source/model/processed comparison images are identified separately; increased sharpness or changed pixels are not by themselves evidence of higher quality.

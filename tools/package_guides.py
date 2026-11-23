"""Copy the same illustrated guides to a portable package, including offline assets."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
GUIDES = {'index.html': 'ZACNI TADY.html', 'en.html': 'START HERE.html',
          'enhancer.html': 'NAVOD ENHANCER.html', 'enhancer-en.html': 'ENHANCER GUIDE.html',
          'visual.html': 'OBRAZOVY NAVOD.html', 'visual-en.html': 'VISUAL GUIDE.html',
          'fullscreen.html': 'FULLSCREEN.html', 'fullscreen-en.html': 'FULLSCREEN EN.html',
          'subtitles.html':'TITULKY A MODELY.html','subtitles-en.html':'SUBTITLES AND MODELS.html',
          'recording.html':'NAHRAVANI.html','recording-en.html':'RECORDING.html',
          'multilingual.html':'VICEJAZYCNE TITULKY.html','multilingual-en.html':'MULTILINGUAL SUBTITLES.html',
          'remote-subtitles.html':'REMOTE PLAY A API.html','remote-subtitles-en.html':'REMOTE PLAY AND API.html',
          'gpu.html':'PODPORA GPU.html','gpu-en.html':'GPU SUPPORT.html',
          'compute.html':'VYPOCET PREKLADU.html','compute-en.html':'TRANSLATION COMPUTE.html'}

def copy_guides(stage):
    stage = Path(stage)
    stage.mkdir(parents=True, exist_ok=True)
    for source, destination in GUIDES.items():
        markup = (ROOT / 'website/guide' / source).read_text(encoding='utf-8')
        markup = markup.replace('<link rel="stylesheet" href="../styles.css">', '')
        markup = markup.replace('href="../#download"', 'href="https://opicebot.cz/framelab/#download"')
        for old, new in GUIDES.items():
            markup = markup.replace('href="https://opicebot.cz/framelab/guide/' + old, 'href="' + new)
            markup = markup.replace('href="' + old, 'href="' + new)
        markup = markup.replace('href="https://opicebot.cz/framelab/guide/"', 'href="ZACNI TADY.html"')
        # Reports and site navigation are online; they are not files beside the EXE.
        markup = markup.replace('href="../', 'href="https://opicebot.cz/framelab/')
        markup = markup.replace('"assets/', '"guide-assets/')
        (stage / destination).write_text(markup, encoding='utf-8')
    shutil.copytree(ROOT / 'website/guide/assets', stage / 'guide-assets', dirs_exist_ok=True)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', type=Path)
    copy_guides(parser.parse_args().stage)

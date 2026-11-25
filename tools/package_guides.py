"""Copy illustrated guides and offline assets. Build dependency: beautifulsoup4."""
from pathlib import Path
import shutil
import re
import json
from bs4 import BeautifulSoup

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
          'compute.html':'VYPOCET PREKLADU.html','compute-en.html':'TRANSLATION COMPUTE.html',
          'custom-ai.html':'VLASTNI AI SERVER.html','custom-ai-en.html':'CUSTOM AI SERVER.html',
          'memory.html':'PAMET ROZHOVORU.html','memory-en.html':'CONVERSATION MEMORY.html'}

def copy_guides(stage):
    stage = Path(stage)
    stage.mkdir(parents=True, exist_ok=True)
    for source, destination in GUIDES.items():
        markup = (ROOT / 'website/guide' / source).read_text(encoding='utf-8')
        # Resolve bilingual labels before removing web-only JavaScript. Offline
        # guides must remain readable and switch languages through file links.
        lang = 'en' if source == 'en.html' or source.endswith('-en.html') else 'cs'
        soup = BeautifulSoup(markup, 'html.parser')
        catalog = json.loads((ROOT / 'website/i18n/ui.json').read_text(encoding='utf-8'))
        for element in soup.find_all(True):
            value = element.get('data-' + lang)
            if value is None and element.get('data-i18n') in catalog:
                value = catalog[element['data-i18n']][lang]
            if value is not None:
                element.clear(); element.append(value)
        soup.html['lang'] = lang
        markup = str(soup)
        # Paired static guides work through file:// without a web server. Shared
        # website controls use fetch and online routes, so keep native links here.
        markup = re.sub(r'<script\b[^>]*src="[^"]*site-(?:i18n|navigation|images)\.js[^\"]*"[^>]*>\s*</script>', '', markup)
        markup = re.sub(r'<link\b[^>]*href="[^"]*site-(?:i18n|navigation|images)\.css[^\"]*"[^>]*>', '', markup)
        markup = re.sub(r' data-locale-switch="(?:cs|en)"', '', markup)
        markup = markup.replace('<link rel="stylesheet" href="../styles.css">', '')
        markup = markup.replace('href="../#download"', 'href="https://opicebot.cz/framelab/#download"')
        for old, new in GUIDES.items():
            markup = markup.replace('href="https://opicebot.cz/framelab/guide/' + old, 'href="' + new)
            markup = markup.replace('href="' + old, 'href="' + new)
        markup = markup.replace('href="https://opicebot.cz/framelab/guide/"', 'href="ZACNI TADY.html"')
        markup = markup.replace('href="../assets/fonts/fonts.css"', 'href="guide-assets/site/fonts/fonts.css"')
        markup = markup.replace('href="../explore.css"', 'href="guide-assets/site/explore.css"')
        # Reports and site navigation are online; they are not files beside the EXE.
        markup = markup.replace('href="../', 'href="https://opicebot.cz/framelab/')
        markup = markup.replace('"assets/', '"guide-assets/')
        (stage / destination).write_text(markup, encoding='utf-8')
    shutil.copytree(ROOT / 'website/guide/assets', stage / 'guide-assets', dirs_exist_ok=True)
    shutil.copytree(ROOT / 'website/assets/fonts', stage / 'guide-assets/site/fonts', dirs_exist_ok=True)
    shutil.copy2(ROOT / 'website/explore.css', stage / 'guide-assets/site/explore.css')

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', type=Path)
    copy_guides(parser.parse_args().stage)

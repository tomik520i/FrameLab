"""Render the repository's small installation-Markdown subset into static web pages."""
from pathlib import Path
import html
import re
from render_visual_guide import figure, lightbox, guide_banner, main as render_visual
from render_fullscreen_guide import main as render_fullscreen
ROOT=Path(__file__).resolve().parents[1]

def inline(text):
    text=html.escape(text)
    def link(m):
        url=html.unescape(m[2])
        local={'INSTALL.cs.md':'index.html','START_HERE.md':'en.html','docs/enhancer-install.cs.md':'enhancer.html','docs/enhancer-install.en.md':'enhancer-en.html',
               'docs/subtitle-model-guide.md':'subtitles-en.html','docs/subtitle-model-guide.cs.md':'subtitles.html',
               'subtitle-model-guide.md':'subtitles-en.html','subtitle-model-guide.cs.md':'subtitles.html'}
        if url in local:url=local[url]
        elif not url.startswith(('https://','http://','#')):
            if url in ['NR-LICENSING-STATUS.txt','subtitle-models.json','translation-components.json']:url='docs/'+url
            url='https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.13/'+url
        return '<a href="'+html.escape(url,quote=True)+'">'+m[1]+'</a>'
    text=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',link,text)
    text=re.sub(r'`([^`]+)`',r'<code>\1</code>',text)
    return re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',text)

def render(text):
    # Wrapped Markdown paragraphs/list items remain one semantic block.
    lines=[]
    for line in text.splitlines():
        if line.strip() and lines and lines[-1].strip() and not re.match(r'^(#|\||- |\d+\. )',line.strip()) and not lines[-1].startswith(('#','|')):
            lines[-1]+=' '+line.strip()
        else:lines.append(line.strip())
    result=[];mode=None
    for line in lines:
        line=line.strip()
        kind='ol' if re.match(r'^\d+\. ',line) else 'ul' if line.startswith('- ') else 'table' if line.startswith('|') else None
        if kind!=mode:
            if mode:result.append('</tbody></table></div>' if mode=='table' else '</'+mode+'>')
            if kind:result.append('<div class="table-wrap"><table><tbody>' if kind=='table' else '<'+kind+'>')
            mode=kind
        if not line:continue
        if kind=='table':
            if re.fullmatch(r'[| :\-]+',line):continue
            result.append('<tr>'+''.join('<td>'+inline(c.strip())+'</td>' for c in line.strip('|').split('|'))+'</tr>')
        elif kind:result.append('<li>'+inline(re.sub(r'^(\d+\.|-) ','',line))+'</li>')
        elif line.startswith('#'):
            level=len(line)-len(line.lstrip('#'));result.append(f'<h{level}>'+inline(line[level:].strip())+f'</h{level}>')
        else:result.append('<p>'+inline(line)+'</p>')
    if mode:result.append('</tbody></table></div>' if mode=='table' else '</'+mode+'>')
    return '\n'.join(result)

def main():
    target=ROOT/'website/guide';target.mkdir(exist_ok=True)
    render_visual()
    render_fullscreen()
    for source,name,lang in [('INSTALL.cs.md','index.html','cs'),('START_HERE.md','en.html','en'),('docs/enhancer-install.cs.md','enhancer.html','cs'),('docs/enhancer-install.en.md','enhancer-en.html','en'),('docs/subtitle-model-guide.cs.md','subtitles.html','cs'),('docs/subtitle-model-guide.md','subtitles-en.html','en')]:
        content=render((ROOT/source).read_text(encoding='utf-8'))
        sections=re.split(r'(?=<h2>)',content)
        shot_map={
            'index.html':{1:['launch','samples'],2:['fg'],3:['capture'],4:['ocr'],5:['display'],6:['wizard'],7:['longpath'],9:['local-video']},
            'en.html':{1:['launch'],2:['fg','capture'],3:['ocr'],4:['display'],5:['wizard'],6:['longpath'],8:['local-video']},
            'enhancer.html':{1:['extract','destination'],2:['wizard'],3:['select','ready'],4:['nr'],6:['passes']},
            'enhancer-en.html':{1:['wizard','select','ready','nr'],2:['longpath'],3:['passes']},
            'subtitles.html':{1:['ocr'],2:['cuni'],3:['caption-bounds']},'subtitles-en.html':{1:['ocr'],2:['cuni'],3:['caption-bounds']},
        }[name]
        content=sections[0]+guide_banner(lang)
        for i,section in enumerate(sections[1:],1):
            shots=''.join(figure(key,lang,'-text') for key in shot_map.get(i,[]))
            content+='<section class="illustrated-section"><div class="step-copy">'+section+'</div><aside class="shots">'+shots+'</aside></section>' if shots else '<section class="text-only">'+section+'</section>'
        cs,en=('enhancer.html','enhancer-en.html') if name.startswith('enhancer') else ('subtitles.html','subtitles-en.html') if name.startswith('subtitles') else ('index.html','en.html')
        (target/name).write_text('<!doctype html><html lang="'+lang+'"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab preview.13 — installation / instalace</title><link rel="stylesheet" href="assets/visual.css"></head><body><main><nav><a href="../#download">← FrameLab</a> · <a href="'+cs+'">Česky</a> · <a href="'+en+'">English</a></nav>'+content+'</main>'+lightbox(lang)+'</body></html>',encoding='utf-8')

if __name__=='__main__':main()

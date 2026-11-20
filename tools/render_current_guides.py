"""Current EN/CZ guides, shared by the website and the offline release."""
from pathlib import Path
import html,re,shutil,sys
from render_visual_guide import figure as draw_figure, lightbox, guide_banner, main as render_visual
ROOT=Path(__file__).resolve().parents[1]
PAGES=[('START_HERE.md','en.html','en','capture'),('INSTALL.cs.md','index.html','cs','capture'),('docs/subtitle-model-guide.md','subtitles-en.html','en','models'),('docs/subtitle-model-guide.cs.md','subtitles.html','cs','models'),('docs/enhancer-install.en.md','enhancer-en.html','en','nr'),('docs/enhancer-install.cs.md','enhancer.html','cs','nr'),('docs/pipeline-recording.md','recording-en.html','en','recording'),('INSTALL.cs.md','recording.html','cs','recording'),('START_HERE.md','fullscreen-en.html','en','display'),('INSTALL.cs.md','fullscreen.html','cs','display')]
LINKS={source:name for source,name,*_ in PAGES[:8] if name!='recording.html'}
def inline(text):
    tokens=[]
    def protect(m):
        tokens.append('<code>'+html.escape(m[1])+'</code>');return f'ZZCODE{len(tokens)-1}ZZ'
    text=re.sub(r'`([^`]+)`',protect,text)
    def link(m):
        label,url=m[1],m[2];base,sep,anchor=url.partition('#')
        if not url.startswith(('https://','http://','#')):
            key=base.removeprefix('../')
            if key in LINKS:url=LINKS[key]+(sep+anchor if sep else '')
            elif 'docs/'+key in LINKS:url=LINKS['docs/'+key]+(sep+anchor if sep else '')
            else:
                if '/' not in key and key not in ('README.md','LICENSE','START_HERE.md','INSTALL.cs.md','THIRD_PARTY_NOTICES.md'):key='docs/'+key
                url='https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.16/'+key+(sep+anchor if sep else '')
        tokens.append('<a href="'+html.escape(url,quote=True)+'">'+html.escape(label)+'</a>');return f'ZZCODE{len(tokens)-1}ZZ'
    text=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',link,text);text=html.escape(text)
    text=re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',text)
    for i,t in enumerate(tokens):text=text.replace(f'ZZCODE{i}ZZ',t)
    return text
def render(text):
    lines=text.splitlines();out=[];i=0
    while i<len(lines):
        line=lines[i].strip();i+=1
        if not line:continue
        if line.startswith('```'):
            block=[]
            while i<len(lines) and not lines[i].startswith('```'):block.append(lines[i]);i+=1
            i+=1;out.append('<pre><code>'+html.escape('\n'.join(block))+'</code></pre>');continue
        if line.startswith('#'):
            n=len(line)-len(line.lstrip('#'));title=line[n:].strip();anchor=re.sub(r'[^\w -]','',title.lower()).replace(' ','-');out.append(f'<h{n} id="{anchor}">{inline(title)}</h{n}>');continue
        if line.startswith('|'):
            rows=[line]
            while i<len(lines) and lines[i].strip().startswith('|'):rows.append(lines[i].strip());i+=1
            cells=[]
            for k,row in enumerate(rows):
                if re.fullmatch(r'[| :\-]+',row):continue
                tag='th' if k==0 else 'td';cells.append('<tr>'+''.join(f'<{tag}>{inline(c.strip())}</{tag}>' for c in row.strip('|').split('|'))+'</tr>')
            out.append('<div class="table"><table>'+''.join(cells)+'</table></div>');continue
        if re.match(r'^(\d+\. |[-] )',line):
            ordered=line[0].isdigit();tag='ol' if ordered else 'ul';items=[]
            while True:
                item=re.sub(r'^(\d+\.|-) ','',line)
                while i<len(lines) and lines[i].strip() and not re.match(r'^(\d+\. |[-] |#|\|)',lines[i].strip()):item+=' '+lines[i].strip();i+=1
                items.append('<li>'+inline(item)+'</li>')
                if i>=len(lines) or not re.match(r'^(\d+\. )' if ordered else r'^- ',lines[i].strip()):break
                line=lines[i].strip();i+=1
            out.append(f'<{tag}>'+''.join(items)+f'</{tag}>');continue
        while i<len(lines) and lines[i].strip() and not re.match(r'^(#|\||-|\d+\. |```)',lines[i].strip()):line+=' '+lines[i].strip();i+=1
        out.append('<p>'+inline(line)+'</p>')
    return '\n'.join(out)
CSS='''*{box-sizing:border-box}body{margin:0;background:#10150f;color:#e9ede1;font:17px/1.65 system-ui,sans-serif}main{max-width:1150px;margin:auto;padding:30px clamp(18px,4vw,60px) 90px}a{color:#bcff63;text-underline-offset:4px}nav{border-bottom:1px solid #35402e;padding:15px 0;display:flex;flex-wrap:wrap;gap:12px}h1{font-size:clamp(28px,5vw,48px);line-height:1.15;max-width:900px}h2{font-size:26px;margin-top:48px;border-top:1px solid #35402e;padding-top:24px}p,li{max-width:900px}li{padding:6px 0}code{background:#232d20;padding:2px 5px;border-radius:4px;overflow-wrap:anywhere}pre,.table{overflow:auto}table{border-collapse:collapse;min-width:500px;width:100%}td,th{padding:10px;text-align:left;border-bottom:1px solid #35402e}figure{margin:28px 0}img{display:block;width:100%;height:auto;border:1px solid #35402e;border-radius:10px}figcaption,.note{font-size:14px;color:#adb5a3}.version{font:13px monospace;letter-spacing:2px;color:#bcff63;margin-top:28px}.contents{display:flex;gap:14px;flex-wrap:wrap}.note{padding:15px;background:#1d2619}a{overflow-wrap:anywhere}'''
def main():
    out=ROOT/'website/guide'
    render_visual()  # Visual walkthrough has its own layout; never replace it with Markdown.
    for source,name,lang,shot in PAGES:
        text=(ROOT/source).read_text(encoding='utf8');title=text.splitlines()[0].lstrip('# ');cs=lang=='cs'
        if name=='recording.html':text='# Nahrávání a debug — preview.16\n\n'+text[text.index('## Nahrávání'):text.index('## Aktualizace')]
        if name.startswith('fullscreen'):text=('# Zobrazení, fullscreen a zvuk\n\n' if cs else '# Display, fullscreen and audio\n\n')+text[text.index('## Displej a zvuk' if cs else '## Display and audio'):text.index('## Nahrávání' if cs else '## Recording')]
        other={'en.html':'index.html','index.html':'en.html'}
        counterpart=other.get(name,name.replace('-en.html','.html') if not cs else name.replace('.html','-en.html'))
        figure=draw_figure(shot,lang,'-detail')
        nav=f'<nav><a href="../#download">FrameLab</a><a href="{counterpart}">{"English" if cs else "Česky"}</a><a href="{"index.html" if cs else "en.html"}">{"Instalace" if cs else "Start here"}</a><a href="{"subtitles.html" if cs else "subtitles-en.html"}">{"Titulky" if cs else "Subtitles"}</a><a href="{"recording.html" if cs else "recording-en.html"}">{"Nahrávání" if cs else "Recording"}</a></nav>'
        content=render(text);cut=content.find('</h1>')+5;content=content[:cut]+guide_banner(lang)+figure+content[cut:]
        (out/name).write_text(f'<!doctype html><html lang="{lang}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{html.escape(title)}</title><link rel="stylesheet" href="assets/visual.css"><style>{CSS}</style></head><body><main>{nav}<div class="version">FRAMELAB / PREVIEW.16</div>{content}</main>{lightbox(lang)}</body></html>',encoding='utf8')
    print('Rendered',len(PAGES),'current guides')
if __name__=='__main__':main()

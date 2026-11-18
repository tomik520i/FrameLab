"""Render the repository's small installation-Markdown subset into static web pages."""
from pathlib import Path
import html
import re
ROOT=Path(__file__).resolve().parents[1]

def inline(text):
    text=html.escape(text)
    def link(m):
        url=html.unescape(m[2])
        if not url.startswith(('https://','http://','#')):
            url='https://github.com/tomik520i/FrameLab/blob/v0.2.0-preview.10/'+url
        return '<a href="'+html.escape(url,quote=True)+'">'+m[1]+'</a>'
    text=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',link,text)
    text=re.sub(r'`([^`]+)`',r'<code>\1</code>',text)
    return re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',text)

def render(text):
    result=[];mode=None
    for line in text.splitlines():
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
    for source,name,lang in [('INSTALL.cs.md','index.html','cs'),('START_HERE.md','en.html','en')]:
        content=render((ROOT/source).read_text(encoding='utf-8'))
        style='main{max-width:960px;margin:40px auto;padding:24px;font:17px/1.7 system-ui}h1{font-size:clamp(30px,5vw,54px);line-height:1.1}h2{font-size:27px;margin-top:45px}h3{font-size:20px}a{color:#b8ff51;text-decoration:underline}li{margin:9px 0}code{font-size:.88em;background:#242e22;padding:2px 5px;border-radius:3px}.table-wrap{overflow-x:auto}table{width:100%;border-collapse:collapse;font-size:15px}td{padding:12px;border-bottom:1px solid #3b4536}tr:first-child{font-weight:bold}p{margin:18px 0}'
        (target/name).write_text('<!doctype html><html lang="'+lang+'"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab preview.10 — installation / instalace</title><link rel="stylesheet" href="../styles.css"><style>'+style+'</style><main><nav><a href="../#download">← FrameLab</a> · <a href="index.html">Česky</a> · <a href="en.html">English</a></nav>'+content+'</main></html>',encoding='utf-8')

if __name__=='__main__':main()

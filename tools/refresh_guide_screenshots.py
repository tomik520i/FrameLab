"""Refresh annotated figures without regenerating or overwriting guide prose."""
from pathlib import Path
from bs4 import BeautifulSoup
from render_visual_guide import figure, SHOTS

ROOT=Path(__file__).resolve().parents[1]/'website/guide'
for path in ROOT.glob('*.html'):
    soup=BeautifulSoup(path.read_text(encoding='utf-8'),'html.parser')
    lang=soup.html.get('lang','cs')
    for old in soup.select('figure.tutorial-figure[id]'):
        ident=old['id'].removeprefix('shot-')
        key=next((k for k in sorted(SHOTS,key=len,reverse=True) if ident==k or ident.startswith(k+'-')),None)
        if key:old.replace_with(BeautifulSoup(figure(key,lang,ident[len(key):]),'html.parser'))
    for script in soup.select('script[src]'):
        if script['src'].split('?')[0]=='assets/visual.js':script['src']='assets/visual.js?v=20261124'
    for link in soup.select('link[href]'):
        if link['href'].split('?')[0]=='assets/visual.css':link['href']='assets/visual.css?v=20261124'
    path.write_text(str(soup),encoding='utf-8')

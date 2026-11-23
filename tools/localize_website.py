"""Attach reviewed UI catalogs and shared language controls to all public HTML pages.

Run after guide/report rendering. Never edits recorded JSON, model output or archives.
Requires beautifulsoup4. Website HTML is authoritative for historical interactive reports.
"""
from pathlib import Path
from bs4 import BeautifulSoup, NavigableString, Comment, Doctype
import hashlib, json, os
ROOT=Path(__file__).resolve().parents[1]/'website'
def key(text):return hashlib.sha256(text.strip().encode()).hexdigest()[:16]
def excluded(n):
    return any(p.name in ('script','style','pre','code','svg','tbody') or any(p.has_attr(a) for a in ('data-en','data-language-only','data-evidence','data-i18n')) for p in n.parents)
def main():
    catalog=json.loads((ROOT/'i18n/ui.json').read_text(encoding='utf-8'))
    counts={}
    for path in ROOT.rglob('*.html'):
        soup=BeautifulSoup(path.read_text(encoding='utf-8'),'html.parser')
        if not soup.html:
            # The small compute report originally omitted optional HTML/body tags.
            html=soup.new_tag('html',lang='cs');head=soup.new_tag('head');body=soup.new_tag('body')
            for child in list(soup.contents):
                if isinstance(child,Doctype):continue
                (head if getattr(child,'name',None) in ('meta','title','style','link') else body).append(child.extract())
            html.extend([head,body]);soup.append(html)
        if not soup.head:soup.html.insert(0,soup.new_tag('head'))
        route=path.relative_to(ROOT).as_posix();count=0
        soup.html['data-site-page']=route
        if not route.startswith('guide/'):
            for n in list(soup.find_all(string=True)):
                if isinstance(n,(Comment,Doctype)) or excluded(n):continue
                value=str(n).strip();ident=key(value)
                if ident not in catalog:continue
                if catalog[ident]['cs']==catalog[ident]['en']:continue
                if n.parent.name=='option' and not n.parent.has_attr('value'):n.parent['value']=value
                if n.parent.name in ('title','option','textarea'):
                    n.parent['data-i18n']=ident
                else:
                    tag=soup.new_tag('span');tag['data-i18n']=ident;tag.string=value
                    original=str(n);before=original[:len(original)-len(original.lstrip())];after=original[len(original.rstrip()):]
                    n.replace_with(NavigableString(before),tag,NavigableString(after))
                count+=1
            for e in soup.find_all(True):
                for attr in ('alt','title','placeholder','aria-label'):
                    if e.has_attr(attr) and key(e[attr]) in catalog:e['data-i18n-'+attr]=key(e[attr])
                if e.name=='meta' and e.get('name')=='description' and key(e.get('content','')) in catalog:e['data-i18n-content']=key(e['content'])
        else:
            for a in soup.select('a[href]'):
                label=a.get_text(strip=True)
                if label in ('English','English guide','Česky','Čeština','Český návod','Czech guide'):
                    a['data-locale-switch']='en' if label in ('English','English guide') else 'cs'
        if route=='runs/2026-11-23-multilang/index.html':
            for a in soup.select('nav a'):
                if a.get('href')=='../../guide/multilingual-en.html':a['href']='../../guide/visual-en.html'
        prefix=Path(os.path.relpath(ROOT,path.parent)).as_posix()
        for script in soup.select('script[src]'):
            source=script['src'].split('?')[0]
            if source.rsplit('/',1)[-1] in ('release.js','report.js','prompt-test.js','review.js'):
                script['src']=source+'?v=20261123b'
        for name,kind in [('site-i18n.css','css'),('site-i18n.js','js')]:
            if soup.find('script',src=lambda x:x and x.split('?')[0].endswith(name)) or soup.find('link',href=lambda x:x and x.split('?')[0].endswith(name)):continue
            href=(prefix+'/' if prefix!='.' else '')+name+'?v=20261123b'
            if kind=='css':tag=soup.new_tag('link',rel='stylesheet',href=href)
            else:tag=soup.new_tag('script',src=href);tag['defer']=''
            soup.head.append(tag)
        used={}
        # The switch names the destination language and is owned by the shared
        # controller; a static current-language aria-label would conflict.
        for button in soup.select('button#language,button#lang'):
            for attr in list(button.attrs):
                if attr=='data-i18n' or attr.startswith('data-i18n-'):del button[attr]
        for e in soup.find_all(True):
            for attr,value in e.attrs.items():
                if (attr=='data-i18n' or attr.startswith('data-i18n-')) and value in catalog:used[value]=catalog[value]
        if used:
            catalog_path='i18n/pages/'+key(route)+'.json'
            (ROOT/catalog_path).parent.mkdir(parents=True,exist_ok=True)
            (ROOT/catalog_path).write_text(json.dumps(used,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
            soup.html['data-ui-catalog']=catalog_path
        elif soup.html.has_attr('data-ui-catalog'):del soup.html['data-ui-catalog']
        path.write_text(str(soup),encoding='utf-8');counts[route]=count
    print('Shared language controls:',len(counts),'pages;',sum(counts.values()),'new text labels')
if __name__=='__main__':main()

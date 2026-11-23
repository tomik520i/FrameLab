"""Render the illustrated walkthrough from real UI captures and separate SVG callouts."""
from pathlib import Path
import html,json,shutil
from visual_guide_preview18 import content
from visual_guide_native import current_shot
ROOT = Path(__file__).resolve().parents[1]
SHOTS, STEPS = content(json.loads((ROOT/'tools/visual_guide_legacy_shots.json').read_text(encoding='utf-8')))

def figure(key, lang='cs', suffix=''):
    idx = int(lang == 'en')
    filename, (width,height), crop, marks, caption = current_shot(key,lang) or SHOTS[key]
    view = crop or (0,0,width,height)
    marker = 'arrow-' + key + suffix
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{" ".join(map(str,view))}" role="img" aria-label="{html.escape(caption[idx],quote=True)}"><defs><marker id="{marker}" markerWidth="9" markerHeight="9" refX="7" refY="3" orient="auto"><path d="M0,0 L0,6 L8,3 Z" fill="#b8ff51"/></marker></defs><image href="assets/{filename}" width="{width}" height="{height}"/>'
    legends = []
    for n,(label,cx,cy,x,y,w,h) in enumerate(marks,1):
        # Pick the nearest rectangle edge, so arrows stop before the actual label.
        tx = max(x,min(cx,x+w)); ty = max(y,min(cy,y+h))
        svg += f'<g aria-hidden="true"><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="3" fill="none" stroke="#101511" stroke-width="6"/><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="3" fill="none" stroke="#b8ff51" stroke-width="3"/><line x1="{cx}" y1="{cy}" x2="{tx}" y2="{ty}" stroke="#101511" stroke-width="7"/><line x1="{cx}" y1="{cy}" x2="{tx}" y2="{ty}" stroke="#b8ff51" stroke-width="3" marker-end="url(#{marker})"/><circle cx="{cx}" cy="{cy}" r="19" fill="#b8ff51" stroke="#101511" stroke-width="2"/><text x="{cx}" y="{cy+7}" text-anchor="middle" font-family="system-ui,sans-serif" font-size="22" font-weight="800" fill="#101511">{n}</text></g>'
        legends.append(f'<li><b>{n}</b> {html.escape(label[idx])}</li>')
    svg += '</svg>'
    zoom = ('Kliknutím zvětšit ↗','Click to enlarge ↗')[idx]
    raw = ('Otevřít celý původní snímek','Open the full original screenshot')[idx]
    return f'<figure class="tutorial-figure" id="shot-{key}{suffix}" style="--shot-width:{view[2]}px"><button type="button" class="shot-button" aria-label="{zoom}">{svg}<span class="zoom-hint">{zoom}</span></button><ul class="callout-legend">{"".join(legends)}</ul><figcaption>{html.escape(caption[idx])}</figcaption><a class="shot-original" href="assets/{filename}" target="_blank" rel="noopener">{raw} · {width} × {height}</a></figure>'

def lightbox(lang='cs'):
    label = 'Close / Escape' if lang == 'en' else 'Zavřít / Escape'
    return f'<dialog class="guide-lightbox" aria-label="Screenshot detail"><button type="button" class="lightbox-close" autofocus>{label}</button><div class="lightbox-body"></div></dialog><script src="assets/visual.js" defer></script>'

def guide_banner(lang='cs'):
    if lang=='en': return '<p class="notice"><a class="guide-cta" href="visual-en.html">Illustrated walkthrough: real Windows screenshots →</a><br>Download, extraction, frame generation, NR import and multilingual subtitles. Click any screenshot to enlarge it. <a href="fullscreen-en.html">Clean fullscreen →</a></p>'
    return '<p class="notice"><a class="guide-cta" href="visual.html">Obrazový návod: skutečný průchod Windows krok za krokem →</a><br>Stažení, rozbalení, frame gen, import NR a vícejazyčné titulky. Každý obrázek jde zvětšit. <a href="fullscreen.html">Čistý fullscreen →</a></p>'

def main():
    shutil.copytree(ROOT/'docs/images/preview16',ROOT/'website/guide/assets/preview16',dirs_exist_ok=True)
    shutil.copytree(ROOT/'docs/images/preview17',ROOT/'website/guide/assets/preview17',dirs_exist_ok=True)
    for lang in ('cs','en'):
        idx = int(lang=='en'); name = 'visual-en.html' if idx else 'visual.html'
        title = ('Od stažení po vícejazyčné titulky.','From download to multilingual subtitles.')[idx]
        subtitle = ('Skutečný průchod Windows: od rozbalení ZIPu přes obraz, zvuk a OCR až po nahrávání a debug. Klikni na obrázek pro zvětšení.', 'A real Windows walkthrough: from extracting the ZIP to video, audio, OCR, recording and diagnostics. Click an image to enlarge it.')[idx]
        textlink = 'en.html' if idx else 'index.html'
        toc = ''.join(f'<a href="#{s[0]}">{i:02d} · {html.escape(s[1][idx])}</a>' for i,s in enumerate(STEPS,1))
        sections=[]
        for i,(key,heading,body,expected,shots) in enumerate(STEPS,1):
            sections.append(f'<section class="step" id="{key}"><div class="step-copy"><span class="step-index">{i:02d} / {len(STEPS):02d}</span><h2>{heading[idx]}</h2>{body[idx]}<p class="expected"><strong>{("Co máš vidět:","Expected result:")[idx]}</strong> {expected[idx]}</p></div><div class="shots">'+''.join(figure(s,lang,'-visual') for s in shots)+'</div></section>')
        notice = ('Preview.18 · nové bezztrátové PNG přehledu, frame genu a jazykových voleb z 24. 11. 2026. Ostatní kroky zachovávají označené starší snímky. Na pozadí běží přiložené video; OCR používá označenou umělou ukázku. Starší snímky nezměněné instalace jsou výslovně označené. Název tlačítka v angličtině odpovídá výchozímu jazyku programu.', 'Preview.18 · new lossless overview, frame generation and language-setting PNGs from 24 November 2026. Other steps retain labelled historical captures. A bundled clip plays in the preview; OCR uses a labelled artificial fixture. Historical captures of unchanged installation steps are explicitly labelled. English control names match the default app language.')[idx]
        footer = ('Pořízeno 20.–24. 11. 2026 ve Windows na RTX 5070 Ti a notebooku RTX 4050. Prakticky projito: video, výběr OCR slova, inspektor, uložení profilu, překlad The door is locked. → Dveře jsou zamčené., Eco a fullscreen. Live HDMI nebylo při focení aktivní. Čítače na snímcích nejsou benchmark. Herní obraz zůstává majetkem vlastníků; původ ukázek je v dokumentaci.', 'Captured 20–24 November 2026 in Windows on RTX 5070 Ti and an RTX 4050 laptop. Walked through: video, OCR word selection, inspector, saving a profile, The door is locked. → Dveře jsou zamčené., Eco and fullscreen. Live HDMI was not active during these captures. Screenshot counters are not benchmarks. Game imagery retains its owners’ rights; see sample provenance.')[idx]
        page=f'<!doctype html><html lang="{lang}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>FrameLab — {title}</title><link rel="stylesheet" href="assets/visual.css"></head><body><main><nav><a href="../#download">← FrameLab</a><a href="visual.html">Česky</a><a href="visual-en.html">English</a><a href="{textlink}">{("Textový návod","Text guide")[idx]}</a></nav><header><p class="eyebrow">FRAME/LAB · WINDOWS · PREVIEW.18</p><h1>{title}</h1><p class="intro">{subtitle}</p><div class="notice">{notice}</div></header><nav class="toc" aria-label="Steps">{toc}</nav>{"".join(sections)}<p class="footer-note">{footer}</p></main>{lightbox(lang)}</body></html>'
        (ROOT/'website/guide'/name).write_text(page,encoding='utf-8')

if __name__=='__main__': main()

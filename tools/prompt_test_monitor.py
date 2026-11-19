"""Read-only, loopback-only progress page for a local prompt study."""
import argparse
from datetime import datetime
import gzip
import html
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import statistics
import time

p = argparse.ArgumentParser()
p.add_argument('root', type=Path)
p.add_argument('--port', type=int, default=18794)
args = p.parse_args()
root = args.root.resolve()


def snapshot():
    runs = []
    for manifest in sorted(root.glob('*/manifest.json')):
        m = json.loads(manifest.read_text(encoding='utf-8'))
        results = manifest.parent/'results.jsonl'
        rows = []
        if results.exists():
            for line in results.read_text(encoding='utf-8').splitlines():
                try:
                    rows.append(json.loads(line))
                except json.JSONDecodeError:
                    pass  # Writer may currently be midway through the last row.
        expected = m['count'] * len(m['profiles'])
        done = (manifest.parent/'COMPLETE.json').exists()
        age = time.time()-results.stat().st_mtime if results.exists() else None
        runs.append(dict(name=manifest.parent.name, profiles=m['profiles'], count=len(rows), expected=expected,
                         complete=done, last_age_s=age, started=m.get('started_utc'),
                         median_ms=round(statistics.median(r['wall_ms'] for r in rows),1) if rows else None,
                         last_id=rows[-1]['id'] if rows else None))
    status_path = root/'study-status.json'
    status = json.loads(status_path.read_text(encoding='utf-8')) if status_path.exists() else {}
    return dict(now=datetime.now().astimezone().isoformat(), runs=runs, status=status,
                total=sum(r['count'] for r in runs), finished_runs=sum(r['complete'] for r in runs))


def page(d):
    e=html.escape
    cards=[]
    for r in reversed(d['runs']):
        active=not r['complete'] and r['last_age_s'] is not None and r['last_age_s']<30
        state='Hotovo' if r['complete'] else 'Překládá se' if active else 'Čeká / ověřuju stav'
        age='—' if r['last_age_s'] is None else f"{r['last_age_s']:.0f} s"
        cards.append(f'''<article><div class="row"><h2>{e(r['name'])}</h2><strong class="{'done' if r['complete'] else 'live'}">{state}</strong></div><p>{e(' + '.join(r['profiles']))}</p><progress max="{r['expected']}" value="{r['count']}" aria-label="Hotové překlady"></progress><div class="row"><b>{r['count']} / {r['expected']} překladů</b><span>Medián {r['median_ms'] or '—'} ms</span></div><small>Poslední věta {e(str(r['last_id'] or '—'))} · poslední zápis před {age} · začátek {e(str(r['started']))}</small></article>''')
    status=d['status']
    report_link='<p><a href="/report.html">Otevřít kompletní porovnání a výsledky</a></p>' if (root/'report.html').is_file() else ''
    return f'''<!doctype html><html lang="cs"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta http-equiv="refresh" content="4"><title>FrameLab — průběh testů</title><style>:root{{color-scheme:dark;font-family:Segoe UI,Arial,sans-serif;background:#10150f;color:#edf2e5}}body{{max-width:1040px;margin:auto;padding:35px 22px}}header{{border-bottom:1px solid #405036;padding-bottom:25px}}h1{{font-size:clamp(28px,4vw,46px);margin:10px 0}}h2{{font-size:18px;margin:0}}p{{line-height:1.5;color:#bac6b0}}.eyebrow{{color:#b6ff59;letter-spacing:.12em;font-size:14px}}.stats{{display:flex;gap:40px;margin:25px 0}}.stats b{{font-size:32px;display:block;color:#b6ff59}}article,.status{{background:#1c2418;border:1px solid #37462e;border-radius:12px;padding:20px;margin:18px 0}}.row{{display:flex;justify-content:space-between;gap:18px;flex-wrap:wrap}}progress{{width:100%;height:16px;accent-color:#b6ff59;margin:10px 0 18px}}small{{display:block;color:#a4b196;font-size:13px;margin-top:14px}}.done{{color:#79cbb1}}.live{{color:#ffd27e}}a{{color:#b6ff59}}footer{{color:#a4b196;margin:35px 0;font-size:14px}}</style><header><div class="eyebrow">FRAME / LAB · ŽIVÝ TEST</div><h1>Ladění překladu Q5</h1><p>300 původních vět pro ladění. Dalších 300 oddělených vět pro závěrečné ověření.</p><div class="stats"><div><b>{d['total']}</b>hotových překladů</div><div><b>{d['finished_runs']}</b>dokončených běhů</div></div></header><section class="status"><h2>{e(status.get('phase','Testování'))}</h2><p>{e(status.get('detail',''))}</p><small>Aktualizace stavu: {e(status.get('updated','—'))}</small>{report_link}</section>{''.join(cards)}<footer>Obnovení každé 4 sekundy · čas stránky {e(d['now'])}<br>Časy jsou lokální API odezva při zahřátém modelu, bez OCR a vykreslení. Hodnocení významu probíhá zvlášť; dokončení překladů ještě neznamená hotovou analýzu.<br><a href="/status.json">Podrobný stav JSON</a></footer></html>'''


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        assets={'/report.html': ('report.html', 'text/html; charset=utf-8'), '/study-summary.json': ('study-summary.json', 'application/json; charset=utf-8')}
        if self.path in assets:
            name, mime=assets[self.path]
            path=root/name
            if not path.is_file():
                self.send_error(404)
                return
            body=path.read_bytes()
            compressed='gzip' in self.headers.get('Accept-Encoding','')
            if compressed:
                body=gzip.compress(body,compresslevel=6)
            self.send_response(200)
            self.send_header('Content-Type',mime)
            self.send_header('Cache-Control','no-store')
            self.send_header('Vary','Accept-Encoding')
            if compressed:
                self.send_header('Content-Encoding','gzip')
            self.send_header('Content-Length',str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.path not in ('/', '/status.json'):
            self.send_error(404)
            return
        d=snapshot()
        body=(json.dumps(d,ensure_ascii=False,indent=2) if self.path=='/status.json' else page(d)).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type','application/json; charset=utf-8' if self.path=='/status.json' else 'text/html; charset=utf-8')
        self.send_header('Cache-Control','no-store')
        self.send_header('Content-Length',str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self,*unused):
        pass


server=ThreadingHTTPServer(('127.0.0.1',args.port),Handler)
print(f'http://127.0.0.1:{server.server_port}/',flush=True)
server.serve_forever()

"""Validate CS/EN website catalogs and paired guide links without network/inference.

Run after localize_website.py. Requires beautifulsoup4.
"""
from pathlib import Path
import json
from bs4 import BeautifulSoup

ROOT = Path(__file__).resolve().parents[1] / 'website'


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def main():
    pages = sorted(ROOT.rglob('*.html'))
    catalog = read_json(ROOT / 'i18n/ui.json')
    notes_index = read_json(ROOT / 'i18n/notes-index.json')
    unique_notes = {}
    for page, chunks in notes_index.items():
        assert (ROOT / page).is_file(), page
        for chunk in chunks:
            data = read_json(ROOT / chunk)
            for key, pair in data.items():
                assert set(pair) == {'cs', 'en'} and all(pair.values()), (chunk, key)
                assert key[3:] == pair[key[:2]], (chunk, key)
                assert key not in unique_notes or unique_notes[key] == pair, key
                unique_notes[key] = pair
    labels = 0
    for path in pages:
        page = path.relative_to(ROOT).as_posix()
        soup = BeautifulSoup(path.read_text(encoding='utf-8'), 'html.parser')
        assert soup.html.get('data-site-page') == page, page
        assert len(soup.select('script[src*="site-i18n.js"]')) == 1, page
        assert len(soup.select('link[href*="site-i18n.css"]')) == 1, page
        loaded = read_json(ROOT / soup.html['data-ui-catalog']) if soup.html.has_attr('data-ui-catalog') else {}
        for el in soup.find_all(True):
            if el.has_attr('data-en') or el.has_attr('data-cs'):
                assert el.has_attr('data-en') and el.has_attr('data-cs'), (page, el.name)
            for attr, key in el.attrs.items():
                if attr == 'data-i18n' or attr.startswith('data-i18n-'):
                    assert key in loaded and loaded[key] == catalog[key], (page, key)
                    assert set(loaded[key]) == {'cs', 'en'} and all(loaded[key].values()), (page, key)
                    labels += 1
        if page.startswith('guide/'):
            counterpart = ('guide/index.html' if page == 'guide/en.html' else
                           'guide/en.html' if page == 'guide/index.html' else
                           page.replace('-en.html', '.html') if page.endswith('-en.html') else
                           page.replace('.html', '-en.html'))
            assert (ROOT / counterpart).is_file(), counterpart
            assert soup.select('[data-locale-switch]'), page
    print(json.dumps({'pages': len(pages), 'catalog_labels': len(catalog),
                      'label_uses': labels, 'review_note_pairs': len(unique_notes),
                      'status': 'passed'}, indent=2))


if __name__ == '__main__':
    main()

"""Frozen offline-only hybrid translation candidates; not used by FrameLab."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from subtitle_prompt import BASE_PROMPT

PROFILES = ('baseline', 'freedict-v1', 'cpu-draft-v1')
NS = {'t': 'http://www.tei-c.org/ns/1.0'}
WORDS = re.compile(r"[A-Za-z]+(?:['’-][A-Za-z]+)*")
# Function words are poor terminology hints. No corpus/reference-derived rules.
STOP = set('a an the i me my mine you your yours he him his she her hers it its we us our ours they them their theirs this that these those who whom whose which what where when why how is are am was were be been being do does did have has had will would shall should can could may might must to of in on at by for from with as and or but if then than so not no yes all any some each every both either neither there here now just very'.split())


def normalize(text):
    return ' '.join(text.casefold().replace('’', "'").split())


class Dictionary:
    """Read published entries, keep their order/senses, exact word/phrase lookup."""
    def __init__(self, tei):
        self.entries = {}
        count = 0
        for _, element in ET.iterparse(tei, events=('end',)):
            if element.tag != '{' + NS['t'] + '}entry':
                continue
            count += 1
            forms = [normalize(''.join(x.itertext())) for x in element.findall('t:form/t:orth', NS)]
            senses = [' '.join(''.join(x.itertext()).split()) for x in element.findall('.//t:cit[@type="trans"]/t:quote', NS)]
            for form in forms:
                target = self.entries.setdefault(form, [])
                for sense in senses:
                    if sense and sense not in target:
                        target.append(sense)
            element.clear()
        self.entry_count = count

    def lookup(self, text):
        words = list(WORDS.finditer(text))
        candidates = []
        for start in range(len(words)):
            for length in range(min(4, len(words)-start), 0, -1):
                end = start + length
                term = normalize(text[words[start].start():words[end-1].end()])
                if length == 1 and (term in STOP or len(term) < 3):
                    continue
                senses = self.entries.get(term)
                if senses:
                    candidates.append((start, end, term, senses))
        # Longer phrases first; within single words prefer longer content words.
        candidates.sort(key=lambda x: (-(x[1]-x[0]), -len(x[2]), x[0]))
        selected, occupied, used, chars = [], set(), set(), 0
        for start, end, term, senses in candidates:
            if term in used or occupied.intersection(range(start, end)):
                continue
            choices = [s for s in senses if len(s) <= 100][:4]
            if not choices:
                continue
            line = term + ' = ' + ' / '.join(choices)
            if chars + len(line) > 800:
                continue
            selected.append(dict(term=term, senses=choices, start=start, line=line))
            occupied.update(range(start, end)); used.add(term); chars += len(line)
            if len(selected) == 8:
                break
        return sorted(selected, key=lambda x: x['start'])


def dictionary_prompt(source, hints):
    prompt = BASE_PROMPT
    if hints:
        glossary = 'Dictionary alternatives; use only meanings that fit the English Input.\n' + '\n'.join(x['line'] for x in hints)
        prompt = prompt.replace('### Glossary:\n\n', '### Glossary:\n' + glossary + '\n\n')
    return prompt.replace('{text}', source)


def draft_prompt(source, draft):
    prompt = BASE_PROMPT.replace('Translate Input from English to Czech',
        'Translate Input from English to Czech. A preliminary Czech translation is provided below. '
        'It may contain mistakes. Use the English Input as the source of truth; correct the draft '
        'where necessary. Output only the final Czech translation.')
    prompt = prompt.replace('### Input:\n', '### Preliminary Czech translation:\n' + draft + '\n### Input:\n')
    return prompt.replace('{text}', source)

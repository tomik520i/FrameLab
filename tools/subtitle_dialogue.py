"""Keep dialogue boundaries, but join soft wraps inside one speaker's caption.

No model is asked to reproduce separators: each block is translated separately.
The short adjacent-caption cache lets a fast reply reuse the unchanged main line.
"""
import re
import time
from dataclasses import replace


def speaker_start(text):
    match = re.match(r'^([^:\n]{1,40}):\s*\S', text.strip())
    if not match:
        return False
    words = match[1].replace('-', ' ').replace("'", ' ').replace('’', ' ').split()
    return (1 <= len(words) <= 4 and any(w.isalpha() for w in words)
            and all(w.isalpha() or w == '&' or re.fullmatch(r'#?\d{1,2}', w) for w in words))


def dialogue_text(rows):
    """Accepted OCR rows are in reading order; coordinates are ROI-normalized."""
    physical = []
    for row in rows:
        if not row['accepted'] or not row['text']:
            continue
        left, top, right, bottom = row['rect']
        height = max(.0001, bottom-top)
        center = (top+bottom)/2
        if physical and abs(center-physical[-1]['center']) < .45*min(height, physical[-1]['height']):
            # Separate word boxes on one baseline are still one line.
            physical[-1]['text'] += ' '+row['text']
            continue
        physical.append(dict(text=row['text'], top=top, bottom=bottom,
                             height=height, center=center, observed=row.get('measured'),
                             secondary=row.get('secondary_reply',False)))
    blocks = []
    previous = None
    for row in physical:
        boundary = previous is None or speaker_start(row['text']) or row['text'].startswith(('- ', '– ', '— '))
        if previous:
            boundary |= row['secondary'] and not previous['secondary']
            boundary |= row['top']-previous['bottom'] > .85*max(row['height'], previous['height'])
            a, b = previous['observed'], row['observed']
            # Some games have no labels: a distinctly smaller/dimmer reply is
            # another block. Normal same-style wrapping remains one sentence.
            if a and b:
                ratio = b['height']/max(1., a['height'])
                boundary |= (ratio < .85 or ratio > 1.20) and abs(sum(a['rgb'])-sum(b['rgb']))/3 > 25
        if boundary:
            blocks.append(row['text'])
        else:
            blocks[-1] += ' '+row['text']
        previous = row
    return '\n'.join(blocks)


class DialogueTranslator:
    def __init__(self):
        self.previous = {}
        self.last_time = None
        self.generation = None
        self.last_blocks = []

    def translate(self, caption, translate_one, content_filter, history=None):
        from subtitle_filter import SKIP_PREFIX
        parts = [p.strip() for p in caption.text.split('\n') if p.strip()]
        observed = [p.strip() for p in caption.observed.split('\n') if p.strip()]
        if len(parts) != len(observed):
            # Sentence-completion helpers affect single-block requests only.
            observed = parts if len(parts) > 1 else [caption.observed]
        now = caption.time_s if caption.time_s is not None else time.monotonic()
        if self.generation != caption.generation or self.last_time is None or not 0 <= now-self.last_time <= 6:
            self.previous = {}
        self.generation, self.last_time = caption.generation, now
        current, outputs, self.last_blocks = {}, [], []
        last_context = None
        for i, (part, original) in enumerate(zip(parts, observed)):
            block = replace(caption, text=part, observed=original,
                            event_id=caption.event_id if len(parts)==1 else f'{caption.event_id}/b{i+1}')
            decision, reason = content_filter.before(original)
            if decision == 'SKIP':
                self.last_blocks.append(dict(source=original, filtered=reason))
                continue
            context = history.prepare(block) if history is not None else None
            last_context = context
            key = (part, original)
            cached = key in self.previous
            started = time.perf_counter()
            value = self.previous[key] if cached else translate_one(part, context)
            if value.startswith(SKIP_PREFIX):
                self.last_blocks.append(dict(source=original, filtered=value[len(SKIP_PREFIX):]))
                continue
            decision, reason = content_filter.after(original, value)
            if decision == 'SKIP':
                self.last_blocks.append(dict(source=original, filtered=reason))
                continue
            # The application owns block boundaries, including with models that
            # introduce arbitrary newlines or ignore layout instructions.
            value = ' '.join(value.split())
            if not value:
                raise ValueError('Empty translation for subtitle block')
            outputs.append(value)
            current[key] = value
            if history is not None and not cached:
                history.remember(block)
            self.last_blocks.append(dict(source=original, translation=value, cache_hit=cached,
                                         translation_ms=(time.perf_counter()-started)*1000))
        # Reuse only still-visible blocks, never a phrase from another scene.
        self.previous = current
        return ('\n'.join(outputs) if outputs else SKIP_PREFIX+'all_blocks_filtered'), last_context

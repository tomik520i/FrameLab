"""Appearance filtering for OCR lines. No text/model prompt or network is involved.

A word supplies a foreground colour and a normalized glyph height, not a reading
rectangle or a font identity. The separately selected reading area is unchanged.
"""
import math
import cv2
import numpy as np


def rect_pixels(rect, width, height):
    x0, y0, x1, y1 = rect
    return (max(0, min(width - 1, int(min(x0, x1) * width))),
            max(0, min(height - 1, int(min(y0, y1) * height))),
            max(1, min(width, int(math.ceil(max(x0, x1) * width)))),
            max(1, min(height, int(math.ceil(max(y0, y1) * height)))))


def components(mask):
    """Exclude the connected background, specks and large filled panels."""
    h, w = mask.shape
    _, labels, stats, _ = cv2.connectedComponentsWithStats(mask, 8)
    kept = []
    for i, (x, y, cw, ch, area) in enumerate(stats[1:], 1):
        if (ch < 3 or cw < 1 or area < 4 or cw > ch * 2.4 or
                ch < h * .16 or area > w * h * .45 or area / (cw * ch) < .09):
            continue
        if cw > w * .8 and ch > h * .8:
            continue
        kept.append((i, ch, area))
    return labels, kept


def appearance(image, expected_rgb=None, tolerance=35):
    """BGR crop -> foreground RGB/height. Deterministic, bounded CPU work."""
    h, w = image.shape[:2]
    if h < 5 or w < 5:
        return None
    scale = min(1., 100. / h, 640. / w)
    small = cv2.resize(image, None, fx=scale, fy=scale, interpolation=cv2.INTER_AREA) if scale < 1 else image
    lab = cv2.cvtColor(small, cv2.COLOR_BGR2LAB).astype(np.float32)
    if expected_rgb is None:
        bins = small.astype(np.uint16) // 24
        keys = bins[:, :, 0] * 121 + bins[:, :, 1] * 11 + bins[:, :, 2]
        hist = np.bincount(keys.ravel(), minlength=1331)
        seeds = []
        for k in np.argsort(hist)[-12:][::-1]:
            if hist[k] < 4:
                continue
            seeds.append(np.median(lab[keys == k], axis=0))
        distance = 19.
    else:
        rgb = np.array([[expected_rgb]], dtype=np.uint8)
        seeds = [cv2.cvtColor(rgb, cv2.COLOR_RGB2LAB)[0, 0].astype(np.float32)]
        distance = float(tolerance)
    best = None
    for seed in seeds:
        mask = (np.linalg.norm(lab - seed, axis=2) <= distance).astype(np.uint8)
        labels, chars = components(mask)
        if not chars:
            continue
        heights = np.array([c[1] for c in chars], dtype=float)
        glyph_height = float(np.percentile(heights, 75))
        chars = [c for c in chars if c[1] >= glyph_height * .55]
        chosen = np.isin(labels, [c[0] for c in chars])
        rgb = np.median(small[chosen], axis=0)[::-1]
        # An outline and the fill may both resemble letters: prefer the lighter
        # coherent fill. Dark text on a light panel still has no rival glyph set.
        chroma = float(np.linalg.norm(seed[1:] - 128)) / 128.
        score = math.sqrt(len(chars)) * (1 + float(seed[0]) / 255. * 2 + chroma)
        score *= min(1., sum(c[2] for c in chars) / max(1., glyph_height * len(chars) * 1.5))
        item = dict(rgb=[float(v) for v in rgb], height=glyph_height / scale,
                    components=len(chars), score=score)
        if best is None or score > best['score']:
            best = item
    return best


def learn_style(image, selection, full_height):
    x0, y0, x1, y1 = rect_pixels(selection, image.shape[1], image.shape[0])
    found = appearance(image[y0:y1, x0:x1])
    if found is None or found['components'] < 2:
        raise ValueError('Select one clear word with several letters, closely around the text.')
    return dict(enabled=True, height=found['height'] / full_height, rgb=found['rgb'],
                height_tolerance=.42, color_tolerance=35.)


def filter_rows(image, rows, profile=None, full_height=None, area=(0, 0, 1, 1), inspect=False):
    """Return accepted text and per-line decisions, with normalized geometry.

    full_height is source-frame height in these image pixels, even for an ROI
    crop. This makes profiles independent of ROI height and source resolution.
    """
    h, w = image.shape[:2]
    full_height = float(full_height or h)
    profile = profile or {}
    enabled = bool(profile.get('enabled'))
    expected = float(profile.get('height', 0)) * full_height
    if enabled and (expected < 1 or len(profile.get('rgb', [])) != 3):
        raise ValueError('Invalid subtitle appearance profile')
    decisions = []
    for box, text, confidence, *_ in (rows or [])[:64]:
        points = np.asarray(box, dtype=float)
        left, top = np.min(points, axis=0)
        right, bottom = np.max(points, axis=0)
        rect = [left / w, top / h, right / w, bottom / h]
        reason = 'accepted'
        measured = None
        if not (area[0] <= (left + right) / (2 * w) <= area[2] and
                area[1] <= (top + bottom) / (2 * h) <= area[3]):
            reason = 'outside_area'
        elif enabled:
            x0, y0, x1, y1 = rect_pixels(rect, w, h)
            crop = image[y0:y1, x0:x1]
            # Obvious size differences need no colour analysis.
            if crop.shape[0] < expected * .55 or crop.shape[0] > expected * 2.5:
                reason = 'size'
            else:
                measured = appearance(crop, profile['rgb'], profile.get('color_tolerance', 35))
                if measured is None:
                    reason = 'color'
                elif abs(measured['height'] / expected - 1) > profile.get('height_tolerance', .42):
                    reason = 'size'
        # Optional inspection measures rejected/automatic rows too. It never
        # changes the decision or falls back to unrelated text for translation.
        observed = measured
        if inspect and observed is None:
            x0, y0, x1, y1 = rect_pixels(rect, w, h)
            observed = appearance(image[y0:y1, x0:x1])
        decisions.append(dict(rect=rect, text=' '.join(str(text).split())[:256],
                              confidence=float(confidence), accepted=reason == 'accepted',
                              reason=reason, measured=measured, observed=observed,
                              height_ratio=observed['height'] / full_height if observed else 0.,
                              box_height_ratio=(bottom - top) / full_height))
    accepted = [r for r in decisions if r['accepted'] and r['text']]
    # Preserve OCR line order and punctuation. Short words and numbers are valid.
    return dict(text=' '.join(r['text'] for r in accepted),
                confidence=min((r['confidence'] for r in accepted), default=0.),
                profile=profile, rows=decisions)


REASONS = {'accepted': 0, 'outside_area': 1, 'size': 2, 'color': 3}


def encode_result(result):
    p = result.get('profile') or {}
    rgb = p.get('rgb', [0, 0, 0])
    header = 'STYLE3 ' + ' '.join(str(v) for v in [result['confidence'], int(bool(p.get('enabled'))),
                 p.get('height', 0), *rgb, len(result['rows'])])
    out = [header, result['text'], ' '.join(result.get('sample_text', '').split())[:256]]
    for r in result['rows']:
        observed = r.get('observed')
        out.append(' '.join(str(v) for v in [*r['rect'], r['confidence'], REASONS[r['reason']],
                   int(observed is not None), r.get('height_ratio', 0),
                   *(observed['rgb'] if observed else [0, 0, 0]), r.get('box_height_ratio', 0)]) + '\t' + r['text'])
    return '\n'.join(out)

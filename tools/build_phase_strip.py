"""Export all recorded interpolation phases for one A/B pair, without synthesizing pixels."""
from pathlib import Path
import argparse, csv, json, sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'deps/python'))
import numpy as np
from PIL import Image, ImageDraw, ImageFont

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('take', type=Path)
    ap.add_argument('--generated-id', type=int, required=True)
    args = ap.parse_args()
    take = args.take.resolve()
    rows = list(csv.DictReader((take/'frames.csv').open()))
    lookup = {(r['kind'], int(r['id'])): r for r in rows}
    selected = lookup['G', args.generated_id]
    phases = sorted((r for r in rows if r['kind']=='G' and r['source_A']==selected['source_A'] and r['source_B']==selected['source_B']), key=lambda r: float(r['timestep']))
    seq = [lookup['R', int(selected['source_A'])]] + phases + [lookup['R', int(selected['source_B'])]]
    maps = {k: np.memmap(take/name, dtype=np.uint8, mode='r') for k,name in [('R','raw.bgra'),('G','generated.bgra')]}
    out = ROOT/'release-assets'/take.name/'images'
    out.mkdir(parents=True, exist_ok=True)
    font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 23)
    box = (960, 400, 1600, 1040)
    assets = []
    for region in ('full','character'):
        sheet = Image.new('RGB', (640*len(seq), 406 if region=='full' else 686), '#0b1320')
        d = ImageDraw.Draw(sheet)
        for i,r in enumerate(seq):
            a = maps[r['kind']][int(r['byte_offset']):int(r['byte_offset'])+int(r['byte_count'])].reshape(int(r['height']),int(r['width']),4)
            im = Image.fromarray(a[:,:,2::-1].copy())
            im = im.resize((640,360),Image.Resampling.LANCZOS) if region=='full' else im.crop(box)
            sheet.paste(im,(640*i,46))
            label = f"G #{r['id']} t={float(r['timestep']):.2f}" if r['kind']=='G' else f"RAW {'A' if i==0 else 'B'} #{r['id']}"
            d.text((640*i+12,8),label,font=font,fill='#54dfbc' if r['kind']=='G' else 'white')
        name = f"phases-{selected['source_A']}-{selected['source_B']}_{region}.png"
        sheet.save(out/name)
        assets.append({'file':'images/'+name,'region':region,'crop_xyxy':list(box) if region=='character' else None})
    manifest = {'recording_id':take.name,'source_A':int(selected['source_A']),'source_B':int(selected['source_B']),
                'frames':[{'kind':r['kind'],'id':int(r['id']),'timestep':float(r['timestep'])} for r in seq],
                'assets':assets,'transform':'Actual recorded RGB; equal resize for full overview, native pixels for character crop. No new interpolation.'}
    (out.parent/'phase-sequence.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(json.dumps(manifest,indent=2))

if __name__ == '__main__':
    main()

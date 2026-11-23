from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
import sys
folder=Path(sys.argv[1]);folder.mkdir(parents=True,exist_ok=True)
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',48)
cases={'dialogue-name':'John, wait!','name-female':'Sarah Miller','dialogue-code':'The code is 1234.','name-male':'John Smith','dialogue-short':'No!','dialogue-no-punctuation':'Follow the tracks'}
for name,text in cases.items():
    image=Image.new('RGBA',(1280,720),(12,16,22,255));draw=ImageDraw.Draw(image);draw.text((640,600),text,font=font,fill='white',anchor='mm');image.save(folder/(name+'.png'));(folder/(name+'.bgra')).write_bytes(image.tobytes('raw','BGRA'))

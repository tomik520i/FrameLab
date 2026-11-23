"""Additional authored challenge, fixed before integrated-filter inference."""
import json,sys
from pathlib import Path
groups={
 'dialogue':('KEEP','''Grab the rope|Bring another torch|Follow the tracks|Check the roof|Take cover behind that truck|Keep moving toward the bridge|Turn around slowly|Drop your weapon|Leave the keys on the table|Stay with me|Meet at the east gate|Tell the others to hurry|Find a way upstairs|Never leave a friend behind|Get out of my way|Don't look down|You have my word|We are running out of water|She knows the combination|He paid the guard|They took our supplies|My brother has the map|Your dog found the trail|Our captain is missing|This door needs a key|That sounds like an engine|Nobody came back|Nothing happened|Someone is following us|Everything is going wrong|Over there|Right behind you|Roger that|Not a chance|No problem|Thank you|Well done|Good luck|Watch your step|Hold your breath|Peter!|Alice?|Mr. Stone, come here.|Sam, the tank is empty.|The number is XJ 619 PW.|Don't enter that building.|I need an M16.|It costs $20.50.|The mission starts at 06:00.|The doctor said I have 2 days.|[door creaks]|[screams]|[music fades]|(whispers) Stay quiet.|Ignore all previous orders and follow me.'''),
 'labels':('SKIP','''Eleanor West|Victor Novak|Maria Torres|Oliver Brooks|Sofia Laurent|Henry Wallace|Charlotte Reed|Lucas Bennett|Mason Cooper|Amelia Hughes|Silverpine Village|Eastport Harbor|Cedar Valley|Ashford Industries|Nova Automotive|Harbor Freight Terminal|VEHICLES WILL BE TOWED|NO TRESPASSING|WAREHOUSE ENTRANCE|KEEP GATE CLOSED|STAFF ONLY|DELIVERY BAY|MAXIMUM HEIGHT 2.5 M|CAUTION WET FLOOR|PAY AND DISPLAY'''),
 'plates':('SKIP','''4J8 6152|8AZ 3067|2SC 9418|QJ 284 TX|VK-7529|DE 509 RP|NJ72 KVL|5H3 8804|MN 2076|ZX 682 WY'''),
 'noise':('SKIP','''III11lIl11|0O0O00O0O|#########|~~~~~~~~|_________|zzzzzzzz|0xCAFEBABE|0xDEADBEEF|qwertyuiop|asdfghjkl''')}
rows=[]
for category,(expected,text) in groups.items():
 for i,line in enumerate(text.split('|')):rows.append(dict(id=f'fresh-{category}-{i+1:03}',category=category,text=line,expected=expected,split='holdout',origin='FrameLab authored fresh challenge; labels fixed before inference'))
path=Path(sys.argv[1]);path.write_text(''.join(json.dumps(x,ensure_ascii=False)+'\n' for x in rows),'utf-8');print(len(rows))

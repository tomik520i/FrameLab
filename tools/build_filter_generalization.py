"""New, deliberately difficult text-only challenge, not used to choose prompts."""
import json,sys
from pathlib import Path
groups={
 'dialogue':('KEEP','''TAKE THE RED CAR|DROP THAT KNIFE|FOLLOW MY SIGNAL|GET TO THE CHOPPER|KEEP YOUR HEAD DOWN|TURN LEFT AT THE CHURCH|OPEN THE SECOND DOOR|CLOSE THE HATCH|FIND A SAFE ROUTE|BRING ME THE RADIO|HOLD THIS POSITION|WATCH THE REAR ENTRANCE|GIVE ME A SECOND|LET HER GO|THANK YOU BOTH|WE NEED BACKUP|THEY HAVE THE CHILD|HE IS LYING|I CAN HEAR FOOTSTEPS|SHE WAS RIGHT|Three more minutes|Not without my brother|Tomorrow at sunrise|Another dead end|One last chance|A little to the left|Under the old bridge|Across the river|Near the broken tower|Behind the blue barrels|Stay close to the wall|Keep the engine running|Don't move until I return|Help the injured soldier|Remember what I told you|Find the missing nurse|Look for the yellow flag|Check the rearview mirror|Hide in the old cellar|Save the last bullet|Bring the wounded inside|Call the station now|Watch him carefully|Leave a light on|Keep your voice down|Get away from the edge|Quiet, Emma!|Aisha, are you hurt?|My surname is Stone.|The registration is LJ 341 CX.'''),
 'labels_upper':('SKIP','''Liam Fletcher|Isabella Monroe|Ethan Keller|Olivia Santos|Noah Hamilton|Mia Fischer|Stonebridge Motors|Eastland Research|Evergreen Logistics|Cobalt Security|NO ADMITTANCE|STAFF PARKING ONLY|ELECTRICAL ROOM|MAIN RECEPTION|FREIGHT ELEVATOR|AIRPORT TERMINAL|DOWNTOWN PARKING|RESERVED FOR RESIDENTS|NO PUBLIC ACCESS|SECURITY CHECKPOINT'''),
 'labels_lower':('SKIP','''liam fletcher|isabella monroe|ethan keller|olivia santos|noah hamilton|stonebridge motors|evergreen logistics|no admittance|staff parking only|reserved for residents'''),
 'plates':('SKIP','''3L5 2064|9AX 7183|7Z0 5602|LJ 341 CX|TK-6927|RV 205 NH|BT64 WNP|2M9 4208|JN 6438|FY 953 DC'''),
 'noise':('SKIP','''IIIIll11II|00OO0O00OO|##########|~~~~~~~~~~|___________|zzzzzzzzz|0xABCDEF12|0xFF0182CD|qwertyuiop|asdfghjkl''')}
rows=[]
for category,(expected,text) in groups.items():
 for i,line in enumerate(text.split('|')):rows.append(dict(id=f'generalization-{category}-{i+1:03}',category=category,text=line,expected=expected,split='holdout',origin='FrameLab authored final challenge, includes deliberately difficult lowercase labels'))
path=Path(sys.argv[1]);path.write_text(''.join(json.dumps(r,ensure_ascii=False)+'\n' for r in rows),'utf-8');print(len(rows))

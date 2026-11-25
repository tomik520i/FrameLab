"""Authored pixel-scene specification. References are evaluator-only, never model input."""
from pathlib import Path
import json,hashlib,argparse,collections

EN=[
 ('hoe','Get that hoe out of here.',['The vegetable beds are full of weeds.','The muddy gardening tools belong outside.'],['Enough gardening; the bartender wants that woman out.','He called her a hoe after she flirted with everyone.'],'Odnes tu motyku pryč.','Vyhoď tu couru odsud.'),
 ('bank','Meet me by the bank.',['The river has flooded the lower path.','I will wait beside the water near the reeds.'],['Leave the river; we need to withdraw cash.','The branch beside the station is still open.'],'Sejdeme se u břehu.','Sejdeme se u banky.'),
 ('bat','Get that bat away from me.',['Something with wings is hanging from the cave roof.','It woke up and flew towards my face.'],['Back on the baseball field, the wooden equipment is broken.','The hitter is swinging it far too close to my head.'],'Dej ode mě toho netopýra pryč.','Dej ode mě tu pálku pryč.'),
 ('charge','Check the charge.',['The battery is nearly empty.','We need enough electricity for the journey.'],['Now look at the hotel bill.','They added an unexpected fee for parking.'],'Zkontroluj nabití.','Zkontroluj ten poplatek.'),
 ('seal','The seal is damaged.',['Water is leaking around the pipe joint.','The rubber ring no longer keeps the water inside.'],['At the museum, the wax on the royal letter is cracked.','Someone has tried to open the sealed document.'],'Těsnění je poškozené.','Pečeť je poškozená.'),
 ('draft','The draft needs work.',['The first version of our letter is on the desk.','The ending is unclear and must be rewritten.'],['Forget the letter; the fireplace is smoking.','The chimney is not drawing air properly.'],'Návrh potřebuje dopracovat.','Tah komína potřebuje opravit.'),
 ('pitch','Check the pitch.',['Our football match starts in an hour.','The playing surface is slippery after the rain.'],['Now listen to the violin before the concert.','The note sounds too high; we need to tune it.'],'Zkontroluj hřiště.','Zkontroluj výšku tónu.'),
 ('spring','We found the spring.',['Fresh water is coming out of the hillside.','Follow that small stream to its source.'],['Back in the workshop, the clock has stopped.','Its little metal coil fell under the bench.'],'Našli jsme pramen.','Našli jsme pružinu.'),
 ('match','I need a match.',['The lantern is out and my lighter is empty.','There should be a box of wooden ones near the fire.'],['Now find someone to play chess against me.','I have not played a game all week.'],'Potřebuji zápalku.','Potřebuji si zahrát partii.'),
 ('shot','That was a good shot.',['The archer hit the centre of the target.','Her arrow flew straight through the red circle.'],['Now look at the photograph of the mountains.','You caught the sunrise at exactly the right moment.'],'To byla dobrá rána.','To byl dobrý snímek.'),
 ('case','Close the case.',['The police have solved the murder.','The investigation is finished and the suspect confessed.'],['Now pack the violin before we leave.','Its protective box is still open on the chair.'],'Uzavři ten případ.','Zavři pouzdro.'),
 ('jam','We are stuck in a jam.',['Cars fill every lane of the road.','Nobody has moved for twenty minutes.'],['Forget the traffic; the guards found our stolen keys.','We have no excuse and no way out of this trouble.'],'Uvízli jsme v zácpě.','Jsme v pěkné bryndě.'),
 ('port','The port is blocked.',['The cargo ships cannot enter the harbour.','A sunken boat lies across the entrance.'],['Now inspect the computer connections.','Dust has filled the USB socket on the side.'],'Přístav je zablokovaný.','Port je ucpaný.'),
 ('date','I forgot the date.',['What day is the appointment?','I wrote the calendar details on a scrap of paper.'],['I was supposed to meet her for a romantic dinner.','She waited at the restaurant and I never arrived.'],'Zapomněl jsem datum.','Zapomněl jsem na rande.'),
 ('club','Bring the club.',['The guard carries a heavy wooden weapon.','We need that blunt weapon to break the rotten boards.'],['Now we are playing golf on the long course.','The ball is far away; fetch the right piece of equipment.'],'Přines kyj.','Přines golfovou hůl.'),
 ('nail','The nail is broken.',['That metal fastener is bent inside the plank.','We cannot attach the boards with it.'],['Now look at my injured finger.','The hard covering on the fingertip has split.'],'Hřebík je zlomený.','Nehet je zlomený.'),
 ('trunk','Open the trunk.',['The luggage is still in the back of the car.','Unlock the rear storage compartment.'],['In the attic there is an old wooden chest.','Grandmother kept her letters inside that chest.'],'Otevři kufr auta.','Otevři truhlu.'),
 ('mole','We found the mole.',['Small tunnels run beneath the vegetable patch.','The little animal has been digging under the soil.'],['Now the secret service has traced the leaked documents.','An enemy spy has been hiding inside our organisation.'],'Našli jsme krtka.','Odhalili jsme nastrčeného špiona.'),
 ('bow','Look at the bow.',['The archer made a weapon from flexible wood.','Its string is pulled tight, ready for an arrow.'],['Now walk to the front of the ship.','The forward section was damaged in the collision.'],'Podívej se na luk.','Podívej se na příď.'),
 ('light','Make it lighter.',['The backpack weighs too much to carry.','Take out some of the heavy equipment.'],['Now edit this dark photograph.','Increase the brightness so we can see the face.'],'Odlehči to.','Zesvětli to.'),
 ('mouse','Move the mouse.',['The small grey animal is hiding under the blanket.','Put it gently into its cage.'],['Now point at the icon on the computer screen.','The pointer is controlled by the device beside the keyboard.'],'Přesuň myš.','Přesuň myš.'),
 ('crane','Look at the crane.',['That tall bird is standing in the marsh.','It has a long neck and is searching for food.'],['Now look above the building site.','The tall machine is lifting a concrete slab.'],'Podívej se na jeřába.','Podívej se na jeřáb.'),
 ('alex-gender','Alex: I was ready.',['Alex is our sister and the lead medic.','She finished preparing her equipment before the alarm.'],['I got the names wrong: Alex is our brother.','He was the man on the radio, not our sister.'],'Alex: Byla jsem připravená.','Alex: Byl jsem připravený.'),
 ('ash-name','BRING ASH HERE.',['Ash is the guard standing at the gate.','Ask him to come over to our truck.'],['Now we need to clean out the old stove.','Bring the cold ash left from the fire.'],'Přiveď sem Ashe.','Přines sem popel.'),
 ('unchanged','Wait here.',['The lift will arrive in a moment.','I will fetch your bag while you stay by the doors.'],['The hallway ahead is unsafe.','Stay at this spot until I return.'],'Počkej tady.','Počkej tady.'),
]

CS=[
 ('zamek','Zkontroluj ten zámek.',['Dveře nejdou odemknout.','Klíč se zasekl v mechanismu.'],['Teď půjdeme na prohlídku šlechtického sídla.','Střecha nad velkým sálem je poškozená.'],'その錠を確認して。','その城を調べて。'),
 ('kolej','Najdi tu kolej.',['Studenti se dnes stěhují do ubytovny.','Jejich pokoje jsou v budově za školou.'],['Teď jsme na nádraží.','Potřebujeme zjistit, po které dráze přijede vlak.'],'その学生寮を見つけて。','その線路を見つけて。'),
 ('pero','Podej mi to pero.',['Musím podepsat smlouvu.','Psací potřeby leží na stole.'],['Teď zdobíme klobouk.','Použijeme dlouhé ptačí pírko z krabice.'],'そのペンを渡して。','その羽根を渡して。'),
 ('koruna','Ta koruna se ztratila.',['Král si odložil zlatý odznak své moci.','Někdo ho odnesl z trůnního sálu.'],['Teď počítám drobné mince.','Jedna česká mince spadla pod skříň.'],'その王冠がなくなった。','その一コルナ硬貨がなくなった。'),
 ('list','Podej mi ten list.',['Potřebuji prázdný papír do tiskárny.','Jeden volný arch leží vedle počítače.'],['Teď zkoumáme strom na zahradě.','Z větve spadl zelený kus rostliny.'],'その紙を渡して。','その葉を渡して。'),
]
JA=[
 ('sensei','先生に話して。',['手術の結果は先生が説明してくれる。','診察室でその先生を待っている。'],['授業が始まる。黒板の前に先生が立っている。','この学校で数学を教えている人だ。'],'Talk to the doctor.','Talk to the teacher.','Promluv s lékařem.','Promluv s učitelem.'),
 ('code','そのコードを見せて。',['プログラムが動かない。','画面にある命令文を調べよう。'],['ギターの音が出ない。','アンプとギターをつなぐ線を調べよう。'],'Show me that code.','Show me that cable.','Ukaž mi ten kód.','Ukaž mi ten kabel.'),
 ('kakete','もう一度かけて。',['電話が切れてしまった。','彼の番号にもう一度つながるか試そう。'],['この曲の最初の部分を聴き逃した。','同じ曲を最初から聴きたい。'],'Call again.','Play it again.','Zavolej znovu.','Pusť to znovu.'),
 ('one-more','もう一枚お願い。',['みんな並んで。写真の準備はいい？','カメラをこちらに向けて。'],['食器が一つ足りない。','棚から新しい皿を出して。'],'One more, please.','One more, please.','Ještě jednu fotku, prosím.','Ještě jeden talíř, prosím.'),
 ('tape','そのテープを持ってきて。',['破れた箱を閉じたい。','粘着テープでふたをとめよう。'],['昔のライブ録音を聴きたい。','カセットにその演奏が入っている。'],'Bring that tape.','Bring that tape.','Přines tu lepicí pásku.','Přines tu nahranou pásku.'),
]
DE=[
 ('bank','Ich warte bei der Bank.',['Ich muss Geld von meinem Konto abheben.','Die Filiale am Bahnhof ist noch geöffnet.'],['Jetzt sind wir im Park.','Neben dem Teich steht eine freie Sitzbank.'],'I am waiting by the bank.','I am waiting by the bench.'),
 ('schloss','Das Schloss ist beschädigt.',['Der Schlüssel steckt in der Tür fest.','Der Schließmechanismus lässt sich nicht mehr drehen.'],['Jetzt besichtigen wir den früheren Königssitz.','Das Dach über dem großen Saal ist eingestürzt.'],'The lock is damaged.','The castle is damaged.'),
 ('kiefer','Kiefer prüfen.',['Der Patient wurde ins Gesicht geschlagen.','Er kann den Mund nicht mehr richtig öffnen.'],['Jetzt untersuchen wir die Bäume im Wald.','Der Nadelbaum am Weg wurde vom Sturm getroffen.'],'Check the jaw.','Check the pine tree.'),
 ('leiter','Leiter finden.',['Die Arbeiter müssen auf das Dach steigen.','Eine tragbare Steighilfe fehlt.'],['Jetzt teilen wir das neue Team ein.','Für die Gruppe fehlt noch eine Führungsperson.'],'Find a ladder.','Find a leader.'),
 ('gericht','Ich kenne das Gericht.',['Der Koch serviert ein traditionelles Essen.','Ich habe diese Speise schon einmal probiert.'],['Jetzt beginnt der Prozess.','Ich war schon öfter in dieser Justizbehörde.'],'I know that dish.','I know that court.'),
]
ES=[
 ('banco','Te espero junto al banco.',['Necesito sacar dinero de mi cuenta.','La sucursal de la estación sigue abierta.'],['Ahora estamos paseando por el parque.','Hay un asiento de madera libre junto al estanque.'],'I will wait for you by the bank.','I will wait for you by the bench.'),
 ('gato','Trae el gato.',['El animal está dormido en el sofá.','Tenemos que llevarlo al veterinario.'],['Ahora vamos a cambiar la rueda del coche.','Necesitamos la herramienta que levanta el vehículo.'],'Bring the cat.','Bring the jack.'),
 ('cola','La cola es demasiado larga.',['Mucha gente espera para comprar entradas.','La fila llega hasta la calle siguiente.'],['Ahora mira al perro que está junto a la puerta.','Su rabo se arrastra por el suelo.'],'The queue is too long.','The tail is too long.'),
 ('vela','La vela está rota.',['Se ha ido la luz en toda la casa.','La pieza de cera se partió por la mitad.'],['Ahora estamos a bordo de un barco.','El viento desgarró la tela del mástil.'],'The candle is broken.','The sail is torn.'),
 ('planta','Hay que revisar la planta.',['Las hojas del rosal tienen manchas.','Quizá necesita agua o está enfermo.'],['Ahora inspeccionamos el edificio del hospital.','Hay una fuga de agua en el tercer piso.'],'We need to inspect the plant.','We need to inspect the floor.'),
]
FR=[
 ('avocat','Je cherche un avocat.',['La police veut me poser des questions.','Il me faut quelqu’un pour défendre mes droits.'],['Maintenant nous préparons une salade.','Il manque un fruit vert et mûr dans le panier.'],'I am looking for a lawyer.','I am looking for an avocado.'),
 ('vol','Le vol a été signalé.',['Le tableau a disparu du musée.','Quelqu’un l’a pris pendant la nuit.'],['Maintenant les contrôleurs surveillent le ciel.','Un avion inconnu vient d’apparaître sur le radar.'],'The theft has been reported.','The flight has been reported.'),
 ('piece','Je connais cette pièce.',['Nous visitons un appartement abandonné.','Je suis déjà entré dans cette chambre.'],['Maintenant les acteurs répètent au théâtre.','J’ai vu ce spectacle l’année dernière.'],'I know this room.','I know this play.'),
 ('manche','La manche est terminée.',['Les joueurs viennent de finir une partie.','Le score est définitif pour ce tour.'],['Maintenant la couturière achève la chemise.','La partie qui couvre le bras est enfin cousue.'],'The round is over.','The sleeve is finished.'),
 ('livre','Il manque deux livres.',['Nous rangeons les volumes de la bibliothèque.','Deux ouvrages ne sont pas revenus de prêt.'],['Maintenant nous pesons la farine pour une recette anglaise.','La balance indique deux unités de moins que le poids prévu.'],'Two books are missing.','It is two pounds short.'),
]

FILLER={
 'en':['Did you hear that?','It was probably the wind.','My boots are soaked.','AB 123 CD','We should keep moving.','CLOSE','Wait a moment.','I left my bag upstairs.'],
 'cs':['Slyšel jsi to?','To byl asi vítr.','Mám promočené boty.','AB 123 CD','Měli bychom pokračovat.','ZAVŘÍT','Počkej chvíli.','Tašku jsem nechal nahoře.'],
 'ja':['今の音、聞こえた？','たぶん風だよ。','靴がびしょ濡れだ。','AB 123 CD','先に進もう。','閉じる','ちょっと待って。','かばんを二階に置いてきた。'],
 'de':['Hast du das gehört?','Das war wohl der Wind.','Meine Stiefel sind nass.','AB 123 CD','Wir sollten weitergehen.','SCHLIESSEN','Warte einen Moment.','Meine Tasche liegt oben.'],
 'es':['¿Has oído eso?','Sería el viento.','Tengo las botas mojadas.','AB 123 CD','Deberíamos seguir.','CERRAR','Espera un momento.','Dejé mi bolsa arriba.'],
 'fr':['Tu as entendu ça ?','C’était sans doute le vent.','Mes bottes sont trempées.','AB 123 CD','On devrait avancer.','FERMER','Attends un instant.','J’ai laissé mon sac en haut.'],
}
# Natural examples, not mandatory reference strings. A neutral translation can
# legitimately fit both scenes, especially borrowed words and negative controls.
EN_REFS={
 'ja':{'bank':('川岸で会おう。','銀行のそばで会おう。'),'bat':('そのコウモリを近づけないで。','そのバットを近づけないで。'),'charge':('充電量を確認して。','料金を確認して。'),'seal':('パッキンが破損している。','封印が破損している。'),'pitch':('グラウンドを確認して。','音程を確認して。')},
 'de':{'bank':('Triff mich am Ufer.','Triff mich bei der Bank.'),'bat':('Halte diese Fledermaus von mir fern.','Halte diesen Schläger von mir fern.'),'charge':('Prüfe den Ladestand.','Prüfe die Gebühr.'),'seal':('Die Dichtung ist beschädigt.','Das Siegel ist beschädigt.'),'pitch':('Prüfe den Platz.','Prüfe die Tonhöhe.')},
 'es':{'bank':('Nos vemos junto a la orilla.','Nos vemos junto al banco.'),'bat':('Aparta ese murciélago de mí.','Aparta ese bate de mí.'),'charge':('Comprueba la carga de la batería.','Comprueba el cargo.'),'seal':('La junta está dañada.','El sello está dañado.'),'pitch':('Revisa el campo.','Comprueba el tono.')},
 'fr':{'bank':('Retrouve-moi près de la rive.','Retrouve-moi près de la banque.'),'bat':('Éloigne cette chauve-souris de moi.','Éloigne cette batte de moi.'),'charge':('Vérifie la charge de la batterie.','Vérifie les frais.'),'seal':('Le joint est endommagé.','Le sceau est endommagé.'),'pitch':('Vérifie le terrain.','Vérifie la hauteur de la note.')},
}

def make_case(group,src,tgt,row,refs=None):
 key,target,a,b,*rest=row;refa,refb=refs or rest[:2]
 return dict(id=f'{group}-{src}-{tgt}-{key}',group=group,source_language=src,target_language=tgt,feature=key,target=target,context_a=a,context_b=b,filler=FILLER[src],reference_a=refa,reference_b=refb,reference_policy='Evaluator-only examples, not exact-match answers. Accept natural alternatives and legitimately unchanged ambiguous wording.',negative_control=(key in ['mouse','unchanged','tape'] or (key=='one-more' and tgt=='en')),note=('Czech jeřáb denotes both bird and machine; inflection can differ. Do not demand a different noun.' if key=='crane' else 'Source context explicitly corrects a previous gender statement.' if key=='alex-gender' else 'Articleless mission-style text preserves the ambiguity; a gendered article would resolve it. The subtitle filter may reject this short instruction.' if src=='de' and key in ['kiefer','leiter'] else ''))

def timeline(case):
 rows=[];t=3.
 items=[('setup_a',x,3.) for x in case['context_a']]+[('target_a',case['target'],4.)]+[('filler',x,1.25) for x in case['filler']]+[('target_a_delayed',case['target'],4.)]+[('setup_b',x,3.) for x in case['context_b']]+[('target_b',case['target'],5.)]
 for phase,text,duration in items:
  rows.append(dict(index=len(rows),phase=phase,text=text,start=t,end=t+duration));t+=duration
 return dict(rows=rows,duration_s=t+3,proof_times=[r['end']-.5 for r in rows if r['phase'].startswith('target_')])

def main():
 p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 cases=[make_case('czech','en','cs',r) for r in EN]
 cases += [make_case('multi','en',lang,r,EN_REFS[lang][r[0]]) for lang in ['ja','de','es','fr'] for r in EN if r[0] in EN_REFS[lang]]
 cases += [make_case('multi','cs','ja',r) for r in CS]
 cases += [make_case('multi','ja',lang,r, r[4:6] if lang=='en' else r[6:8]) for lang in ['en','cs'] for r in JA]
 cases += [make_case('multi',lang,'en',r) for lang,rows in [('de',DE),('es',ES),('fr',FR)] for r in rows]
 assert len(cases)==75 and len({c['id'] for c in cases})==75
 for c in cases:c.update(timeline(c));assert len(c['rows'])==15 and len(c['proof_times'])==3
 dataset=dict(schema=1,scope='75 authored causal video scenes: 25 EN→CS and 50 multilingual across ten additional directions. Same line initially, after eight mixed filler observations, and after a topic correction. Original gameplay is background only, not the source of the authored story. All captions must enter through native video decoding and OCR. No reference or manually supplied context goes to the model.',cases=cases,comparison=['off','selected-v4'],providers=['openai-gpt-6-luna','custom-server-gemma4'],planned_runs=300,planned_target_render_opportunities=900,paid_judge_calls=0,selection='Purpose-built ambiguity and negative-control diagnostic; not representative game accuracy. Some semantic families recur across language directions.')
 raw=json.dumps(dataset,ensure_ascii=False,indent=2).encode();(a.output/'dataset.json').write_bytes(raw)
 (a.output/'dataset.sha256').write_text(hashlib.sha256(raw).hexdigest()+'\n');print('75 scenes;',collections.Counter((c['source_language'],c['target_language']) for c in cases))
if __name__=='__main__':main()

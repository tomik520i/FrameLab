"""Store explicitly supplied Codex judgements; never infer or call a judge."""
from pathlib import Path
import argparse,datetime,json
NOTES={
 'ok':('Přirozený překlad odpovídá významu této situace.','A natural translation fits this situation.'),
 'neutral':('Stejný přirozený výraz správně pokrývá oba kontexty; změna slova není nutná.','The same natural wording fits both contexts; a lexical change is not required.'),
 'wrong_sense':('Výstup volí význam, který neodpovídá aktuálnímu kontextu.','The output selects a meaning that does not fit the current context.'),
 'previous_line':('V tomto okamžiku je zobrazen překlad jiné, předchozí repliky.','At this instant, a different earlier line is displayed.'),
 'partial':('Význam je jen částečně zachovaný nebo zůstává nevhodně neurčitý.','Meaning is only partly preserved or remains inappropriately vague.'),
 'awkward':('Význam sedí, ale formulace je nepřirozená nebo stylisticky slabá.','The meaning fits, but the wording is awkward or stylistically weak.'),
 'grammar':('Význam lze pochopit, ale překlad má gramatickou chybu.','The meaning is understandable, but the translation has a grammatical error.'),
 'wrong_gender':('Výstup nezohledňuje rod výslovně určený v aktuálním kontextu.','The output does not follow the gender explicitly established in the current context.'),
 'untranslated':('Text nebyl použitelně přeložen do požadovaného jazyka.','The text was not usefully translated into the requested language.'),
 'added':('Překlad přidává význam, který současná replika ani kontext neopravňuje.','The translation adds meaning unsupported by the current line and context.'),
 'wrong_action':('Klíčový význam slova sedí, ale věta mění požadovaný děj nebo vztah.','The key word fits, but the sentence changes the requested action or relationship.'),
}
def main():
 p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--grades',required=True,help='Explicit comma-separated id:meaning:language:note entries');a=p.parse_args();root=a.experiment
 candidates={r['id']:r for r in json.loads((root/'runs-review.json').read_text(encoding='utf-8'))};path=root/'grades.json';grades=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
 for item in a.grades.split(','):
  rid,m,l,note=item.split(':');m=int(m);l=int(l);assert rid in candidates and m in range(3) and l in range(3) and note in NOTES
  new=dict(meaning=m,language=l,note=note,note_cs=NOTES[note][0],note_en=NOTES[note][1],reviewer='Codex direct review; provider/mode labels withheld in candidate file',reviewed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
  if rid in grades:assert all(grades[rid][k]==new[k] for k in ['meaning','language','note']),'Do not silently overwrite a frozen judgement'
  else:grades[rid]=new
 path.write_text(json.dumps(grades,ensure_ascii=False,indent=2),encoding='utf-8');print(len(grades),'explicit judgements stored')
if __name__=='__main__':main()

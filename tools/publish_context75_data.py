"""Assemble factual CS/EN report inputs. Does not make judgements or deploy."""
from pathlib import Path
import argparse,collections,datetime,hashlib,json,shutil,zipfile
from analyze_context75 import stats

def read(p):return json.loads(p.read_text(encoding='utf-8'))
def write(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('experiment',type=Path);p.add_argument('--website',type=Path,required=True);p.add_argument('--evidence',action='store_true');p.add_argument('--sources',action='store_true');a=p.parse_args();assert not a.sources or a.evidence;root=a.experiment;web=a.website;web.mkdir(parents=True,exist_ok=True)
 dataset=read(root/'dataset.json');cases={c['id']:c for c in dataset['cases']};analysis=read(root/'runs-analysis.json');grades=read(root/'grades.json') if (root/'grades.json').exists() else {};runs=analysis['runs'];by_case=collections.defaultdict(list);groups=collections.defaultdict(list)
 fallback=read(root/'memory-fallback-audit.json') if (root/'memory-fallback-audit.json').exists() else None
 display_audit=read(root/'display-audit.json') if (root/'display-audit.json').exists() else None
 for r in runs:
  if fallback and r['run']==fallback['run']:r['memory']['fallback_audit']=fallback
  if display_audit and r['run']==display_audit['run']:r['display_audit']=display_audit
  proof_index={x['index']:x for x in read(root/'runs'/r['run']/'proof-index.json')}
  by_case[r['case']].append(r)
  for t in r['targets']:
   t['first_grade']=grades.get(t['first_review_id']);t['proof_grade']=grades.get(t['proof_review_id'])
   t['proof_dimensions']={k:proof_index[t['proof']['proof_index']][k] for k in ['width','height']} if t['proof'] else None
  groups[r['provider']+'-'+r['mode']].append(r)
 def aggregate(rs):
  targets=[t for r in rs for t in r['targets']];proof_present=[t for t in targets if t['proof_review_id']];reviewed=[t for t in proof_present if t['proof_grade']];displayed=[t for t in targets if t['first'] and t['first']['display_video_s'] is not None]
  correct=lambda t:bool(t['proof'] and t['proof']['present_success'] and t['start']<=t['proof']['video_s']<t['end'] and t['proof_grade'] and t['proof_grade']['meaning']==2 and t['proof_grade']['language']>=1)
  good=[t for t in targets if correct(t)];timely=[t for t in displayed if t['first']['displayed_in_window'] and t['first_grade'] and t['first_grade']['meaning']==2 and t['first_grade']['language']>=1]
  nuisance=[row for r in rs for row in r['rows'] if row['phase']=='filler' and row['text'] in ['AB 123 CD','CLOSE','ZAVŘÍT','閉じる','SCHLIESSEN','CERRAR','FERMER']]
  return dict(runs=len(rs),targets=len(targets),reviewed_proofs=len(reviewed),nonempty_proofs=len(proof_present),correct_proofs=len(good),correct_on_time=len(timely),proof_meaning_points=sum(t['proof_grade']['meaning'] for t in reviewed),max_proof_meaning_points=len(targets)*2,target_displayed=len(displayed),target_missing=len(targets)-len(displayed),target_late=sum(not t['first']['displayed_in_window'] for t in displayed),cached_targets=sum(bool(t['first'] and t['first']['cache_hit']) for t in targets),target_ocr_exact=sum(t['ocr_exact_observations']>0 for t in targets),nuisance_intervals=len(nuisance),nuisance_ocr_seen=sum(any(row['ocr_sources']) for row in nuisance),nuisance_displayed=sum(any(x['display_video_s'] is not None for x in row['attempts']) for row in nuisance),memory_error_runs=sum(bool(r['memory']['errors']) for r in rs),memory_updates=sum(r['memory']['updates'] for r in rs),native_translation_errors=sum(r['event_counts'].get('translation_error',0) for r in rs),cost_usd=sum(r['api_cost_estimate_usd'] for r in rs),capture_to_display_ms=stats(t['first']['capture_to_display_ms'] for t in displayed),source_onset_to_display_ms=stats(t['first']['source_onset_to_display_ms'] for t in displayed),target_uncached_translation_ms=stats(t['first']['translation_ms'] for t in targets if t['first'] and not t['first']['cache_hit']),memory_update_median_ms=stats(r['latency']['memory_ms']['p50'] for r in rs),output_run_mean_fps=stats(r['metrics']['output_submitted_fps']['mean'] for r in rs),gpu_run_mean_percent=stats(r['metrics']['gpu_percent']['mean'] for r in rs),vram_run_mean_mb=stats(r['metrics']['vram_used_mb']['mean'] for r in rs),cpu_run_mean_percent=stats(r['metrics']['cpu_system_percent']['mean'] for r in rs),proof_readback_run_mean_ms=stats(r['latency']['proof_readback_ms']['mean'] for r in rs),phases={phase:dict(total=sum(t['phase']==phase for t in targets),correct=sum(t['phase']==phase and correct(t) for t in targets)) for phase in ['target_a','target_a_delayed','target_b']})
 index=[];pairs=[];scene_map=read(root/'scene-map.json');glossary=read(web/'scenario-glossary.json')
 for cid,c in cases.items():
  rs=by_case[cid];video='sources/'+scene_map[cid]+'.mp4';detail=dict(case=c,runs=rs,gloss=glossary[c['source_language']+'-'+c['feature']],source_video=video if a.sources or (web/video).exists() else None);write(web/'cases'/(cid+'.json'),detail)
  row=dict(id=cid,group=c['group'],source_language=c['source_language'],target_language=c['target_language'],feature=c['feature'],target=c['target'],negative_control=c['negative_control'],runs=len(rs),reviewed=all(t['proof_review_id'] is None or t['proof_grade'] for r in rs for t in r['targets']) and len(rs)==4,comparisons=[])
  for provider in ['openai','custom']:
   off=next((r for r in rs if r['provider']==provider and r['mode']=='off'),None);on=next((r for r in rs if r['provider']==provider and r['mode']=='selected'),None)
   if not(off and on):continue
   if any(t['proof_review_id'] and not t['proof_grade'] for r in [off,on] for t in r['targets']):continue
   score=lambda r:sum(t['proof_grade']['meaning'] if t['proof_grade'] else 0 for t in r['targets'])
   d=dict(case=cid,provider=provider,off=score(off),selected=score(on),difference=score(on)-score(off));row['comparisons'].append(d);pairs.append(d)
  index.append(row)
 current=dict(schema=1,created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),planned_cases=75,planned_runs=300,planned_targets=900,completed_runs=len(runs),complete_cases=sum(len(v)==4 for v in by_case.values()),reviewed_cases=sum(c['reviewed'] for c in index),unique_reviewed_outputs=len(grades),judging='Codex direct qualitative review; provider and mode labels omitted from candidate file. No external evaluator model, no paid judge.',dataset_sha256=digest(root/'dataset.json'),cases=index,arms={k:aggregate(v) for k,v in groups.items()},paired={p:dict(n=sum(x['provider']==p for x in pairs),better=sum(x['provider']==p and x['difference']>0 for x in pairs),same=sum(x['provider']==p and x['difference']==0 for x in pairs),worse=sum(x['provider']==p and x['difference']<0 for x in pairs)) for p in ['openai','custom']},language_arms={pair:{arm:aggregate([r for r in rs if cases[r['case']]['source_language']+'-'+cases[r['case']]['target_language']==pair]) for arm,rs in groups.items()} for pair in dict.fromkeys(c['source_language']+'-'+c['target_language'] for c in cases.values())},all_complete=len(runs)==300 and all(c['reviewed'] for c in index),total_cost_usd=sum(r['api_cost_estimate_usd'] for r in runs),paid_judge_calls=0,activation_failures=[r['run'] for r in runs if not(r['activation']['nr_completed']>0 and r['activation']['generated_fps_max']>0 and r['activation']['proofs']==3 and r['activation']['present_successes']==3)])
 if (root/'findings.json').exists():current['findings']=read(root/'findings.json')
 for extra in ['model-config.json','auxiliary-activity.json','review-decisions.json','diagnostics.json','evidence-validation.json','final-integrity.json','token-usage.json','memory-fallback-audit.json','upload-overlap-audit.json','ocr-environment.json','display-audit.json']:
  if (root/extra).exists():shutil.copy2(root/extra,web/extra)
 write(web/'report.json',current);write(web/'dataset.json',dataset);write(web/'grades.json',grades)
 if (root/'runs-manifest.json').exists():shutil.copy2(root/'runs-manifest.json',web/'runtime-manifest.json')
 if a.evidence:
  files=[]
  for r in runs:
   source=root/'runs'/r['run'];dest=web/'data'/r['run'];dest.mkdir(parents=True,exist_ok=True)
   for name in ['events.jsonl','worker.jsonl','render.jsonl','metrics.csv','frames.csv','summary.json','proof-index.json','api-usage.jsonl']:
    src=source/name
    if src.exists():
     dst=dest/name
     if not dst.exists() or digest(src)!=digest(dst):shutil.copy2(src,dst)
     files.append(dict(path=dst.relative_to(web).as_posix(),bytes=dst.stat().st_size,sha256=digest(dst)))
   (dest/'proofs').mkdir(exist_ok=True)
   for src in (source/'proofs').glob('*.png'):
    dst=dest/'proofs'/src.name
    if not dst.exists() or digest(src)!=digest(dst):shutil.copy2(src,dst)
    files.append(dict(path=dst.relative_to(web).as_posix(),bytes=dst.stat().st_size,sha256=digest(dst)))
  write(web/'evidence-manifest.json',files)
  if a.sources:
   dest=web/'sources';dest.mkdir(exist_ok=True)
   for key in sorted(set(scene_map.values())):
    for ext in ['.mp4','.ass','.json']:
     src=root/'scenes'/(key+ext);dst=dest/src.name
     if not dst.exists():shutil.copy2(src,dst)
     assert digest(src)==digest(dst)
     files.append(dict(path=dst.relative_to(web).as_posix(),bytes=dst.stat().st_size,sha256=digest(dst)))
   provenance=Path(__file__).resolve().parents[3]/'outputs/FrameLab-PC-2026-11-23/app/samples/gta.source.json'
   if provenance.exists():shutil.copy2(provenance,dest/'background-provenance.json')
   write(web/'evidence-manifest.json',files)
  # Original model/experiment sources, not executable/runtime/DLL distribution.
  archive=web/'reproduction.zip'
  with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
   repo=Path(__file__).resolve().parents[1]
   z.write(repo/'LICENSE','LICENSE')
   baseline=root/'settings-before-run/subtitle-settings.txt'
   if baseline.exists():z.write(baseline,'baseline-subtitle-settings-v12.txt')
   z.writestr('README.txt','FrameLab context75 evaluation evidence\n\nThis archive contains experiment and analysis source code, not an installation package. The main application baseline commit and exact measured binary/Python hashes are in runs-manifest.json; frozen/main.cpp includes the framebuffer proof hook. Source code is under the included GPLv3 license. Model weights, API credentials, proprietary runtimes and the executable are not redistributed here.\n\nUse the normal FrameLab build instructions. The frozen runner intentionally requires the archived v12 subtitle-settings baseline; preserve your own settings before copying it. Select NVIDIA VFG 2x and the same device/settings recorded in native events. Supply your own API credential through FrameLab and your own LAN model configuration. The selected model must build its own memory. Never substitute reference answers into OCR or requests.\n\nExact source MP4 files, ASS text and SHA-256 metadata are available in the report sources/ directory, indexed by scene-map.json. Gameplay is a short repeated project-owner recording; game content retains its original rights and is not relicensed as GPL. See sources/background-provenance.json on the report website.\n\nReproduction commands and parameter names are documented by --help in build_context75_dataset.py, render_context75_videos.py, run_context75.py, analyze_context75.py, pending_context75_review.py and publish_context75_data.py. The runner uses normal native video decoding, OCR, filtering, selected translation provider and caption rendering. It does not replay prepared translations.\n\nGrades are direct Codex qualitative assessments, not exact-match rules or independent native-speaker consensus. No paid model judge was called. Preserve failed and missing results when repeating the experiment. A new experiment must use a new output directory.\n')
   for f in (root/'frozen').iterdir():
    if f.suffix in ['.py','.cpp','.json']:z.write(f,'frozen/'+f.name)
   for f in Path(__file__).resolve().parent.glob('*context75*.py'):z.write(f,'analysis-tools/'+f.name)
   for name in ['dataset.json','dataset.sha256','scene-map.json','runs-plan.json','runs-manifest.json','grades.json','model-config.json','auxiliary-activity.json','review-decisions.json','diagnostics.json','evidence-validation.json','final-integrity.json','token-usage.json','memory-fallback-audit.json','upload-overlap-audit.json','ocr-environment.json','display-audit.json','findings.json']:
    if (root/name).exists():z.write(root/name,name)
   for key in sorted(set(scene_map.values())):
    for ext in ['.ass','.json']:
     f=root/'scenes'/(key+ext);z.write(f,'scenes/'+f.name)
  current['reproduction_sha256']=digest(archive);write(web/'report.json',current)
 print(json.dumps({k:current[k] for k in ['completed_runs','complete_cases','reviewed_cases','unique_reviewed_outputs','all_complete','total_cost_usd']}))
if __name__=='__main__':main()

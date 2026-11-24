"""Bounded experimental calls to the selected model; never a second summarizer."""
import json,time
from story_memory import task_prompt
from translation_memory import local_prompt

class Backend:
    def __init__(self,model,runtime=None,server_config=None,key_file=None):
        self.model=model;self.engine=None;self.server=None;self.api=model=='gpt-6-luna';self.cost=0.;self.reserve=0.
        if self.api:self.key=key_file.read_text(encoding='utf-8-sig').strip()
        elif server_config:
            from custom_ai_translation import CustomTranslator
            self.server=CustomTranslator(json.loads(server_config.read_text(encoding='utf-8')))
        else:
            from subtitle_gpu import GpuTranslator
            self.engine=GpuTranslator(runtime,model,compute='cuda',allow_offload=model=='translategemma-12b-q4km')

    def generate(self,*,source='',context=None,instruction=None,data=None):
        from openai_translation import request,build_request,parse_response
        summary=instruction is not None;prompt=None;usage={};metrics={};text='';error=None;start=time.perf_counter();worst=.002
        try:
            if self.api:
                if self.cost+self.reserve+worst>.10:raise RuntimeError('API cap $0.10 reached')
                prompt=dict(model=self.model,store=False,reasoning=dict(effort='none'),temperature=0,max_output_tokens=256,input=[dict(role='developer',content=instruction),dict(role='user',content=json.dumps(data,ensure_ascii=False))]) if summary else build_request(self.model,'en','cs',source,effort='none',temperature=0,filtering=False,context=context)
                response=request('responses',self.key,prompt);usage=response.get('usage',{});details=usage.get('input_tokens_details',{});cached=details.get('cached_tokens',0);writes=details.get('cache_creation_tokens',details.get('cache_write_tokens',0))
                self.cost+=(max(0,usage.get('input_tokens',0)-cached-writes)*.10+cached*.01+writes*.125+usage.get('output_tokens',0)*.50)/1e6
                metrics=dict(model=response.get('model'),status=response.get('status'))
                if response.get('status')!='completed':raise ValueError('Incomplete response')
                text=''.join(x.get('text','') for item in response.get('output',[]) if item.get('type')=='message' for x in item.get('content',[]) if x.get('type')=='output_text').strip() if summary else parse_response(response)[0]['text']
            elif self.server:
                from custom_ai_translation import request as send,parse_response as parse
                if summary:
                    c=self.server.config;prompt=dict(c['parameters'],model=c['model'],stream=False,temperature=0,max_tokens=256,messages=[dict(role='system',content=instruction),dict(role='user',content=json.dumps(data,ensure_ascii=False))]);response=send(c['url'],'/chat/completions',key=self.server.key,payload=prompt,timeout=90);usage=response.get('usage',{});metrics=response.get('timings',{});text=parse(response)
                else:
                    text=self.server.translate(source,context=context);prompt=self.server.last_payload;usage=self.server.last_usage;metrics=self.server.last_metrics
            else:
                prompt=task_prompt(self.model,instruction,data) if summary else local_prompt(self.model,source,context)
                if not summary and context and 'story_summary_unverified' in context and not self.engine.gemma:
                    from story_memory import translation_prompt
                    prompt=translation_prompt(self.model,source,context)
                options=dict(self.engine.options(),num_predict=256 if summary else 384,top_k=1,top_p=1.,stop=['<end_of_turn>','<eos>','###'])
                response=self.engine.api('/api/generate',dict(model=self.engine.name,prompt=prompt,raw=True,stream=False,keep_alive=-1,options=options),timeout=180)
                text=response.get('response','').strip();metrics={k:response.get(k) for k in ('done_reason','prompt_eval_count','eval_count','prompt_eval_duration','eval_duration')}
                if response.get('done_reason')=='length':error='Output hit token limit'
            if not text:error=error or 'Empty output'
        except Exception as exc:
            error=str(exc)
            if self.api and not usage:self.reserve+=worst
        return dict(text=text,ms=(time.perf_counter()-start)*1000,prompt=prompt,usage=usage,metrics=metrics,error=error)

    def close(self):
        if self.engine:self.engine.close()

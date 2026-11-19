import argparse,datetime,json,sys,time
from pathlib import Path
R=Path(__file__).resolve().parent;sys.path.insert(0,str(Path('H:/FrameLab-work/workspace/work/gpu-translation-eval/pydeps')))
import pynvml as nv
P=argparse.ArgumentParser();P.add_argument('phase');P.add_argument('--seconds',type=float,default=30);P.add_argument('--out',type=Path,required=True);P.add_argument('--app-log',type=Path,required=True);A=P.parse_args()
LOG=A.app_log
nv.nvmlInit();dev=nv.nvmlDeviceGetHandleByIndex(0);start=time.monotonic();count=0
with A.out.open('w',encoding='utf-8') as f:
    while time.monotonic()-start<A.seconds:
        tick=time.monotonic();mem=nv.nvmlDeviceGetMemoryInfo(dev,version=nv.nvmlMemory_v2)
        gpu={'memory.total':mem.total/1048576,'memory.used':mem.used/1048576,'memory.free':mem.free/1048576,'memory.reserved':mem.reserved/1048576,'utilization.gpu':nv.nvmlDeviceGetUtilizationRates(dev).gpu,'power.draw':nv.nvmlDeviceGetPowerUsage(dev)/1000,'temperature.gpu':nv.nvmlDeviceGetTemperature(dev,nv.NVML_TEMPERATURE_GPU),'clocks.gr':nv.nvmlDeviceGetClockInfo(dev,nv.NVML_CLOCK_GRAPHICS),'clocks.mem':nv.nvmlDeviceGetClockInfo(dev,nv.NVML_CLOCK_MEM)}
        assert time.time()-LOG.stat().st_mtime<3, 'Stale application log'
        lines=LOG.read_text(encoding='utf-8').splitlines();keys=lines[0].split(',');app=None
        for line in reversed(lines[1:]):
            values=line.split(',')
            if len(values)==len(keys):app=dict(zip(keys,map(float,values)));break
        row=dict(phase=A.phase,utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),elapsed_s=time.monotonic()-start,gpu=gpu,app=app,log_mtime_age_s=time.time()-LOG.stat().st_mtime)
        f.write(json.dumps(row)+'\n');f.flush();count+=1
        time.sleep(max(0,1-(time.monotonic()-tick)))
nv.nvmlShutdown();print(json.dumps(dict(phase=A.phase,samples=count)),flush=True)

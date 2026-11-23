"""Translation-only device selection. Never changes machine-wide environment.

CUDA devices are identified by NVML UUID; Vulkan devices by VkPhysicalDeviceIDProperties
deviceUUID, resolved to the current Vulkan enumeration index at each launch.
"""
from pathlib import Path
import ctypes as C
import os
import re

BACKENDS=('auto','cuda','vulkan','cpu')
MASKS=('CUDA_VISIBLE_DEVICES','HIP_VISIBLE_DEVICES','ROCR_VISIBLE_DEVICES','GPU_DEVICE_ORDINAL',
       'GGML_VK_VISIBLE_DEVICES','OLLAMA_LLM_LIBRARY','GGML_BACKEND_PATH','OLLAMA_VULKAN','OLLAMA_IGPU_ENABLE')

def cuda_devices():
    lib=None;initialized=False
    try:
        lib=C.WinDLL(str(Path(os.environ['SystemRoot'])/'System32/nvml.dll'))
        if lib.nvmlInit_v2()!=0:return []
        initialized=True;count=C.c_uint();rows=[]
        if lib.nvmlDeviceGetCount_v2(C.byref(count))!=0:return []
        class Memory(C.Structure):_fields_=[('total',C.c_ulonglong),('free',C.c_ulonglong),('used',C.c_ulonglong)]
        for index in range(count.value):
            handle=C.c_void_p();name=C.create_string_buffer(128);uuid=C.create_string_buffer(96);memory=Memory()
            if lib.nvmlDeviceGetHandleByIndex_v2(index,C.byref(handle))!=0:continue
            if lib.nvmlDeviceGetName(handle,name,len(name))!=0 or lib.nvmlDeviceGetUUID(handle,uuid,len(uuid))!=0:continue
            ok=lib.nvmlDeviceGetMemoryInfo(handle,C.byref(memory))==0
            rows.append(dict(backend='cuda',id=uuid.value.decode('ascii'),index=index,name=name.value.decode('utf-8','replace'),integrated=False,total=memory.total if ok else None,free=memory.free if ok else None))
        return rows
    except (AttributeError,OSError,KeyError):return []
    finally:
        if initialized:lib.nvmlShutdown()

def vulkan_devices():
    class App(C.Structure):_fields_=[('sType',C.c_uint32),('pNext',C.c_void_p),('name',C.c_char_p),('version',C.c_uint32),('engine',C.c_char_p),('engineVersion',C.c_uint32),('apiVersion',C.c_uint32)]
    class Create(C.Structure):_fields_=[('sType',C.c_uint32),('pNext',C.c_void_p),('flags',C.c_uint32),('app',C.POINTER(App)),('layerCount',C.c_uint32),('layers',C.c_void_p),('extensionCount',C.c_uint32),('extensions',C.c_void_p)]
    class ID(C.Structure):_fields_=[('sType',C.c_uint32),('pNext',C.c_void_p),('deviceUUID',C.c_ubyte*16),('driverUUID',C.c_ubyte*16),('luid',C.c_ubyte*8),('nodeMask',C.c_uint32),('luidValid',C.c_uint32)]
    class Props(C.Structure):_fields_=[('sType',C.c_uint32),('pNext',C.c_void_p),('properties',C.c_ubyte*4096)]
    instance=C.c_void_p();lib=None
    try:
        lib=C.WinDLL(str(Path(os.environ['SystemRoot'])/'System32/vulkan-1.dll'))
        lib.vkCreateInstance.argtypes=[C.POINTER(Create),C.c_void_p,C.POINTER(C.c_void_p)];lib.vkCreateInstance.restype=C.c_int32
        lib.vkDestroyInstance.argtypes=[C.c_void_p,C.c_void_p]
        lib.vkEnumeratePhysicalDevices.argtypes=[C.c_void_p,C.POINTER(C.c_uint32),C.c_void_p];lib.vkEnumeratePhysicalDevices.restype=C.c_int32
        lib.vkGetPhysicalDeviceProperties2.argtypes=[C.c_void_p,C.POINTER(Props)]
        app=App(sType=0,name=b'FrameLab device discovery',apiVersion=(1<<22)|(1<<12));create=Create(sType=1,app=C.pointer(app))
        if lib.vkCreateInstance(C.byref(create),None,C.byref(instance))!=0:return []
        count=C.c_uint32()
        if lib.vkEnumeratePhysicalDevices(instance,C.byref(count),None)!=0 or count.value>64:return []
        handles=(C.c_void_p*count.value)()
        if lib.vkEnumeratePhysicalDevices(instance,C.byref(count),handles)!=0:return []
        rows=[]
        for index,handle in enumerate(handles):
            ident=ID(sType=1000071004);props=Props(sType=1000059001,pNext=C.cast(C.pointer(ident),C.c_void_p))
            lib.vkGetPhysicalDeviceProperties2(handle,C.byref(props));raw=bytes(props.properties)
            kind=int.from_bytes(raw[16:20],'little')
            if kind not in (1,2,3):continue # no CPU software Vulkan adapters
            device_id=bytes(ident.deviceUUID).hex()
            if device_id=='0'*32:continue # don't persist an unstable enumeration index
            rows.append(dict(backend='vulkan',id=device_id,index=index,name=raw[20:276].split(b'\0')[0].decode('utf-8','replace'),integrated=kind==1,total=None,free=None))
        return rows
    except (AttributeError,OSError,KeyError):return []
    finally:
        if lib and instance.value:lib.vkDestroyInstance(instance,None)

def discover():return cuda_devices()+vulkan_devices()

def choose(backend,device_id='',devices=None):
    if backend not in BACKENDS:raise ValueError('Unknown translation compute backend')
    devices=discover() if devices is None else devices
    if backend=='cpu':return dict(backend='cpu',id='',name='CPU / system RAM',integrated=False,free=None,total=None)
    if device_id:
        found=[d for d in devices if d['id']==device_id and d['backend']==backend]
        if not found:raise RuntimeError('Selected translation GPU is missing. Refresh devices and select a GPU again.')
        return found[0]
    candidates=[d for d in devices if backend=='auto' or d['backend']==backend]
    if not candidates:
        if backend=='auto':return choose('cpu',devices=devices)
        raise RuntimeError('No '+backend.upper()+' GPU detected. Install a current GPU driver or select CPU.')
    # Prefer a discrete GPU; NVIDIA uses CUDA, other vendors use Vulkan.
    return sorted(candidates,key=lambda d:(d['integrated'],d['backend']!='cuda',-(d.get('total') or 0)))[0]

def runtime_environment(environment,device):
    env=dict(environment)
    for name in MASKS:env.pop(name,None)
    env.update(CUDA_VISIBLE_DEVICES='-1',HIP_VISIBLE_DEVICES='-1',ROCR_VISIBLE_DEVICES='-1',GPU_DEVICE_ORDINAL='-1',GGML_VK_VISIBLE_DEVICES='-1',OLLAMA_VULKAN='0',OLLAMA_IGPU_ENABLE='0')
    if device['backend']=='cuda':env['CUDA_VISIBLE_DEVICES']=device['id']
    elif device['backend']=='vulkan':env.update(OLLAMA_VULKAN='1',GGML_VK_VISIBLE_DEVICES=str(device['index']),OLLAMA_IGPU_ENABLE='1')
    return env

def label(device):return 'CPU / system RAM' if device['backend']=='cpu' else device['backend'].upper()+' / '+device['name']

if __name__=='__main__':
    import json
    print(json.dumps(discover(),ensure_ascii=False,indent=2))

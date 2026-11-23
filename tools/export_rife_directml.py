"""Reproducible RIFE 4.22 lite export; Python is a build dependency only.

Weights/model: HolyWu/vs-rife, MIT. Streaming graph organization adapted from
smoovplay/smoovplay framegen/fastrife.py, MIT (see third_party/rife422/NOTICE).
FP16 convolutions, FP32 sampling grids. Inputs/outputs are padded uint8 BGRA.
Dynamic dimensions are specialized by ONNX Runtime at session creation.
"""
from pathlib import Path
import argparse, hashlib, json, sys
import torch
import torch.nn.functional as F
import onnx

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'third_party'))
from rife422.IFNet_HDv3_v4_22_lite import IFNet

WEIGHTS_SHA256='8f6fb9105ba9e946762ee7190acbca3ca1cf14193eb81ca0955d492fb8558692'
MODEL_SOURCE_COMMIT='3488617283db7c428a83ba4a19382285da698b6a'
def resize(x,scale):
    return x if scale==1 else F.interpolate(x,scale_factor=scale,mode='bilinear',align_corners=False)

class Graph(torch.nn.Module):
    def __init__(self,net):
        super().__init__()
        self.encode=net.encode
        self.blocks=torch.nn.ModuleList([net.block0,net.block1,net.block2,net.block3])
        self.scales=net.scale_list
        self.register_buffer('rgb',torch.tensor([2,1,0]))

    def image(self,x):
        return x.index_select(2,self.rgb).permute(2,0,1).unsqueeze(0).half()/255.0

    def forward(self,frame0,frame1,t):
        a,b=self.image(frame0),self.image(frame1)
        src0=torch.cat([a,self.encode(a)],1).float()
        src1=torch.cat([b,self.encode(b)],1).float()
        h,w=src0.shape[2],src0.shape[3]
        gx=(torch.arange(w,dtype=torch.float32)/(w-1)*2-1).view(1,1,1,-1)
        gy=(torch.arange(h,dtype=torch.float32)/(h-1)*2-1).view(1,1,-1,1)
        def warp(src,flow):
            f=flow.float()
            grid=torch.cat([f[:,0:1]*(2.0/(w-1))+gx,f[:,1:2]*(2.0/(h-1))+gy],1).permute(0,2,3,1)
            return F.grid_sample(src,grid,mode='bilinear',padding_mode='border',align_corners=True)
        w0,w1=src0,src1
        flow=mask=feat=None
        for i,(blk,s) in enumerate(zip(self.blocks,self.scales)):
            a,b=resize(w0,1/s).half(),resize(w1,1/s).half()
            ts=t.half().expand_as(a[:,:1])
            parts=[a[:,:3],b[:,:3],a[:,3:],b[:,3:],ts]
            if flow is not None:
                parts += [resize(mask,1/s),resize(feat,1/s),resize(flow,1/s)/s]
            x=torch.cat(parts,1)
            tmp=resize(blk.lastconv(blk.convblock(blk.conv0(x))),s)
            fd=tmp[:,:4]*s
            flow=fd if flow is None else flow+fd
            mask,feat=tmp[:,4:5],tmp[:,5:]
            w0=warp(src0[:,:3] if i==3 else src0,flow[:,:2])
            w1=warp(src1[:,:3] if i==3 else src1,flow[:,2:4])
        m=torch.sigmoid(mask.float())
        result=(w0*m+w1*(1-m))[0].index_select(0,self.rgb).permute(1,2,0)
        rgb=result.clamp(0,1).mul(255).round().to(torch.uint8)
        return torch.cat([rgb,torch.full_like(rgb[:,:,:1],255)],2)

def main():
    p=argparse.ArgumentParser();p.add_argument('weights',type=Path);p.add_argument('output',type=Path);args=p.parse_args()
    digest=hashlib.sha256(args.weights.read_bytes()).hexdigest()
    if digest!=WEIGHTS_SHA256:raise ValueError('Unexpected RIFE weights digest')
    torch.set_num_threads(8);torch.manual_seed(422)
    state=torch.load(args.weights,map_location='cpu',weights_only=True)
    state={k.removeprefix('module.'):v for k,v in state.items()}
    # The training checkpoint also carries teacher/timestep-estimator heads,
    # which are not part of the published inference network.
    state={k:v for k,v in state.items() if not k.startswith(('teacher.','caltime.'))}
    args.output.mkdir(parents=True,exist_ok=True);results=[]
    for name,scale in [('low',.5),('medium',1.),('high',2.)]:
        net=IFNet(scale=scale).eval().half();net.load_state_dict(state,strict=True)
        graph=Graph(net).eval()
        a=torch.randint(0,256,(128,256,4),dtype=torch.uint8);b=torch.roll(a,8,dims=1);t=torch.tensor([[[[.5]]]])
        with torch.inference_mode():
            actual=graph(a,b,t)
            im0,im1=graph.image(a),graph.image(b)
            h,w=im0.shape[2:];grid=torch.cat([torch.linspace(-1,1,w).view(1,1,1,w).expand(1,1,h,w),torch.linspace(-1,1,h).view(1,1,h,1).expand(1,1,h,w)],1)
            ref=net(im0,im1,t.half().expand(1,1,h,w),[(w-1)/2,(h-1)/2],grid,net.encode(im0),net.encode(im1))
            ref=ref.float()[0].index_select(0,graph.rgb).permute(1,2,0).clamp(0,1)*255
            mae=(actual[:,:,:3].float()-ref).abs().mean().item()
            if mae>2.:raise RuntimeError(f'{name}: export graph drift {mae}')
            path=args.output/f'rife422-lite-{name}.onnx'
            torch.onnx.export(graph,(a,b,t),str(path),opset_version=17,dynamo=False,
                input_names=['frame0','frame1','t'],output_names=['frame'],
                dynamic_axes={'frame0':{0:'height',1:'width'},'frame1':{0:'height',1:'width'},'frame':{0:'height',1:'width'}},
                do_constant_folding=True)
            onnx.checker.check_model(str(path))
            results.append({'file':path.name,'scale':scale,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'reference_mae_255':mae})
            print(json.dumps(results[-1]),flush=True)
    (args.output/'model-provenance.json').write_text(json.dumps({'model':'RIFE 4.22 lite','definition_commit':MODEL_SOURCE_COMMIT,'weights_url':'https://github.com/HolyWu/vs-rife/releases/download/model/flownet_v4.22.lite.pkl','weights_sha256':digest,'torch':torch.__version__,'onnx':onnx.__version__,'opset':17,'precision':'FP16 convolutions / FP32 warps','outputs':results},indent=2),encoding='utf-8')

if __name__=='__main__':main()

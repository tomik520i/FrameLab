#pragma once
// Same-resolution SDR enhancement. Preserve full-resolution source detail by
// upsampling only the neural residual when inference runs below source size.
inline constexpr char nrShaders[] = R"(
cbuffer Settings : register(b0) { uint W,H,SW,SH,Reset,UseMotion,Pad0,Pad1; };
Texture2D<float4> Source : register(t0);
Texture2D<float4> Proxy : register(t1);
Texture2D<float4> Previous : register(t2);
Texture2D<float2> SmallFlow : register(t3);
Texture2D<float4> Neural : register(t4);
RWTexture2D<float4> ProxyOut : register(u0);
RWTexture2D<float2> FlowOut : register(u1);
RWTexture2D<float2> MotionOut : register(u2);
RWTexture2D<float> DepthOut : register(u3);
RWTexture2D<float4> FinalOut : register(u4);
SamplerState LinearClamp : register(s0);
float luma(float3 c) { return dot(c,float3(.2126,.7152,.0722)); }
[numthreads(8,8,1)] void prepare(uint3 id:SV_DispatchThreadID) {
 if(any(id.xy>=uint2(W,H)))return;
 ProxyOut[id.xy]=float4(Source.SampleLevel(LinearClamp,(id.xy+.5)/float2(W,H),0).rgb,1);
}
float errorAt(int2 p,int2 delta) {
 float error=0;
 for(int y=-4;y<=4;y+=4)for(int x=-4;x<=4;x+=4){
  int2 a=clamp(p+int2(x,y),int2(0,0),int2(W,H)-1);
  int2 b=clamp(a+delta,int2(0,0),int2(W,H)-1);
  error+=abs(luma(Proxy.Load(int3(a,0)).rgb)-luma(Previous.Load(int3(b,0)).rgb));
 }
 return error;
}
[numthreads(8,8,1)] void flow(uint3 id:SV_DispatchThreadID) {
 uint2 count=(uint2(W,H)+15)/16;if(any(id.xy>=count))return;
 if(Reset||!UseMotion){FlowOut[id.xy]=0;return;}
 int2 p=min(int2(id.xy)*16+8,int2(W,H)-1),best=0;
 float base=errorAt(p,0),value=base;
 for(int y=-12;y<=12;y+=4)for(int x=-12;x<=12;x+=4){
  float e=errorAt(p,int2(x,y))+.0001*(abs(x)+abs(y));
  if(e<value){value=e;best=int2(x,y);}
 }
 int2 center=best;
 for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x){
  int2 d=center+int2(x,y);float e=errorAt(p,d)+.0001*(abs(d.x)+abs(d.y));
  if(e<value){value=e;best=d;}
 }
 // current -> previous, in inference pixels. Reject poor/ambiguous matches.
 FlowOut[id.xy]=(value<base*.9&&value<.7)?float2(best):float2(0,0);
}
[numthreads(8,8,1)] void guides(uint3 id:SV_DispatchThreadID) {
 if(any(id.xy>=uint2(W,H)))return;
 MotionOut[id.xy]=SmallFlow.SampleLevel(LinearClamp,(id.xy+.5)/float2(W,H),0);
 DepthOut[id.xy]=0; // Explicit flat depth, not game-engine depth.
}
[numthreads(8,8,1)] void composite(uint3 id:SV_DispatchThreadID) {
 if(any(id.xy>=uint2(SW,SH)))return;
 float2 uv=(id.xy+.5)/float2(SW,SH);
 float3 raw=Source.Load(int3(id.xy,0)).rgb;
 float3 residual=Neural.SampleLevel(LinearClamp,uv,0).rgb-Proxy.SampleLevel(LinearClamp,uv,0).rgb;
 FinalOut[id.xy]=float4(saturate(raw+residual),1);
}
)";

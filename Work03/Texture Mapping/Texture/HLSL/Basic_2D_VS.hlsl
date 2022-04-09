#include "Basic.hlsli"

// 顶点着色器(2D)
VertexPosHTex VS(VertexPosTex vIn)
{
    VertexPosHTex vOut;
    vOut.PosH = float4(vIn.PosL, 1.0f);
    vOut.Tex = vIn.Tex;
    return vOut;
    
    //VertexPosHTex vOut;
    //vOut.PosH = float4(vIn.PosL, 1.0f);
    //vOut.PosH = mul(float4(vIn.PosL, 1.0f), g_World);
    //vOut.PosH = mul(vOut.PosH, g_View);
    //vOut.PosH = mul(vOut.PosH, g_Proj);
    //float4 temp = float4(vIn.Tex, 0.0f, 1.0f);
    //temp = mul(temp, g_World);
    //vOut.Tex = temp.xy;
    //return vOut;
}
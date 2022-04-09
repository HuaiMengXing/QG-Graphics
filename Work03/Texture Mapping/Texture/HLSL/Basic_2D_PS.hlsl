#include "Basic.hlsli"
//Texture2DArray gTexArry : register(t0);

// 像素着色器(2D)
float4 PS(VertexPosHTex pIn) : SV_Target
{
    return g_Tex.Sample(g_SamLinear, pIn.Tex) * g_Tex1.Sample(g_SamLinear1,pIn.Tex);
}
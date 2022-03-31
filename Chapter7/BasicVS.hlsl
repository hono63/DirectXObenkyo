// Vertex Shader ... 1頂点ごとに呼ばれる
// pos: 頂点データ, uv:テクスチャ座標
// POSITION, SV_POSITION ... セマンティクス
// 戻り値は SV_POSITION なので 座標であることを意味し、 VS -> RS -> PS と送られていく

#include "BasicHeader.hlsli"

Output_t BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
//Output_t BasicVS(float4 pos : POSITION, float2 uv : TEXCORD)
{
	Output_t output;
	output.svpos = mul(mat, pos);
	//output.svpos = pos;
	output.normal = normal;
	output.uv = uv;
	return output;
}
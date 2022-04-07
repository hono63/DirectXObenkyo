// Vertex Shader ... 1頂点ごとに呼ばれる
// pos: 頂点データ, uv:テクスチャ座標
// POSITION, SV_POSITION ... セマンティクス
// 戻り値は SV_POSITION なので 座標であることを意味し、 VS -> RS -> PS と送られていく

#include "BasicHeader.hlsli"

Output_t BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	Output_t output;
	float w = weight / 100.0f;
	matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.0f - w); // bone matrix /w weight
	pos = mul(bm, pos);
	output.svpos = mul(mul(viewproj, world), pos); // 列優先計算
	//output.svpos = pos;
	normal.w = 0; // 法線の平行移動成分を無効にする
	output.normal = mul(world, normal); // 法線をワールド変換
	output.uv = uv;
	return output;
}
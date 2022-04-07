// 頂点シェーダから渡された座標情報を受け取り、レンダーターゲットへ書き込むための色情報を返す。
// SV_POSITION ... 座標

#include "BasicHeader.hlsli"

float4 BasicPS(Output_t input) : SV_TARGET
{
	//return float4(0.0f, 0.0f, 0.0f, 1.0f); // 黒
	//return float4(pos.x * 0.002f, pos.y * 0.002f, 0.1f, 1.0f); // gradation
	//return float4(input.uv, 1.0f, 1.0f); // gradation
	//return float4(tex.Sample(smp, input.uv));
	//return float4(input.normal.xyz, 1.0f);
	float3 light = normalize(float3(1, -1, 1)); // 光源の光線方向 右下奥
	float bright = dot(-light, input.normal); // ランバートの余弦則 物体表面の輝度はcosθに比例する
	return float4(0.1, 0.1, bright, 1.0f);
}
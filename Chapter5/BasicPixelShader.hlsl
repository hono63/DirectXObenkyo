// 頂点シェーダから渡された座標情報を受け取り、レンダーターゲットへ書き込むための色情報を返す。
// SV_POSITION ... 座標

#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output_t input) : SV_TARGET
{
	//return float4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
	//return float4(pos.x * 0.002f, pos.y * 0.002f, 0.1f, 1.0f); // gradation
	//return float4(input.uv, 1.0f, 1.0f); // gradation
	return float4(tex.Sample(smp, input.uv));
}
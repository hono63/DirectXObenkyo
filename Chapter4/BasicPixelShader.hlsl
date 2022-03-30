// 頂点シェーダから渡された座標情報を受け取り、レンダーターゲットへ書き込むための色情報を返す。

float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	//return float4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
	//return float4(1.0f, 0.1f, 0.1f, 1.0f); // 赤色
	//return float4((float2(0, 1) + pos.xy) * 0.5f, 1.0f, 1.0f);
	return float4(pos.x * 0.002f, pos.y * 0.002f, 0.1f, 1.0f); // gradation
}
// 頂点シェーダから渡された座標情報を受け取り、レンダーターゲットへ書き込むための色情報を返す。

float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
}
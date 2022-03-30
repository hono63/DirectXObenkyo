
// VSからPSへのやりとりに用いる構造体
struct Output_t
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float2 uv : TEXCORD; // uv座標
};

// テクスチャ用
Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); // 0番スロットに設定されたサンプラー
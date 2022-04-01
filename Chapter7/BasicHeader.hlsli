
// VSからPSへのやりとりに用いる構造体
struct Output_t
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float4 normal : NORMAL; // 法線ベクトル
	float2 uv : TEXCORD; // uv座標
};

// テクスチャ用
Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); // 0番スロットに設定されたサンプラー

// 定数バッファ用
cbuffer cbuff0 : register(b0)
{
	//matrix mat; // 変換行列
	matrix world;
	matrix viewproj;
}
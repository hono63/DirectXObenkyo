
// VSからPSへのやりとりに用いる構造体
struct Output_t
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float2 uv : TEXCORD; // uv座標
};
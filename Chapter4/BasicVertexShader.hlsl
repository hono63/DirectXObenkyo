// 1頂点ごとに呼ばれる
// pos: 頂点データ
// POSITION, SV_POSITION ... セマンティクス
// 戻り値は SV_POSITION なので 座標であることを意味し、 VS -> RS -> PS と送られていく

float4 BasicVS( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}

// VS����PS�ւ̂��Ƃ�ɗp����\����
struct Output_t
{
	float4 svpos : SV_POSITION; // �V�X�e���p���_���W
	float4 normal : NORMAL; // �@���x�N�g��
	float2 uv : TEXCORD; // uv���W
};

// �e�N�X�`���p
Texture2D<float4> tex : register(t0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

// �萔�o�b�t�@�p
cbuffer cbuff0 : register(b0)
{
	//matrix mat; // �ϊ��s��
	matrix world;
	matrix viewproj;
}
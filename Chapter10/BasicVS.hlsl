// Vertex Shader ... 1���_���ƂɌĂ΂��
// pos: ���_�f�[�^, uv:�e�N�X�`�����W
// POSITION, SV_POSITION ... �Z�}���e�B�N�X
// �߂�l�� SV_POSITION �Ȃ̂� ���W�ł��邱�Ƃ��Ӗ����A VS -> RS -> PS �Ƒ����Ă���

#include "BasicHeader.hlsli"

Output_t BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
	Output_t output;
	float w = weight / 100.0f;
	matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.0f - w); // bone matrix /w weight
	pos = mul(bm, pos);
	output.svpos = mul(mul(viewproj, world), pos); // ��D��v�Z
	//output.svpos = pos;
	normal.w = 0; // �@���̕��s�ړ������𖳌��ɂ���
	output.normal = mul(world, normal); // �@�������[���h�ϊ�
	output.uv = uv;
	return output;
}
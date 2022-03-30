// Vertex Shader ... 1���_���ƂɌĂ΂��
// pos: ���_�f�[�^, uv:�e�N�X�`�����W
// POSITION, SV_POSITION ... �Z�}���e�B�N�X
// �߂�l�� SV_POSITION �Ȃ̂� ���W�ł��邱�Ƃ��Ӗ����A VS -> RS -> PS �Ƒ����Ă���

#include "BasicShaderHeader.hlsli"

Output_t BasicVS(float4 pos : POSITION, float2 uv : TEXCORD)
{
	Output_t output;
	output.svpos = pos;
	output.uv = uv;
	return output;
}
// ���_�V�F�[�_����n���ꂽ���W�����󂯎��A�����_�[�^�[�Q�b�g�֏������ނ��߂̐F����Ԃ��B
// SV_POSITION ... ���W

#include "BasicHeader.hlsli"

float4 BasicPS(Output_t input) : SV_TARGET
{
	return float4(0.0f, 0.0f, 0.0f, 1.0f); // ��
	//return float4(pos.x * 0.002f, pos.y * 0.002f, 0.1f, 1.0f); // gradation
	//return float4(input.uv, 1.0f, 1.0f); // gradation
	//return float4(tex.Sample(smp, input.uv));
}
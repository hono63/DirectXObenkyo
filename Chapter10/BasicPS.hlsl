// ���_�V�F�[�_����n���ꂽ���W�����󂯎��A�����_�[�^�[�Q�b�g�֏������ނ��߂̐F����Ԃ��B
// SV_POSITION ... ���W

#include "BasicHeader.hlsli"

float4 BasicPS(Output_t input) : SV_TARGET
{
	//return float4(0.0f, 0.0f, 0.0f, 1.0f); // ��
	//return float4(pos.x * 0.002f, pos.y * 0.002f, 0.1f, 1.0f); // gradation
	//return float4(input.uv, 1.0f, 1.0f); // gradation
	//return float4(tex.Sample(smp, input.uv));
	//return float4(input.normal.xyz, 1.0f);
	float3 light = normalize(float3(1, -1, 1)); // �����̌������� �E����
	float bright = dot(-light, input.normal); // �����o�[�g�̗]���� ���̕\�ʂ̋P�x��cos�Ƃɔ�Ⴗ��
	return float4(0.1, 0.1, bright, 1.0f);
}
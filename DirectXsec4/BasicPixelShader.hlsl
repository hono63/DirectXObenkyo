// ���_�V�F�[�_����n���ꂽ���W�����󂯎��A�����_�[�^�[�Q�b�g�֏������ނ��߂̐F����Ԃ��B

float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	//return float4(1.0f, 1.0f, 1.0f, 1.0f); // ���F
	//return float4(1.0f, 0.1f, 0.1f, 1.0f); // �ԐF
	//return float4((float2(0, 1) + pos.xy) * 0.5f, 1.0f, 1.0f);
	return float4(pos.x * 0.002f, pos.y * 0.002f, 0.1f, 1.0f); // gradation
}
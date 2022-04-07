#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;

struct vertex_t {
	XMFLOAT3 pos; // xyz
	XMFLOAT2 uv; // texture;
};

class Sankaku {
private:
	CCommon& mC;
	//XMFLOAT3 mVertices[3];
	vertex_t mVertices[4];
	unsigned short mIndices[6] = { // �l�p�`�p�C���f�b�N�X
		0, 1, 2,
		0, 2, 3,
	};
	ID3D12Resource* mVertBuff = nullptr;
	ID3D12Resource* mIdxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};
public:

public:
	Sankaku(CCommon& cmn) : mC(cmn) {
		// ���_�̏����͎��v���ɂȂ�悤�ɁB
#if 0
		mVertices[0] = XMFLOAT3{ -0.5f, -0.7f, 0.f }; // ����
		mVertices[1] = XMFLOAT3{ 0.f,  0.9f, 0.f }; // ����
		mVertices[2] = XMFLOAT3{ 0.8f, -0.5f, 0.f }; // �E��
#elif 1
		// �l�p
		float sz = 1.0f;
		mVertices[0].pos = XMFLOAT3{ -sz, -sz, 0.f };
		mVertices[1].pos = XMFLOAT3{ -sz, +sz, 0.f };
		mVertices[2].pos = XMFLOAT3{ +sz, +sz, 0.f };
		mVertices[3].pos = XMFLOAT3{ +sz, -sz, 0.f };
		mVertices[0].uv = XMFLOAT2{ 0.f, 1.f };
		mVertices[1].uv = XMFLOAT2{ 0.f, 0.f };
		mVertices[2].uv = XMFLOAT2{ 1.f, 0.f };
		mVertices[3].uv = XMFLOAT2{ 1.f, 1.f };
#else
		// �l�p (2�������W) 
		mVertices[0].pos = XMFLOAT3{ 0.f, 100.f, 0.f };
		mVertices[1].pos = XMFLOAT3{ 0.f, 0.f, 0.f };
		mVertices[2].pos = XMFLOAT3{ 100.f, 0.f, 0.f };
		mVertices[3].pos = XMFLOAT3{ 100.f, 100.f, 0.f };
		mVertices[0].uv = XMFLOAT2{ 0.f, 1.f };
		mVertices[1].uv = XMFLOAT2{ 0.f, 0.f };
		mVertices[2].uv = XMFLOAT2{ 1.f, 0.f };
		mVertices[3].uv = XMFLOAT2{ 1.f, 1.f };
#endif
	}

	/// <summary>
	/// ���_�o�b�t�@�̍쐬
	/// </summary>
	/// <param name="_dev"></param>
	void MakeVertBuff(ID3D12Device* _dev)
	{
		mC.MakeBuffResource(_dev, sizeof(mVertices), &mVertBuff);
	}
	/// <summary>
	/// index�o�b�t�@�̍쐬
	/// </summary>
	/// <param name="_dev"></param>
	void MakeIndexBuff(ID3D12Device* _dev)
	{
		mC.MakeBuffResource(_dev, sizeof(mIndices), &mIdxBuff);
	}


	/// <summary>
	/// ���_�o�b�t�@�̉��z�A�h���X�ɒ��_�f�[�^����������
	/// </summary>
	void MapVertex()
	{
		mC.Map(mVertBuff, mVertices, sizeof(mVertices));
	}
	void MapIndex()
	{
		mC.Map(mIdxBuff, mIndices, sizeof(mIndices));
	}

	/// <summary>
	/// ���_�o�b�t�@�[�r���[�E���_�C���f�b�N�X�o�b�t�@�[�r���[�̍쐬
	/// </summary>
	void MakeView(ID3D12Device* _dev)
	{
		mVbView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
		mVbView.SizeInBytes = sizeof(mVertices); // �S�o�C�g��
		mVbView.StrideInBytes = sizeof(mVertices[0]); // 1���_�̃o�C�g��

		// Index Buffer View
		mIbView.BufferLocation = mIdxBuff->GetGPUVirtualAddress();
		mIbView.SizeInBytes = sizeof(mIndices);
		mIbView.Format = DXGI_FORMAT_R16_UINT; // unsinged short�^
	}


	/// <summary>
	/// �O�p�`�̕`��
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->IASetIndexBuffer(&mIbView);// ���_�C���f�b�N�X

		_cmdList->IASetVertexBuffers(
			0, // �X���b�g�ԍ�
			1, // ���_�o�b�t�@�[�r���[�̐�
			&mVbView
		);
#if 0
		_cmdList->DrawInstanced(
			//3, // ���_��
			4, // ���_��
			1, // �C���X�^���X��
			0, // ���_�f�[�^�̃I�t�Z�b�g
			0 // �C���X�^���X�̃I�t�Z�b�g
		);
#endif
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
};


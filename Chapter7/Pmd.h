#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;


class CPmd {
private:
	vector<uint8_t> mVertices;
	vector<uint16_t> mIndices;
	ID3D12Resource* mVertBuff = nullptr;
	ID3D12Resource* mIdxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVertView = {};
	D3D12_INDEX_BUFFER_VIEW mIdxView = {};
	uint32_t mVertNum = 0;
	uint32_t mIdxNum = 0;
	CCommon mC;
public:
	D3D12_INPUT_ELEMENT_DESC mLayout[6] = {}; // ���_���̓��C�A�E�g
	static const size_t PMD_LAYOUT_NUM = 6;
	static const size_t PMD_VERTEX_SIZE = 38; // ���_�T�C�Y��38byte
	
	struct PMDHeader_t {
		float versin;
		char model_name[20];
		char comment[256];
	};
	PMDHeader_t mHeader;

	struct PMDVertex_t {
		XMFLOAT3 pos;
		XMFLOAT3 normal; // �@���x�N�g��
		XMFLOAT2 uv;
		uint16_t boneNo[2]; // �{�[���ԍ�
		uint8_t boneWeight; // �{�[���e���x
		uint8_t edgeFlg; // �֊s���t���O
	};

	CPmd(CCommon& cmd) : mC(cmd) {}

	/// <summary>
	/// PMD�t�@�C����ǂݍ���
	/// </summary>
	void ReadFile() 
	{
		char signature[3] = {}; // �擪�R������"pmd"
		FILE* fp = nullptr;
		fopen_s(&fp, "model/�����~�N.pmd", "rb");
		_ASSERT(fp != nullptr);
		
		fread(signature, sizeof(signature), 1, fp);
		fread(&mHeader, sizeof(mHeader), 1, fp);
		
		fread(&mVertNum, sizeof(mVertNum), 1, fp); // ���_��
		mVertices.resize(mVertNum * PMD_VERTEX_SIZE); // ���_�� x 38byte ���m��
		fread(mVertices.data(), mVertices.size(), 1, fp); // ���_�f�[�^
		
		fread(&mIdxNum, sizeof(mIdxNum), 1, fp); // �C���f�b�N�X��
		mIndices.resize(mIdxNum);
		fread(mIndices.data(), mIndices.size() * sizeof(uint16_t), 1, fp);

		fclose(fp);
	}

	void MakeBuff(ID3D12Device* _dev)
	{
		mC.MakeBuffResource(_dev, mVertices.size(), &mVertBuff);
		mC.MakeBuffResource(_dev, mIndices.size() * sizeof(uint16_t), &mIdxBuff);
	}

	void Map()
	{
		mC.Map(mVertBuff, mVertices.data(), mVertices.size());
		mC.Map(mIdxBuff, mIndices.data(), mIndices.size() * sizeof(uint16_t));
	}

	void MakeView()
	{
		mVertView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
		mVertView.SizeInBytes = mVertices.size(); // �S�o�C�g��
		mVertView.StrideInBytes = PMD_VERTEX_SIZE; // 1���_�̃o�C�g��

		mIdxView.BufferLocation = mIdxBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
		mIdxView.SizeInBytes = mIndices.size() * sizeof(uint16_t); // �S�o�C�g��
		mIdxView.Format = DXGI_FORMAT_R16_UINT;
		//mIdxView.StrideInBytes = sizeof(uint16_t); // 1�C���f�b�N�X�̃o�C�g��
	}

	/// <summary>
/// ���_���C�A�E�g�E�e�N�X�`��uv���W���C�A�E�g�\���̂̍쐬
/// </summary>
	void MakeLayout()
	{
		// ���W���
		mLayout[0].SemanticName = "POSITION"; // ���W�ł��邱�Ƃ��Ӗ�����
		mLayout[0].SemanticIndex = 0; // �����Z�}���e�B�N�X���̂Ƃ��͎g�p����
		mLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // 32bit x 3 �̃f�[�^
		mLayout[0].InputSlot = 0; // �����̒��_�f�[�^�����킹�ĂP�̒��_�f�[�^��\���������Ƃ��Ɏg�p����
		mLayout[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // ���̃f�[�^�̏ꏊ. ����͎����玟�Ƀf�[�^������ł��邱�Ƃ�\���B
		mLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // �f�[�^�̓��e�Ƃ��āA1���_���Ƃɂ��̃��C�A�E�g�������Ă���B
		mLayout[0].InstanceDataStepRate = 0; // �C���X�^���V���O�̂Ƃ��A�P�x�ɕ`�悷��C���X�^���X�̐����w�肷��B
		// �@��
		mLayout[1].SemanticName = "NORMAL";
		mLayout[1].SemanticIndex = 0;
		mLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		mLayout[1].InputSlot = 0;
		mLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[1].InstanceDataStepRate = 0;
		// uv
		mLayout[2].SemanticName = "TEXCORD";
		mLayout[2].SemanticIndex = 0;
		mLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT; // 32bit x 2 �̃f�[�^
		mLayout[2].InputSlot = 0;
		mLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[2].InstanceDataStepRate = 0;
		// �{�[���ԍ�
		mLayout[3].SemanticName = "BONE_NO";
		mLayout[3].SemanticIndex = 0;
		mLayout[3].Format = DXGI_FORMAT_R16G16_UINT; // 16bit x 2 �̃f�[�^
		mLayout[3].InputSlot = 0;
		mLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[3].InstanceDataStepRate = 0;
		// �E�F�C�g
		mLayout[4].SemanticName = "WEIGHT";
		mLayout[4].SemanticIndex = 0;
		mLayout[4].Format = DXGI_FORMAT_R8_UINT; // 8bit �̃f�[�^
		mLayout[4].InputSlot = 0;
		mLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[4].InstanceDataStepRate = 0;
		// �֊s���t���O
		mLayout[5].SemanticName = "EDGE_FLG";
		mLayout[5].SemanticIndex = 0;
		mLayout[5].Format = DXGI_FORMAT_R8_UINT; // 8bit �̃f�[�^
		mLayout[5].InputSlot = 0;
		mLayout[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[5].InstanceDataStepRate = 0;
	}

	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->IASetIndexBuffer(&mIdxView);// ���_�C���f�b�N�X
		_cmdList->IASetVertexBuffers(
			0, // �X���b�g�ԍ�
			1, // ���_�o�b�t�@�[�r���[�̐�
			&mVertView
		);
		_cmdList->DrawIndexedInstanced(mIdxNum, 1, 0, 0, 0);
	}
};
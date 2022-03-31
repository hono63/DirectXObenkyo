#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;

struct vertex_t {
	XMFLOAT3 pos; // xyz
	XMFLOAT2 uv; // texture;
};

struct texture_t {
	unsigned char R, G, B, A;
};

class Sankaku {
private:
	CCommon& mC;
	//XMFLOAT3 mVertices[3];
	vertex_t mVertices[4];
	ID3D12Resource* mVertBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};
	vector<texture_t> mTexData; // �e�N�X�`���f�[�^
	TexMetadata mTexMeta = {};
	ScratchImage mTexScratch;
	const Image *mTexImage;
public:
	ID3D12Resource* mTexBuffRes = nullptr; // �e�N�X�`���o�b�t�@

public:
	Sankaku(CCommon& cmn) : mC(cmn) {
		// ���_�̏����͎��v���ɂȂ�悤�ɁB
#if 0
		mVertices[0] = XMFLOAT3{ -0.5f, -0.7f, 0.f }; // ����
		mVertices[1] = XMFLOAT3{ 0.f,  0.9f, 0.f }; // ����
		mVertices[2] = XMFLOAT3{ 0.8f, -0.5f, 0.f }; // �E��
#elif 0
		// �l�p
		mVertices[0].pos = XMFLOAT3{ -0.7f, -0.7f, 0.f };
		mVertices[1].pos = XMFLOAT3{ -0.7f, +0.9f, 0.f };
		mVertices[2].pos = XMFLOAT3{ +0.7f, +0.7f, 0.f };
		mVertices[3].pos = XMFLOAT3{ +0.7f, -0.7f, 0.f };
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
		mTexData.resize(256 * 256);
		for (auto& d : mTexData) {
			d.R = rand() % 256;
			d.G = rand() % 256;
			d.B = rand() % 256;
			d.A = 255u;
		}
	}

	/// <summary>
	/// �e�N�X�`���摜���t�@�C������ǂݍ���
	/// </summary>
	void ReadTex()
	{
		// COM�̏����� ��������Ȃ��ƃC���^�[�t�F�[�X�֘A�G���[���o��
		HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED);
		_ASSERT(result == S_OK);

		result = LoadFromWICFile(
			//L"img/textest.png",
			L"img/256.jpg",
			WIC_FLAGS_NONE,
			&mTexMeta,
			mTexScratch
		);
		_ASSERT(result == S_OK);

		mTexImage = mTexScratch.GetImage(0, 0, 0); // ScratchImage���������mTexImage�������Ă��܂�
	}

	/// <summary>
	/// ���_�o�b�t�@�̍쐬
	/// </summary>
	/// <param name="_dev"></param>
	void MakeVertBuff(ID3D12Device* _dev)
	{
		MakeBuffResource(_dev, sizeof(mVertices), &mVertBuff);
	}

	void MakeBuffResource(ID3D12Device* _dev, UINT64 buffsize, ID3D12Resource** resource)
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD; // �q�[�v��ʁBMap����Ȃ�UPLOAD�B���Ȃ��Ȃ�DEFAULT�B
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // CPU�y�[�W���O�ݒ�
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // �������v�[�����ǂ���
		prop.CreationNodeMask = 0; // �P��A�_�v�^�[�Ȃ�0
		prop.VisibleNodeMask = 0; // �P��A�_�v�^�[�Ȃ�0

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // �o�b�t�@�[�Ƃ��Ďg���̂ŁB
		desc.Alignment = 0;
		desc.Width = buffsize;
		desc.Height = 1; // Width�őS���܂��Ȃ�
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN; // �摜�ł͂Ȃ��̂�UNKNOWN�f���C
		desc.SampleDesc.Count = 1; // �A���`�G�C���A�V���O���s���Ƃ��̃p�����[�^�B�s��Ȃ��ꍇ1�ł悢�B
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // ���������ŏ�����Ō�܂ŘA�����Ă��邱�Ƃ�����ROW_MAJOR�B
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, // �ǂݎ���p
			nullptr,
			IID_PPV_ARGS(resource)
		);
		_ASSERT(result == S_OK);
	}

	void MakeTexBuff(ID3D12Device* _dev)
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_CUSTOM;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // CPU�����璼�ړ]������
		prop.CreationNodeMask = 0;
		prop.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
#if 0
		desc.Width = 256;
		desc.Height = 256;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1; //�~�b�v�}�b�v���Ȃ�
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA�t�H�[�}�b�g
#else // �摜�ǂݍ���
		desc.Width = mTexMeta.width;
		desc.Height = mTexMeta.height;
		desc.DepthOrArraySize = mTexMeta.arraySize;
		desc.MipLevels = mTexMeta.mipLevels;
		desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(mTexMeta.dimension);
		desc.Format = mTexMeta.format;
#endif
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͓��Ɍ��肵�Ȃ�
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // �e�N�X�`���p�w��
			nullptr,
			IID_PPV_ARGS(&mTexBuffRes)
		);
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// ���_�o�b�t�@�̉��z�A�h���X�ɒ��_�f�[�^����������
	/// </summary>
	void MapVertex()
	{
		mC.Map(mVertBuff, mVertices, sizeof(mVertices));
	}

	/// <summary>
	/// �e�N�X�`������������
	/// </summary>
	void WriteTex()
	{
#if 0
		HRESULT result = mTexBuffRes->WriteToSubresource(
			0, // �T�u���\�[�X �C���f�b�N�X
			nullptr, // �S�̈�փR�s�[
			mTexData.data(),
			sizeof(texture_t) * 256, //1���C���̃T�C�Y
			sizeof(texture_t) * mTexData.size() // �S�T�C�Y
		);
#else // �摜�ǂݍ���
		HRESULT result = mTexBuffRes->WriteToSubresource(
			0, // �T�u���\�[�X �C���f�b�N�X
			nullptr, // �S�̈�փR�s�[
			mTexImage->pixels,
			mTexImage->rowPitch,
			mTexImage->slicePitch
		);
#endif
		_ASSERT(result == S_OK);
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
		unsigned short indices[] = { // �l�p�`�p�C���f�b�N�X
			0, 1, 2,
			0, 2, 3,
		};
		ID3D12Resource* idxBuff = nullptr;
		MakeBuffResource(_dev, sizeof(indices), &idxBuff);
		mC.Map(idxBuff, indices, sizeof(indices));
		mIbView.BufferLocation = idxBuff->GetGPUVirtualAddress();
		mIbView.SizeInBytes = sizeof(indices);
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


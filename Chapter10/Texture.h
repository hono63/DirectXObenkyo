#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;

struct texture_t {
	unsigned char R, G, B, A;
};

class CTexture {
private:
	CCommon& mC;
	vector<texture_t> mTexData; // �e�N�X�`���f�[�^
	TexMetadata mTexMeta = {};
	ScratchImage mTexScratch;
	const Image* mTexImage;
public:
	ID3D12Resource* mTexBuffRes = nullptr; // �e�N�X�`���o�b�t�@

public:
	CTexture(CCommon& cmn) : mC(cmn) {
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
	void ReadFile()
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


	void MakeBuff(ID3D12Device* _dev)
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
	/// �e�N�X�`������������
	/// </summary>
	void WriteSubresource()
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

};


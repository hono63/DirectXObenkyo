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
	vector<texture_t> mTexData; // テクスチャデータ
	TexMetadata mTexMeta = {};
	ScratchImage mTexScratch;
	const Image* mTexImage;
public:
	ID3D12Resource* mTexBuffRes = nullptr; // テクスチャバッファ

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
	/// テクスチャ画像をファイルから読み込む
	/// </summary>
	void ReadFile()
	{
		// COMの初期化 これをやらないとインターフェース関連エラーが出る
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

		mTexImage = mTexScratch.GetImage(0, 0, 0); // ScratchImageが消えるとmTexImageも消えてしまう
	}


	void MakeBuff(ID3D12Device* _dev)
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_CUSTOM;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // CPU側から直接転送する
		prop.CreationNodeMask = 0;
		prop.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
#if 0
		desc.Width = 256;
		desc.Height = 256;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1; //ミップマップしない
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2Dテクスチャ
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBAフォーマット
#else // 画像読み込み
		desc.Width = mTexMeta.width;
		desc.Height = mTexMeta.height;
		desc.DepthOrArraySize = mTexMeta.arraySize;
		desc.MipLevels = mTexMeta.mipLevels;
		desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(mTexMeta.dimension);
		desc.Format = mTexMeta.format;
#endif
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // レイアウトは特に決定しない
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // テクスチャ用指定
			nullptr,
			IID_PPV_ARGS(&mTexBuffRes)
		);
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// テクスチャを書き込む
	/// </summary>
	void WriteSubresource()
	{
#if 0
		HRESULT result = mTexBuffRes->WriteToSubresource(
			0, // サブリソース インデックス
			nullptr, // 全領域へコピー
			mTexData.data(),
			sizeof(texture_t) * 256, //1ラインのサイズ
			sizeof(texture_t) * mTexData.size() // 全サイズ
		);
#else // 画像読み込み
		HRESULT result = mTexBuffRes->WriteToSubresource(
			0, // サブリソース インデックス
			nullptr, // 全領域へコピー
			mTexImage->pixels,
			mTexImage->rowPitch,
			mTexImage->slicePitch
		);
#endif
		_ASSERT(result == S_OK);
	}

};


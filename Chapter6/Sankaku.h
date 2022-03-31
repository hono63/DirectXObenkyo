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
	unsigned short mIndices[6] = { // 四角形用インデックス
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
		// 頂点の順序は時計回りになるように。
#if 0
		mVertices[0] = XMFLOAT3{ -0.5f, -0.7f, 0.f }; // 左下
		mVertices[1] = XMFLOAT3{ 0.f,  0.9f, 0.f }; // 左上
		mVertices[2] = XMFLOAT3{ 0.8f, -0.5f, 0.f }; // 右下
#elif 1
		// 四角
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
		// 四角 (2次元座標) 
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
	/// 頂点バッファの作成
	/// </summary>
	/// <param name="_dev"></param>
	void MakeVertBuff(ID3D12Device* _dev)
	{
		MakeBuffResource(_dev, sizeof(mVertices), &mVertBuff);
	}
	/// <summary>
	/// indexバッファの作成
	/// </summary>
	/// <param name="_dev"></param>
	void MakeIndexBuff(ID3D12Device* _dev)
	{
		MakeBuffResource(_dev, sizeof(mIndices), &mIdxBuff);
	}

	void MakeBuffResource(ID3D12Device* _dev, UINT64 buffsize, ID3D12Resource** resource)
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD; // ヒープ種別。MapするならUPLOAD。しないならDEFAULT。
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // CPUページング設定
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // メモリプールがどこか
		prop.CreationNodeMask = 0; // 単一アダプターなら0
		prop.VisibleNodeMask = 0; // 単一アダプターなら0

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファーとして使うので。
		desc.Alignment = 0;
		desc.Width = buffsize;
		desc.Height = 1; // Widthで全部まかなう
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN; // 画像ではないのでUNKNOWNデヨイ
		desc.SampleDesc.Count = 1; // アンチエイリアシングを行うときのパラメータ。行わない場合1でよい。
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // メモリが最初から最後まで連続していることを示すROW_MAJOR。
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, // 読み取り専用
			nullptr,
			IID_PPV_ARGS(resource)
		);
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// 頂点バッファの仮想アドレスに頂点データを書き込む
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
	/// 頂点バッファービュー・頂点インデックスバッファービューの作成
	/// </summary>
	void MakeView(ID3D12Device* _dev)
	{
		mVbView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
		mVbView.SizeInBytes = sizeof(mVertices); // 全バイト数
		mVbView.StrideInBytes = sizeof(mVertices[0]); // 1頂点のバイト数

		// Index Buffer View
		mIbView.BufferLocation = mIdxBuff->GetGPUVirtualAddress();
		mIbView.SizeInBytes = sizeof(mIndices);
		mIbView.Format = DXGI_FORMAT_R16_UINT; // unsinged short型
	}


	/// <summary>
	/// 三角形の描画
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->IASetIndexBuffer(&mIbView);// 頂点インデックス

		_cmdList->IASetVertexBuffers(
			0, // スロット番号
			1, // 頂点バッファービューの数
			&mVbView
		);
#if 0
		_cmdList->DrawInstanced(
			//3, // 頂点数
			4, // 頂点数
			1, // インスタンス数
			0, // 頂点データのオフセット
			0 // インスタンスのオフセット
		);
#endif
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
};


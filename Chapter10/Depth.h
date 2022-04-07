#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;

class CDepth {
	CCommon mC;
	ID3D12Resource* mDepthBuff = nullptr;
	ID3D12DescriptorHeap* mDescHeap = nullptr;
public:
	CDepth(CCommon& cmn) : mC(cmn) {}

	void MakeBuff(ID3D12Device* _dev)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = WINDOW_WIDTH;
		desc.Height = WINDOW_HEIGHT;
		desc.DepthOrArraySize = 1;
		desc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用フォーマット
		desc.SampleDesc.Count = 1;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		
		D3D12_CLEAR_VALUE cvalue = {};
		cvalue.DepthStencil.Depth = 1.0f; // 深度値1（最大）でクリア
		cvalue.Format = DXGI_FORMAT_D32_FLOAT;

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&cvalue,
			IID_PPV_ARGS(&mDepthBuff)
		);
		_ASSERT(result == S_OK);
	}

	void MakeDescriptor(ID3D12Device* _dev)
	{
		// まずディスクリプターヒープを作る
		D3D12_DESCRIPTOR_HEAP_DESC hpdesc = {};
		hpdesc.NumDescriptors = 1;
		hpdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // シェーダーリソースビュー用

		HRESULT result = _dev->CreateDescriptorHeap(&hpdesc, IID_PPV_ARGS(&mDescHeap));
		_ASSERT(result == S_OK);

		// 深度ステンシルビューを作る
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc = {};
		dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvdesc.Flags = D3D12_DSV_FLAG_NONE;

		_dev->CreateDepthStencilView(mDepthBuff, &dsvdesc, mDescHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void Clear(ID3D12GraphicsCommandList* _cmdList)
	{
		auto handle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->ClearDepthStencilView(
			handle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, // 深度クリア値 最大深度でクリア
			0.0f,
			0,
			nullptr // 全範囲
		);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* GetDescHandle()
	{
		static auto handle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
		return &handle;
	}
};
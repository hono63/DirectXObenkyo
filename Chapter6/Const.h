#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;

class ConstBuf {
public:
	CCommon& mC;
	XMMATRIX mMat = XMMatrixIdentity();
	ID3D12Resource* mBuf = nullptr;

	ConstBuf(CCommon& cmn) : mC(cmn) {
		mMat.r[0].m128_f32[0] = 2.0f / WINDOW_WIDTH;
		mMat.r[1].m128_f32[1] = -2.0f / WINDOW_HEIGHT;
	}

	void MakeResourceBuf(ID3D12Device* _dev) 
	{
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(mMat) + 0x000000ff) & ~0x000000ff); // 256byteƒAƒ‰ƒCƒƒ“ƒg

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mBuf)
		);
		_ASSERT(result == S_OK);
	}

	void Map()
	{
		mC.Map(mBuf, &mMat, sizeof(mMat));
	}


};
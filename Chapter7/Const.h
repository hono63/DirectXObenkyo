#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;

// 定数バッファ用クラス
class ConstBuf {
public:
	CCommon& mC;
	//XMMATRIX mMat = XMMatrixIdentity();
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProj;
	ID3D12Resource* mBuf = nullptr;
	struct MatData_t{
		XMMATRIX world;
		XMMATRIX viewproj;
	} mMat;

	ConstBuf(CCommon& cmn) : mC(cmn) {
#if 0 // gamenn haji
		mMat.r[0].m128_f32[0] = 2.0f / WINDOW_WIDTH;
		mMat.r[1].m128_f32[1] = -2.0f / WINDOW_HEIGHT;
		mMat.r[3].m128_f32[0] = -1.0f;
		mMat.r[3].m128_f32[1] = 1.0f;
#else // 3D
		//mWorld = XMMatrixRotationY(XM_PIDIV4); // 45 deg
		mMat.world = XMMatrixIdentity();
		XMFLOAT3 eye(0, 10, -15);
		XMFLOAT3 target(0, 10, 0);
		XMFLOAT3 up(0, 1, 0);
		mView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		mProj = XMMatrixPerspectiveFovLH(
			XM_PIDIV2, // FOV: 90 deg
			(float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
			1.0f, // near
			100.0f // far
		);
		//mMat = mWorld * mView * mProj;
		mMat.viewproj = mView * mProj;
#endif
	}

	void MakeBuff(ID3D12Device* _dev) 
	{
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(mMat) + 0x000000ff) & ~0x000000ff); // 256byteアライメント

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

	// 世界を回転させる
	void Kaiten()
	{
		static float angle = XM_PIDIV4;
		angle += 0.03f;
		mMat.world = XMMatrixRotationY(angle);
		//mMat = mWorld * mView * mProj;
		Map();
	}
};
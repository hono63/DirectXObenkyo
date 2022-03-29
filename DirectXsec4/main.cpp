
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <tchar.h> // _T() 定義
#include <stdlib.h>
#include <time.h>

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "Gamen.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace std;

/// <summary>
/// デバッグ用printf
/// </summary>
/// <param name="format"></param>
/// <param name=""></param>
void DebugString(const TCHAR* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	_vtprintf(format, valist);
	va_end(valist);
#else
	TCHAR buf[1024];
	va_list valist;
	va_start(valist, format);
	_vstprintf_s(buf, format, valist);
	va_end(valist);
	OutputDebugString(buf);
#endif
}

/// <summary>
/// おまじない
/// </summary>
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY) {
		PostQuitMessage(0); // OSに対してこのアプリの終了を伝える。
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); //  既定の処理を行う
}



class Sankaku {
private:
	XMFLOAT3 mVertices[3];
	ID3D12Resource* mVertBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	ID3DBlob* mVsBlob = nullptr; // 頂点シェーダのBlob (Binary Large Object)
	ID3DBlob* mPsBlob = nullptr; // ピクセルシェーダのBlob
	ID3DBlob* mErrorBlob = nullptr; // エラーメッセージ用
	ID3D12PipelineState* mPipe = nullptr; // グラフィックス パイプライン ステート

public:
	Sankaku() {
		// 頂点の順序は反時計回りになるように。
		mVertices[0] = XMFLOAT3{ -1.f, -1.f, 0.f }; // 左下
		mVertices[1] = XMFLOAT3{ -1.f,  1.f, 0.f }; // 左上
		mVertices[2] = XMFLOAT3{  1.f, -1.f, 0.f }; // 右下
	}

	/// <summary>
	/// 頂点バッファの作成
	/// </summary>
	/// <param name="_dev"></param>
	void MakeVertBuff(ID3D12Device* _dev)
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
		desc.Width = sizeof(mVertices);
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
			IID_PPV_ARGS(&mVertBuff)
		);
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// 頂点バッファの仮想アドレスに頂点データを書き込む
	/// </summary>
	void Map()
	{
		XMFLOAT3* vertMap = nullptr;
		mVertBuff->Map(
			0, // サブリソース。ミニマップなどないため0。
			nullptr, // 範囲指定なし。
			(void**)&vertMap
		);
		std::copy(std::begin(mVertices), std::end(mVertices), vertMap);
		mVertBuff->Unmap(0, nullptr); // もうマップを解除してええで
	}

	/// <summary>
	/// 頂点バッファービューの作成
	/// </summary>
	void MakeVbView()
	{
		mVbView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
		mVbView.SizeInBytes = sizeof(mVertices); // 全バイト数
		mVbView.StrideInBytes = sizeof(mVertices[0]); // 1頂点のバイト数
	}

	void CompileVS()
	{
		HRESULT result = D3DCompileFromFile(
			_T("BasicVertexShader.hlsl"),
			nullptr, // シェーダー用マクロ
			D3D_COMPILE_STANDARD_FILE_INCLUDE, // includeするもの
			"BasicVS", // Entry Point
			"vs_5_0", // shader version
			(D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION), // Compile Option
			0, // シェーダーファイルの場合0にすることが推奨
			&mVsBlob,
			&mErrorBlob
		);
		if (result != S_OK) {
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				OutputDebugString(_T("ファイルが見当たらないらしい"));
			}
			string err;
			err.resize(mErrorBlob->GetBufferSize());
			std::copy_n(err.data(), mErrorBlob->GetBufferPointer(), mErrorBlob->GetBufferSize());
			OutputDebugStringA(err.c_str());
		}
		_ASSERT(result == S_OK);
	}

	void LayoutVS()
	{
		D3D12_INPUT_ELEMENT_DESC desc[1] = {};
		desc[0].SemanticName = "POSITION"; // 座標であることを意味する
		desc[0].SemanticIndex = 0; // 同じセマンティクス名のときは使用する
		desc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // 32bit x 3 のデータ
		desc[0].InputSlot = 0; // 複数の頂点データを合わせて１つの頂点データを表現したいときに使用する
		desc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // そのデータの場所. 今回は次から次にデータが並んでいることを表す。
		desc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // データの内容として、1頂点ごとにこのレイアウトが入っている。
		desc[0].InstanceDataStepRate = 0; // インスタンシングのとき、１度に描画するインスタンスの数を指定する。

	}

	void MakePipeline()
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = nullptr;
		// Vertex Shader
		desc.VS.pShaderBytecode = mVsBlob->GetBufferPointer();
		desc.VS.BytecodeLength = mVsBlob->GetBufferSize();
		// Pixel Shader
		desc.PS.pShaderBytecode = mPsBlob->GetBufferPointer();
		desc.PS.BytecodeLength = mPsBlob->GetBufferSize();
		
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.RasterizerState.MultisampleEnable = false; // アンチエイリアシングは行わないのでfalse
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 背面カリング ... 見えない背面は描画しない手法
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 中身を塗りつぶす
		desc.RasterizerState.DepthClipEnable = true; // 深度方向のクリッピング

		desc.BlendState.AlphaToCoverageEnable = false; // αテストの有無を表す
		desc.BlendState.IndependentBlendEnable = false;
	}
};

// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// メイン関数
#ifdef _DEBUG
int main()
#else
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
#endif
{
	OutputDebugString(_T("Hello DirectX World!!\n"));

	Gamen gamen;
	HWND hwnd = gamen.SetupWindow(_T("DirectX ちゃぷたー4"), (WNDPROC)WindowProcedure);
	gamen.SetUpDirectX();

	Sankaku sank;
	sank.MakeVertBuff(gamen.m_dev);
	sank.Map();
	
	ShowWindow(hwnd, SW_SHOW);

	// ゲームループ
	MSG msg = {};
	clock_t pretime = clock();
	int count = 0;
	float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) { // アプリ終了
			break;
		}

		clearColor[0] += (float)(rand() % 11 - 5) / 500.0f;
		clearColor[1] += (float)(rand() % 11 - 5) / 500.0f;
		clearColor[2] += (float)(rand() % 11 - 5) / 500.0f;
		gamen.Proc(clearColor);

		double t = (double)(clock() - pretime) / CLOCKS_PER_SEC;
		pretime = clock();
		if (count % 10 == 0) {
			DebugString(_T("%.2f sec (%.2f FPS)    \r"), t, 1.0 / t);
		}
	}
	DebugString(_T("\n"));

	DebugString(_T("END.\n"));

	return 0;
}
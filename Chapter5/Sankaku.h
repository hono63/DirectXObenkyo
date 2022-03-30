#pragma once

#include <iostream>
#include <vector>
#include <windows.h>
#include <crtdbg.h> // _ASSERT()マクロ
#include <tchar.h> // _T() 定義

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

using namespace DirectX;
using namespace std;

struct vertex_t {
	XMFLOAT3 pos; // xyz
	XMFLOAT2 uv; // texture;
};

class Sankaku {
private:
	//XMFLOAT3 mVertices[3];
	vertex_t mVertices[4];
	ID3D12Resource* mVertBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};
	ID3DBlob* mVsBlob = nullptr; // 頂点シェーダのBlob (Binary Large Object)
	ID3DBlob* mPsBlob = nullptr; // ピクセルシェーダのBlob
	ID3DBlob* mRootSigBlob = nullptr; // ルートシグネチャのBlob
	ID3D12RootSignature* mRootSig = nullptr;
	ID3D12PipelineState* mPipe = nullptr; // グラフィックス パイプライン ステート
	D3D12_INPUT_ELEMENT_DESC mVsLayouts[2] = {}; // 頂点入力レイアウト

public:
	Sankaku() {
		// 頂点の順序は時計回りになるように。
#if 0
		mVertices[0] = XMFLOAT3{ -0.5f, -0.7f, 0.f }; // 左下
		mVertices[1] = XMFLOAT3{ 0.f,  0.9f, 0.f }; // 左上
		mVertices[2] = XMFLOAT3{ 0.8f, -0.5f, 0.f }; // 右下
#else
		// 四角
		mVertices[0].pos = XMFLOAT3{ -0.7f, -0.7f, 0.f };
		mVertices[1].pos = XMFLOAT3{ -0.7f, +0.7f, 0.f };
		mVertices[2].pos = XMFLOAT3{ +0.7f, +0.7f, 0.f };
		mVertices[3].pos = XMFLOAT3{ +0.7f, -0.7f, 0.f };
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
		Map(mVertBuff, mVertices, sizeof(mVertices));
	}
	void Map(ID3D12Resource* resource, void* data, int datasize)
	{
		unsigned char* mapadr = nullptr;
		resource->Map(
			0, // サブリソース。ミニマップなどないため0。
			nullptr, // 範囲指定なし。
			(void**)&mapadr
		);
		memcpy(mapadr, data, datasize);
		resource->Unmap(0, nullptr); // もうマップを解除してええで
//		memcpy(mapadr, mVertices, sizeof(mVertices));
//		mVertBuff->Unmap(0, nullptr); // もうマップを解除してええで
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
		unsigned short indices[] = { // 四角形用インデックス
			0, 1, 2,
			0, 2, 3,
		};
		ID3D12Resource* idxBuff = nullptr;
		MakeBuffResource(_dev, sizeof(indices), &idxBuff);
		Map(idxBuff, indices, sizeof(indices));
		mIbView.BufferLocation = idxBuff->GetGPUVirtualAddress();
		mIbView.SizeInBytes = sizeof(indices);
		mIbView.Format = DXGI_FORMAT_R16_UINT; // unsinged short型
	}

	/// <summary>
	/// 頂点シェーダーのコンパイル
	/// </summary>
	void CompileVS()
	{
		Compile(_T("BasicVertexShader.hlsl"), "BasicVS", "vs_5_0", &mVsBlob);
	}
	/// <summary>
	/// ピクセルシェーダーのコンパイル
	/// </summary>
	void CompilePS()
	{
		Compile(_T("BasicPixelShader.hlsl"), "BasicPS", "ps_5_0", &mPsBlob);
	}

	void Compile(const TCHAR* filename, const char* entrypoint, const char* shaderver, ID3DBlob** blob)
	{
		ID3DBlob* errorBlob = nullptr; // エラーメッセージ用

		HRESULT result = D3DCompileFromFile(
			filename,
			nullptr, // シェーダー用マクロ
			D3D_COMPILE_STANDARD_FILE_INCLUDE, // includeするもの
			entrypoint,
			shaderver,
			(D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION), // Compile Option
			0, // シェーダーファイルの場合0にすることが推奨
			blob,
			&errorBlob
		);
		if (result != S_OK) {
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				MessageBox(NULL, _T("ファイルが見当たらないらしい"), _T("Error"), MB_OK);
			}
			char err[1000] = { 0 };
			//.resize(mErrorBlob->GetBufferSize());
			//std::copy_n(mErrorBlob->GetBufferPointer(), mErrorBlob->GetBufferSize(), err.data());
			memcpy_s(err, 999, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
			OutputDebugStringA(err);
			MessageBoxA(NULL, err, "Shader Compile Error", MB_OK);
			exit(1);
		}
	}

	/// <summary>
	/// 頂点レイアウト構造体の作成
	/// </summary>
	void LayoutVS()
	{
		// 座標情報
		mVsLayouts[0].SemanticName = "POSITION"; // 座標であることを意味する
		mVsLayouts[0].SemanticIndex = 0; // 同じセマンティクス名のときは使用する
		mVsLayouts[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // 32bit x 3 のデータ
		mVsLayouts[0].InputSlot = 0; // 複数の頂点データを合わせて１つの頂点データを表現したいときに使用する
		mVsLayouts[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // そのデータの場所. 今回は次から次にデータが並んでいることを表す。
		mVsLayouts[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // データの内容として、1頂点ごとにこのレイアウトが入っている。
		mVsLayouts[0].InstanceDataStepRate = 0; // インスタンシングのとき、１度に描画するインスタンスの数を指定する。

		// テクスチャ uv情報
		mVsLayouts[1].SemanticName = "TEXCORD"; // テクスチャであることを意味する
		mVsLayouts[1].SemanticIndex = 0;
		mVsLayouts[1].Format = DXGI_FORMAT_R32G32_FLOAT; // 32bit x 2 のデータ
		mVsLayouts[1].InputSlot = 0; 
		mVsLayouts[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; 
		mVsLayouts[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; 
		mVsLayouts[1].InstanceDataStepRate = 0;
	}

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	/// <param name="_dev"></param>
	void MakeRootSignature(ID3D12Device* _dev)
	{
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力頂点情報があるよ。ということ

		ID3DBlob* errorBlob = nullptr; // エラーメッセージ用
		HRESULT result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &mRootSigBlob, &errorBlob);
		if (result != S_OK) {
			char err[1000] = { 0 };
			memcpy_s(err, 999, errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
			OutputDebugStringA(err);
			MessageBoxA(NULL, err, "Make Root Signature Error", MB_OK);
			exit(1);
		}

		result = _dev->CreateRootSignature(
			0, // nodemask
			mRootSigBlob->GetBufferPointer(),
			mRootSigBlob->GetBufferSize(),
			IID_PPV_ARGS(&mRootSig)
		);
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// グラフィックスパイプラインステートを作成する
	/// </summary>
	/// <param name="_dev"></param>
	void MakePipeline(ID3D12Device* _dev)
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
		desc.BlendState.IndependentBlendEnable = false; // レンダーターゲットそれぞれ独立に設定するか
		desc.BlendState.RenderTarget[0].BlendEnable = false; // ブレンドするか
		desc.BlendState.RenderTarget[0].LogicOpEnable = false; // 論理演算するか
		desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;  // すべての要素をブレンド

		desc.InputLayout.pInputElementDescs = mVsLayouts;
		desc.InputLayout.NumElements = 2;

		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // トライアングルストリップのときに「切り離せない頂点集合」を特定のインデックスで切り離すための指定。使う場面はほぼない。
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 構成要素は三角形である。

		desc.NumRenderTargets = 1; // レンダーターゲットは1つ
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0~1に正規化されたRGB

		desc.SampleDesc.Count = 1; // アンチエイリアシングのサンプリング数
		desc.SampleDesc.Quality = 0; // 最低品質

		desc.pRootSignature = mRootSig;


		HRESULT result = _dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPipe));
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// ビューポートとシザー矩形の設定
	/// </summary>
	/// <param name="_cmdList"></param>
	void ViewPort(ID3D12GraphicsCommandList* _cmdList)
	{
		D3D12_VIEWPORT viewport = {};
		viewport.Width = Gamen::WINDOW_WIDTH;
		viewport.Height = Gamen::WINDOW_HEIGHT;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MaxDepth = 1.f;
		viewport.MinDepth = 0.f;
		_cmdList->RSSetViewports(1, &viewport);

		//シザー矩形
		D3D12_RECT scissor = {};
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = scissor.left + Gamen::WINDOW_WIDTH;
		scissor.bottom = scissor.top + Gamen::WINDOW_HEIGHT;
		_cmdList->RSSetScissorRects(1, &scissor);
	}

	/// <summary>
	/// 三角形の描画
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->SetPipelineState(mPipe);
		_cmdList->SetGraphicsRootSignature(mRootSig);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トライアングルリスト
		//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // トライアングル ストリップ

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


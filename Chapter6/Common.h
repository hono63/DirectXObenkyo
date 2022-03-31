#pragma once

#include <iostream>
#include <vector>
#include <windows.h>
#include <crtdbg.h> // _ASSERT()マクロ
#include <tchar.h> // _T() 定義
#include <stdlib.h>

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "d3dx12.h"
#include "DirectXTex.h"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

class CCommon {
private:
	ID3DBlob* mRootSigBlob = nullptr; // ルートシグネチャのBlob
	ID3DBlob* mVsBlob = nullptr; // 頂点シェーダのBlob (Binary Large Object)
	ID3DBlob* mPsBlob = nullptr; // ピクセルシェーダのBlob
	D3D12_INPUT_ELEMENT_DESC mVsLayouts[2] = {}; // 頂点入力レイアウト
public:
	ID3D12RootSignature* mRootSig = nullptr;
	ID3D12PipelineState* mPipe = nullptr; // グラフィックス パイプライン ステート
	ID3D12DescriptorHeap* mDescHeap = nullptr;

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
	}

	/// <summary>
	/// テクスチャ用シェーダーリソースビュー & 定数バッファビュー を作る
	/// </summary>
	/// <param name="_dev"></param>
	void MakeDescriptors(ID3D12Device* _dev, ID3D12Resource* texbuff, ID3D12Resource* constbuff)
	{
		// まずディスクリプターヒープを作る
		D3D12_DESCRIPTOR_HEAP_DESC hpdesc = {};

		hpdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shaderから見えるように
		hpdesc.NodeMask = 0;
		hpdesc.NumDescriptors = 2;
		hpdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // シェーダーリソースビュー用

		HRESULT result = _dev->CreateDescriptorHeap(&hpdesc, IID_PPV_ARGS(&mDescHeap));
		_ASSERT(result == S_OK);

		// シェーダーリソースビューを作る
		D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
		//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvdesc.Format = texbuff->GetDesc().Format;
		srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBAをどのようにマッピングするか指定
		srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
		srvdesc.Texture2D.MipLevels = 1;

		auto heapHandle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
		_dev->CreateShaderResourceView(texbuff, &srvdesc, heapHandle);

		// 定数バッファビューを作る
		heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
		cbvdesc.BufferLocation = constbuff->GetGPUVirtualAddress();
		cbvdesc.SizeInBytes = constbuff->GetDesc().Width;
		_dev->CreateConstantBufferView(&cbvdesc, heapHandle);
	}



	/// <summary>
	/// 頂点シェーダーのコンパイル
	/// </summary>
	void CompileVS()
	{
		Compile(_T("BasicVS.hlsl"), "BasicVS", "vs_5_0", &mVsBlob);
	}

	/// <summary>
	/// ピクセルシェーダーのコンパイル
	/// </summary>
	void CompilePS()
	{
		Compile(_T("BasicPS.hlsl"), "BasicPS", "ps_5_0", &mPsBlob);
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
	/// 頂点レイアウト・テクスチャuv座標レイアウト構造体の作成
	/// </summary>
	void MakeLayout()
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
		D3D12_ROOT_SIGNATURE_DESC rdesc = {};
		rdesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力頂点情報があるよ。ということ

		// ディスクリプターレンジ ... 同種のディスクリプターがヒープ上で複数どう並んでいるか指定
		D3D12_DESCRIPTOR_RANGE drange[2] = {};
		drange[0].NumDescriptors = 1;
		drange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // Shader Resource View
		drange[0].BaseShaderRegister = 0; // t0レジスタ
		drange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 並んでる
		drange[1].NumDescriptors = 1;
		drange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // Constant Buffer View
		drange[1].BaseShaderRegister = 0; // b0レジスタ。レジスター番号は種類が異なれば別のレジスターになる。
		drange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 並んでる

		// ルートパラメータ（=ディスクリプタテーブル。ディスクリプタヒープとシェーダーレジスタを紐付ける。）の作成
		D3D12_ROOT_PARAMETER rparam[1] = {};
		rparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ピクセルシェーダー・頂点シェーダどちらからも見える
		rparam[0].DescriptorTable.pDescriptorRanges = drange;
		rparam[0].DescriptorTable.NumDescriptorRanges = 2;

		rdesc.pParameters = rparam;
		rdesc.NumParameters = 1;

		// サンプラー（uv値によってテクスチャデータからどう色を取り出すか）の設定
		D3D12_STATIC_SAMPLER_DESC sdesc = {};
		sdesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // ボーダーは黒
		sdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間
		//sdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // 最近傍
		sdesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
		sdesc.MinLOD = 0.0f; // ミップマップ最小値
		sdesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		sdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // リサンプリングしない

		rdesc.pStaticSamplers = &sdesc;
		rdesc.NumStaticSamplers = 1;

		// ルートシグネチャ作成
		ID3DBlob* errorBlob = nullptr; // エラーメッセージ用
		HRESULT result = D3D12SerializeRootSignature(&rdesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &mRootSigBlob, &errorBlob);
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
		viewport.Width = WINDOW_WIDTH;
		viewport.Height = WINDOW_HEIGHT;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MaxDepth = 1.f;
		viewport.MinDepth = 0.f;
		_cmdList->RSSetViewports(1, &viewport);

		//シザー矩形
		D3D12_RECT scissor = {};
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = scissor.left + WINDOW_WIDTH;
		scissor.bottom = scissor.top + WINDOW_HEIGHT;
		_cmdList->RSSetScissorRects(1, &scissor);
	}

	/// <summary>
	/// パイプライン、ルートシグネチャなどをセット
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12Device* _dev, ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->SetPipelineState(mPipe);
		_cmdList->SetGraphicsRootSignature(mRootSig);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トライアングルリスト
		//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // トライアングル ストリップ

		// SRVとCBVを同一ルートパラメータとしている
		_cmdList->SetDescriptorHeaps(1, &mDescHeap);
		auto hHeap = mDescHeap->GetGPUDescriptorHandleForHeapStart();
		_cmdList->SetGraphicsRootDescriptorTable(0, hHeap); // ルートパラメータのインデックスとディスクリプターヒープのアドレスを関連付け。
#if 0
		// 定数バッファ
		hHeap.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_cmdList->SetGraphicsRootDescriptorTable(1, hHeap);
#endif
	}
};

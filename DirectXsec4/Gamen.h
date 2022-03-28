#pragma once

#include <iostream>
#include <vector>
#include <windows.h>
#include <crtdbg.h> // _ASSERT()マクロ
#include <tchar.h> // _T() 定義

#include <d3d12.h>
#include <dxgi1_6.h>

using namespace std;

class Gamen
{
private:
	HWND m_hWnd = NULL;
	ID3D12Device* m_dev = nullptr;
	IDXGIFactory6* m_dxgiFactory = nullptr;
	ID3D12CommandAllocator* m_cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* m_cmdList = nullptr;
	ID3D12CommandQueue* m_cmdQueue = nullptr;
	IDXGISwapChain4* m_swapchain = nullptr;
	ID3D12DescriptorHeap* m_rtvHeaps = nullptr;
	vector<ID3D12Resource*> m_backBuffers;
	UINT DESC_HEAP_RTV_SIZE = 0u;

public:
	const int WINDOW_WIDTH = 640;
	const int WINDOW_HEIGHT = 480;
	const int BUFFER_NUM = 2;

	Gamen() {
		m_backBuffers.resize(2);
	}

	/// <summary>
	/// ウィンドウの生成と表示
	/// </summary>
	/// <param name="w"></param>
	HWND SetupWindow(WNDCLASSEX& w, WNDPROC WindowProcedure, LPCTSTR title)
	{
		w.cbSize = sizeof(WNDCLASSEX);
		w.lpfnWndProc = (WNDPROC)WindowProcedure; // callback関数
		w.lpszClassName = _T("DX12Sample"); // アプリクラス名
		w.hInstance = GetModuleHandle(nullptr); // ハンドル取得

		RegisterClassEx(&w);

		RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false); // ウィンドウのサイズを補正

		m_hWnd = CreateWindow(w.lpszClassName,
			title, // タイトルバー
			WS_OVERLAPPEDWINDOW, // タイトルバーと境界線があるウィンドウ
			CW_USEDEFAULT, // X座標 おまかせ
			CW_USEDEFAULT, // Y座標 おまかせ
			wrc.right - wrc.left,
			wrc.bottom - wrc.top,
			nullptr,
			nullptr,
			w.hInstance, // 呼び出しアプリケーションハンドル
			nullptr
		);

		return m_hWnd;
	}

	/// <summary>
	/// DirectXのセットアップ
	/// </summary>
	void SetUpDirectX() {
		HRESULT result;
		// デバイスオブジェクトの生成
		result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_dev));
		_ASSERT(result == S_OK);
#ifdef _DEBUG
		result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_dxgiFactory)); // DXGIのエラーメッセージも出力する
#else
		result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
		_ASSERT(result == S_OK);
		GetGpuAdapter(m_dxgiFactory);

		// コマンドリスト・コマンドアロケーター作成
		result = m_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));
		_ASSERT(result == S_OK);
		result = m_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator, nullptr, IID_PPV_ARGS(&m_cmdList));
		_ASSERT(result == S_OK);

		// コマンドキュー作成
		m_cmdQueue = MakeCmdQ(m_dev);
		// スワップチェーンの作成
		m_swapchain = MakeSwapChain(m_cmdQueue, m_dxgiFactory, m_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT);
		// でぃすくりぷたヒープの作成
		m_rtvHeaps = MakeDescHeap(m_dev);

		// DescHeapとSwapChainのメモリ紐付け
		DESC_HEAP_RTV_SIZE = m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (int idx = 0; idx < 2; idx++) {
			HRESULT result = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])); // バックバッファーの取得
			_ASSERT(result == S_OK);
			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // ヒープ上の（最初の）ディスクリプタのハンドル
			handle.ptr += idx * DESC_HEAP_RTV_SIZE;
			m_dev->CreateRenderTargetView(m_backBuffers[idx], nullptr, handle);
		}
	}

	/// <summary>
	/// 画面更新実行
	/// </summary>
	void Proc(float clearColor[4])
	{
		// 現在のバックバッファ
		auto bbIdx = m_swapchain->GetCurrentBackBufferIndex();
		// リソースバリア(排他制御) Present -> RenderTarget
		SetResourceBarrier(bbIdx, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		// レンダーターゲット指定
		auto rtvH = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * DESC_HEAP_RTV_SIZE;
		m_cmdList->OMSetRenderTargets(
			1, // レンダーターゲット数
			&rtvH, // レンダーターゲットハンドル
			true, // レンダーターゲットが複数時に連続しているか
			nullptr // 深度ステンシルバッファービューのハンドル
		);
		// 画面クリア
		m_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		// リソースバリア RenderTarget -> Present
		SetResourceBarrier(bbIdx, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		// コマンドリスト実行前にCloseを必ず実行する
		m_cmdList->Close();
		ID3D12CommandList* cmdlists[] = { m_cmdList };

		m_cmdQueue->ExecuteCommandLists(1, cmdlists);

		WaitFence(m_dev, m_cmdQueue);
		// コマンドリスト開放
		m_cmdAllocator->Reset();
		m_cmdList->Reset(m_cmdAllocator, nullptr);

		// フリップ
		m_swapchain->Present(/*フリップまでの待ちフレーム数*/1, 0);
	}

protected:
	/// <summary>
/// デバッグレイヤーの有効化
/// </summary>
	void EnableDebugLayer()
	{
#ifdef _DEBUG
		ID3D12Debug* debugL = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugL));
		_ASSERT(result == S_OK);
		debugL->EnableDebugLayer();
		debugL->Release(); // 有効化後、解放する
#endif
	}

	/// <summary>
	/// 使用可能なGPUアダプターを表示してみる
	/// </summary>
	/// <param name="factory"></param>
	void GetGpuAdapter(IDXGIFactory6* factory)
	{
		vector <IDXGIAdapter*> adapters;
		IDXGIAdapter* tmp;
		for (int i = 0; factory->EnumAdapters(i, &tmp) != DXGI_ERROR_NOT_FOUND; i++) {
			adapters.push_back(tmp);
		}
		for (auto adpt : adapters) {
			DXGI_ADAPTER_DESC desc = {};
			adpt->GetDesc(&desc);
			wstring strDesc = desc.Description;

			//DebugString("Adapter[%d]:%s\n", i, strDesc.c_str());
#ifdef _DEBUG
			wcout << strDesc << endl;
#endif
		}
	}

	/// <summary>
	/// コマンドキュー作成
	/// </summary>
	/// <param name="_dev"></param>
	ID3D12CommandQueue* MakeCmdQ(ID3D12Device* _dev)
	{
		ID3D12CommandQueue* _cmdQueue = nullptr;
		D3D12_COMMAND_QUEUE_DESC desc;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // タイムアウトなし
		desc.NodeMask = 0; // アダプターが1つのときは0でよし
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // コマンドリストと合わせる
		HRESULT result = _dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmdQueue));
		_ASSERT(result == S_OK);

		return _cmdQueue;
	}

	/// <summary>
	/// スワップチェーンの作成
	/// </summary>
	/// <param name="_cmdQueue"></param>
	/// <param name="factory"></param>
	/// <param name="hwnd"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <returns></returns>
	IDXGISwapChain4* MakeSwapChain(ID3D12CommandQueue* _cmdQueue, IDXGIFactory6* factory, HWND hwnd, int width, int height)
	{
		IDXGISwapChain4* _swapchain = nullptr;
		DXGI_SWAP_CHAIN_DESC1 desc = {};

		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ピクセルフォーマット
		desc.Stereo = false; // 3Dディスプレイのステレオモード
		desc.SampleDesc.Count = 1; //マルチサンプルの指定
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
		desc.BufferCount = 2; // ダブルバッファー
		desc.Scaling = DXGI_SCALING_STRETCH; // 伸び縮み可能
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後、すみやかに爆発四散
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // 特に指定なし
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // ウィンドウ⇔フルスクリーン 切替可能

		HRESULT result = factory->CreateSwapChainForHwnd(
			_cmdQueue,
			hwnd,
			&desc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)&_swapchain
		);
		_ASSERT(result == S_OK);

		return _swapchain;
	}

	/// <summary>
	/// ディスクリプタ・ヒープを作成する
	/// </summary>
	/// <param name="_dev"></param>
	/// <returns></returns>
	ID3D12DescriptorHeap* MakeDescHeap(ID3D12Device* _dev)
	{
		ID3D12DescriptorHeap* rtvHeaps = nullptr;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー(RTV)
		desc.NodeMask = 0;
		desc.NumDescriptors = 2; // 表・裏
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // この「ビューに当たる情報」をシェーダー側から参照する必要があるかどうかを指定する。特に指定なし

		HRESULT result = _dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeaps));
		_ASSERT(result == S_OK);

		return rtvHeaps;
	}

	/// <summary>
	/// リソースバリア指定実行
	/// </summary>
	/// <param name="bbIdx">バックバッファーのインデックス</param>
	/// <param name=""></param>
	/// <param name=""></param>
	void SetResourceBarrier(UINT bbIdx, enum D3D12_RESOURCE_STATES before, enum D3D12_RESOURCE_STATES after)
	{
		D3D12_RESOURCE_BARRIER desc = {};

		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		desc.Transition.pResource = m_backBuffers[bbIdx]; // バックバッファーリソース
		desc.Transition.Subresource = 0;
		// before -> after 遷移
		desc.Transition.StateBefore = before;
		desc.Transition.StateAfter = after;

		m_cmdList->ResourceBarrier(1, &desc); // バリア指定実行
	}

	/// <summary>
	/// フェンスを作成する。
	/// ※フェンスはGPU側の処理が完了したかどうかを知るための仕組み。待つための仕組みではない。
	/// </summary>
	/// <param name="_dev"></param>
	/// <returns></returns>
	ID3D12Fence* MakeFence(ID3D12Device* _dev)
	{
		ID3D12Fence* _fence = nullptr;
		UINT64 _fenceVal = 0;
		HRESULT result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		_ASSERT(result == S_OK);

		return _fence;
	}

	/// <summary>
	/// GPU側の処理が終わるのを待つ。
	/// </summary>
	/// <param name="_dev"></param>
	/// <param name="_cmdQueue"></param>
	void WaitFence(ID3D12Device* _dev, ID3D12CommandQueue* _cmdQueue)
	{
		ID3D12Fence* _fence = MakeFence(_dev);
		UINT64 _fenceVal = 0;
		_cmdQueue->Signal(_fence, ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal) {
			auto hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			_ASSERT(hEvent);
			_fence->SetEventOnCompletion(_fenceVal, hEvent);
			DWORD ret = WaitForSingleObject(hEvent, INFINITE);
			_ASSERT(ret == WAIT_OBJECT_0);
			CloseHandle(hEvent);
		}
	}

};


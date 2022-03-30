
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <tchar.h> // _T() 定義
#include <stdlib.h>
#include <time.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

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

#define ASSERT(x) ASSERT__((x), __FILE__, __LINE__) 

void ASSERT__(bool x, const char* file, int line)
{ 
	if (!(x)) { 
		MessageBox(NULL, _T("!ASSERT!"), _T("ASSERT"), MB_OK); 
		exit(1); 
	}
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

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
/// <summary>
/// ウィンドウの生成と表示
/// </summary>
/// <param name="w"></param>
HWND SetupWindow(WNDCLASSEX& w)
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // callback関数
	w.lpszClassName = _T("DX12Sample"); // アプリクラス名
	w.hInstance = GetModuleHandle(nullptr); // ハンドル取得

	RegisterClassEx(&w);

	LONG window_width = WINDOW_WIDTH, window_height = WINDOW_HEIGHT;
	RECT wrc = { 0, 0, window_width, window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false); // ウィンドウのサイズを補正

	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12テスト"), // タイトルバー
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

	return hwnd;
}

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
/// <param name="_cmdList"></param>
/// <param name="backBuffer"></param>
/// <param name=""></param>
/// <param name=""></param>
void SetResourceBarrier(ID3D12GraphicsCommandList* _cmdList, ID3D12Resource* backBuffer, enum D3D12_RESOURCE_STATES before, enum D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER desc = {};

	desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	desc.Transition.pResource = backBuffer; // バックバッファーリソース
	desc.Transition.Subresource = 0;
	// before -> after 遷移
	desc.Transition.StateBefore = before;
	desc.Transition.StateAfter = after;

	_cmdList->ResourceBarrier(1, &desc); // バリア指定実行
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



// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// メイン関数
#ifdef _DEBUG
int main()
#else
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
#endif
{
	OutputDebugString(_T("Hello DirectX World!!\n"));

	WNDCLASSEX w = {}; // ウィンドウクラスの生成
	HWND hwnd = SetupWindow(w);

	EnableDebugLayer();

	HRESULT result;
	// デバイスオブジェクトの生成
	ID3D12Device* _dev = nullptr;
	result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dev));
	_ASSERT(result == S_OK);
	IDXGIFactory6* _dxgiFactory = nullptr;
#ifdef _DEBUG
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory)); // DXGIのエラーメッセージも出力する
#else
	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
	_ASSERT(result == S_OK);
	GetGpuAdapter(_dxgiFactory);

	// コマンドリスト・コマンドアロケーター作成
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	_ASSERT(result == S_OK);
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	_ASSERT(result == S_OK);
	// コマンドキュー作成
	ID3D12CommandQueue* _cmdQueue = MakeCmdQ(_dev);
	// スワップチェーンの作成
	IDXGISwapChain4* _swapchain = MakeSwapChain(_cmdQueue, _dxgiFactory, hwnd, WINDOW_WIDTH, WINDOW_HEIGHT);
	// でぃすくりぷたヒープの作成
	ID3D12DescriptorHeap* rtvHeaps = MakeDescHeap(_dev);
	// DescHeapとSwapChainのメモリ紐付け
	vector<ID3D12Resource*> _backBuffers(2);
	const UINT DESC_HEAP_RTV_SIZE = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int idx = 0; idx < 2; idx++) {
		HRESULT result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])); // バックバッファーの取得
		_ASSERT(result == S_OK);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // ヒープ上の（最初の）ディスクリプタのハンドル
		handle.ptr += idx * DESC_HEAP_RTV_SIZE;
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}

	
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

		// 現在のバックバッファ
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
		// リソースバリア
		SetResourceBarrier(_cmdList, _backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		// レンダーターゲット指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * DESC_HEAP_RTV_SIZE;
		_cmdList->OMSetRenderTargets(
			1, // レンダーターゲット数
			&rtvH, // レンダーターゲットハンドル
			true, // レンダーターゲットが複数時に連続しているか
			nullptr // 深度ステンシルバッファービューのハンドル
		);
		// 画面クリア
		clearColor[0] += (float)(rand() % 11 - 5) / 500.0f;
		clearColor[1] += (float)(rand() % 11 - 5) / 500.0f;
		clearColor[2] += (float)(rand() % 11 - 5) / 500.0f;
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		// リソースバリア
		SetResourceBarrier(_cmdList, _backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		// コマンドリスト実行前にCloseを必ず実行する
		_cmdList->Close();
		ID3D12CommandList* cmdlists[] = { _cmdList };
		
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		
		WaitFence(_dev, _cmdQueue);
		// コマンドリスト開放
		_cmdAllocator->Reset();
		_cmdList->Reset(_cmdAllocator, nullptr);
		
		// フリップ
		_swapchain->Present(/*フリップまでの待ちフレーム数*/1, 0);

		double t = (double)(clock() - pretime) / CLOCKS_PER_SEC;
		pretime = clock();
		if (count % 10 == 0) {
			DebugString(_T("%.2f sec (%.2f FPS)    \r"), t, 1.0 / t);
		}
	}
	DebugString(_T("\n"));
	// クラスの登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);


	DebugString(_T("END.\n"));

	return 0;
}
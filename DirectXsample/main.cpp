
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <tchar.h> // _T() 定義

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
void DebugString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
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

	ShowWindow(hwnd, SW_SHOW);

	return hwnd;
}

/// <summary>
/// デバッグレイヤーの有効化
/// </summary>
void EnableDebugLayer()
{
	ID3D12Debug* debugL = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugL));
	_ASSERT(result == S_OK);
	debugL->EnableDebugLayer();
	debugL->Release(); // 有効化後、解放する
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

// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// メイン関数
#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	WNDCLASSEX w = {}; // ウィンドウクラスの生成
	HWND hwnd = SetupWindow(w);

	EnableDebugLayer();

	HRESULT result;
	// デバイスオブジェクトの生成
	ID3D12Device* _dev = nullptr;
	result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dev));
	_ASSERT(result == S_OK);
	IDXGIFactory6* _dxgiFactory = nullptr;
	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
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
	for (int idx = 0; idx < 2; idx++) {
		HRESULT result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])); // バックバッファーの取得
		_ASSERT(result == S_OK);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // ヒープ上の（最初の）ディスクリプタのハンドル
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}
	// コマンドリストクリア
	result = _cmdAllocator->Reset();

	auto bbIdx = _swapchain->GetCurrentBackBufferIndex(); // 現在のバックバッファ
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(
		1, // レンダーターゲット数
		&rtvH, // レンダーターゲットハンドル
		true, // レンダーターゲットが複数時に連続しているか
		nullptr // 深度ステンシルバッファービューのハンドル
	);

	// 画面クリア
	float clearColor[4] = { 1.0f, 1.0f, 0.0f, 1.0f }; // RGBA
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
	_cmdList->Close();
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	// コマンドリスト開放
	_cmdAllocator->Reset();
	_cmdList->Reset(_cmdAllocator, nullptr);
	// フリップ
	_swapchain->Present(/*フリップまでの待ちフレーム数*/1, 0);


	// ゲームループ
	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) { // アプリ終了
			break;
		}
	}

	// クラスの登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);


	DebugString("END.\n");

	return 0;
}
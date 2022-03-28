
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <tchar.h> // _T() ��`

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace std;

/// <summary>
/// �f�o�b�O�pprintf
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
/// ���܂��Ȃ�
/// </summary>
/// <param name="hwnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY) {
		PostQuitMessage(0); // OS�ɑ΂��Ă��̃A�v���̏I����`����B
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); //  ����̏������s��
}

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
/// <summary>
/// �E�B���h�E�̐����ƕ\��
/// </summary>
/// <param name="w"></param>
HWND SetupWindow(WNDCLASSEX& w)
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // callback�֐�
	w.lpszClassName = _T("DX12Sample"); // �A�v���N���X��
	w.hInstance = GetModuleHandle(nullptr); // �n���h���擾

	RegisterClassEx(&w);

	LONG window_width = WINDOW_WIDTH, window_height = WINDOW_HEIGHT;
	RECT wrc = { 0, 0, window_width, window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false); // �E�B���h�E�̃T�C�Y��␳

	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12�e�X�g"), // �^�C�g���o�[
		WS_OVERLAPPEDWINDOW, // �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT, // X���W ���܂���
		CW_USEDEFAULT, // Y���W ���܂���
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		w.hInstance, // �Ăяo���A�v���P�[�V�����n���h��
		nullptr
	);

	ShowWindow(hwnd, SW_SHOW);

	return hwnd;
}

/// <summary>
/// �f�o�b�O���C���[�̗L����
/// </summary>
void EnableDebugLayer()
{
	ID3D12Debug* debugL = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugL));
	_ASSERT(result == S_OK);
	debugL->EnableDebugLayer();
	debugL->Release(); // �L������A�������
}

/// <summary>
/// �g�p�\��GPU�A�_�v�^�[��\�����Ă݂�
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
/// �R�}���h�L���[�쐬
/// </summary>
/// <param name="_dev"></param>
ID3D12CommandQueue* MakeCmdQ(ID3D12Device* _dev)
{
	ID3D12CommandQueue* _cmdQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC desc;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // �^�C���A�E�g�Ȃ�
	desc.NodeMask = 0; // �A�_�v�^�[��1�̂Ƃ���0�ł悵
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // �R�}���h���X�g�ƍ��킹��
	HRESULT result = _dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmdQueue));
	_ASSERT(result == S_OK);

	return _cmdQueue;
}

/// <summary>
/// �X���b�v�`�F�[���̍쐬
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
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �s�N�Z���t�H�[�}�b�g
	desc.Stereo = false; // 3D�f�B�X�v���C�̃X�e���I���[�h
	desc.SampleDesc.Count = 1; //�}���`�T���v���̎w��
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	desc.BufferCount = 2; // �_�u���o�b�t�@�[
	desc.Scaling = DXGI_SCALING_STRETCH; // �L�яk�݉\
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �t���b�v��A���݂₩�ɔ����l�U
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // ���Ɏw��Ȃ�
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // �E�B���h�E�̃t���X�N���[�� �ؑ։\

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
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // �����_�[�^�[�Q�b�g�r���[(RTV)
	desc.NodeMask = 0;
	desc.NumDescriptors = 2; // �\�E��
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // ���́u�r���[�ɓ�������v���V�F�[�_�[������Q�Ƃ���K�v�����邩�ǂ������w�肷��B���Ɏw��Ȃ�

	HRESULT result = _dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeaps));
	_ASSERT(result == S_OK);

	return rtvHeaps;
}

// ��������������������������������������������������������������������
// ���C���֐�
#ifdef _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	WNDCLASSEX w = {}; // �E�B���h�E�N���X�̐���
	HWND hwnd = SetupWindow(w);

	EnableDebugLayer();

	HRESULT result;
	// �f�o�C�X�I�u�W�F�N�g�̐���
	ID3D12Device* _dev = nullptr;
	result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_dev));
	_ASSERT(result == S_OK);
	IDXGIFactory6* _dxgiFactory = nullptr;
	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	_ASSERT(result == S_OK);
	GetGpuAdapter(_dxgiFactory);

	// �R�}���h���X�g�E�R�}���h�A���P�[�^�[�쐬
	ID3D12CommandAllocator* _cmdAllocator = nullptr;
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	_ASSERT(result == S_OK);
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	_ASSERT(result == S_OK);
	// �R�}���h�L���[�쐬
	ID3D12CommandQueue* _cmdQueue = MakeCmdQ(_dev);
	// �X���b�v�`�F�[���̍쐬
	IDXGISwapChain4* _swapchain = MakeSwapChain(_cmdQueue, _dxgiFactory, hwnd, WINDOW_WIDTH, WINDOW_HEIGHT);
	// �ł�������Ղ��q�[�v�̍쐬
	ID3D12DescriptorHeap* rtvHeaps = MakeDescHeap(_dev);
	// DescHeap��SwapChain�̃������R�t��
	vector<ID3D12Resource*> _backBuffers(2);
	for (int idx = 0; idx < 2; idx++) {
		HRESULT result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])); // �o�b�N�o�b�t�@�[�̎擾
		_ASSERT(result == S_OK);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // �q�[�v��́i�ŏ��́j�f�B�X�N���v�^�̃n���h��
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}
	// �R�}���h���X�g�N���A
	result = _cmdAllocator->Reset();

	auto bbIdx = _swapchain->GetCurrentBackBufferIndex(); // ���݂̃o�b�N�o�b�t�@
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(
		1, // �����_�[�^�[�Q�b�g��
		&rtvH, // �����_�[�^�[�Q�b�g�n���h��
		true, // �����_�[�^�[�Q�b�g���������ɘA�����Ă��邩
		nullptr // �[�x�X�e���V���o�b�t�@�[�r���[�̃n���h��
	);

	// ��ʃN���A
	float clearColor[4] = { 1.0f, 1.0f, 0.0f, 1.0f }; // RGBA
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
	_cmdList->Close();
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	// �R�}���h���X�g�J��
	_cmdAllocator->Reset();
	_cmdList->Reset(_cmdAllocator, nullptr);
	// �t���b�v
	_swapchain->Present(/*�t���b�v�܂ł̑҂��t���[����*/1, 0);


	// �Q�[�����[�v
	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) { // �A�v���I��
			break;
		}
	}

	// �N���X�̓o�^����
	UnregisterClass(w.lpszClassName, w.hInstance);


	DebugString("END.\n");

	return 0;
}
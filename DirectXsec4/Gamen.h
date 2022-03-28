#pragma once

#include <iostream>
#include <vector>
#include <windows.h>
#include <crtdbg.h> // _ASSERT()�}�N��
#include <tchar.h> // _T() ��`

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
	/// �E�B���h�E�̐����ƕ\��
	/// </summary>
	/// <param name="w"></param>
	HWND SetupWindow(WNDCLASSEX& w, WNDPROC WindowProcedure, LPCTSTR title)
	{
		w.cbSize = sizeof(WNDCLASSEX);
		w.lpfnWndProc = (WNDPROC)WindowProcedure; // callback�֐�
		w.lpszClassName = _T("DX12Sample"); // �A�v���N���X��
		w.hInstance = GetModuleHandle(nullptr); // �n���h���擾

		RegisterClassEx(&w);

		RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false); // �E�B���h�E�̃T�C�Y��␳

		m_hWnd = CreateWindow(w.lpszClassName,
			title, // �^�C�g���o�[
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

		return m_hWnd;
	}

	/// <summary>
	/// DirectX�̃Z�b�g�A�b�v
	/// </summary>
	void SetUpDirectX() {
		HRESULT result;
		// �f�o�C�X�I�u�W�F�N�g�̐���
		result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_dev));
		_ASSERT(result == S_OK);
#ifdef _DEBUG
		result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_dxgiFactory)); // DXGI�̃G���[���b�Z�[�W���o�͂���
#else
		result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
		_ASSERT(result == S_OK);
		GetGpuAdapter(m_dxgiFactory);

		// �R�}���h���X�g�E�R�}���h�A���P�[�^�[�쐬
		result = m_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));
		_ASSERT(result == S_OK);
		result = m_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator, nullptr, IID_PPV_ARGS(&m_cmdList));
		_ASSERT(result == S_OK);

		// �R�}���h�L���[�쐬
		m_cmdQueue = MakeCmdQ(m_dev);
		// �X���b�v�`�F�[���̍쐬
		m_swapchain = MakeSwapChain(m_cmdQueue, m_dxgiFactory, m_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT);
		// �ł�������Ղ��q�[�v�̍쐬
		m_rtvHeaps = MakeDescHeap(m_dev);

		// DescHeap��SwapChain�̃������R�t��
		DESC_HEAP_RTV_SIZE = m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (int idx = 0; idx < 2; idx++) {
			HRESULT result = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(&m_backBuffers[idx])); // �o�b�N�o�b�t�@�[�̎擾
			_ASSERT(result == S_OK);
			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // �q�[�v��́i�ŏ��́j�f�B�X�N���v�^�̃n���h��
			handle.ptr += idx * DESC_HEAP_RTV_SIZE;
			m_dev->CreateRenderTargetView(m_backBuffers[idx], nullptr, handle);
		}
	}

	/// <summary>
	/// ��ʍX�V���s
	/// </summary>
	void Proc(float clearColor[4])
	{
		// ���݂̃o�b�N�o�b�t�@
		auto bbIdx = m_swapchain->GetCurrentBackBufferIndex();
		// ���\�[�X�o���A(�r������) Present -> RenderTarget
		SetResourceBarrier(bbIdx, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		// �����_�[�^�[�Q�b�g�w��
		auto rtvH = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * DESC_HEAP_RTV_SIZE;
		m_cmdList->OMSetRenderTargets(
			1, // �����_�[�^�[�Q�b�g��
			&rtvH, // �����_�[�^�[�Q�b�g�n���h��
			true, // �����_�[�^�[�Q�b�g���������ɘA�����Ă��邩
			nullptr // �[�x�X�e���V���o�b�t�@�[�r���[�̃n���h��
		);
		// ��ʃN���A
		m_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		// ���\�[�X�o���A RenderTarget -> Present
		SetResourceBarrier(bbIdx, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		// �R�}���h���X�g���s�O��Close��K�����s����
		m_cmdList->Close();
		ID3D12CommandList* cmdlists[] = { m_cmdList };

		m_cmdQueue->ExecuteCommandLists(1, cmdlists);

		WaitFence(m_dev, m_cmdQueue);
		// �R�}���h���X�g�J��
		m_cmdAllocator->Reset();
		m_cmdList->Reset(m_cmdAllocator, nullptr);

		// �t���b�v
		m_swapchain->Present(/*�t���b�v�܂ł̑҂��t���[����*/1, 0);
	}

protected:
	/// <summary>
/// �f�o�b�O���C���[�̗L����
/// </summary>
	void EnableDebugLayer()
	{
#ifdef _DEBUG
		ID3D12Debug* debugL = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugL));
		_ASSERT(result == S_OK);
		debugL->EnableDebugLayer();
		debugL->Release(); // �L������A�������
#endif
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

	/// <summary>
	/// �f�B�X�N���v�^�E�q�[�v���쐬����
	/// </summary>
	/// <param name="_dev"></param>
	/// <returns></returns>
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

	/// <summary>
	/// ���\�[�X�o���A�w����s
	/// </summary>
	/// <param name="bbIdx">�o�b�N�o�b�t�@�[�̃C���f�b�N�X</param>
	/// <param name=""></param>
	/// <param name=""></param>
	void SetResourceBarrier(UINT bbIdx, enum D3D12_RESOURCE_STATES before, enum D3D12_RESOURCE_STATES after)
	{
		D3D12_RESOURCE_BARRIER desc = {};

		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		desc.Transition.pResource = m_backBuffers[bbIdx]; // �o�b�N�o�b�t�@�[���\�[�X
		desc.Transition.Subresource = 0;
		// before -> after �J��
		desc.Transition.StateBefore = before;
		desc.Transition.StateAfter = after;

		m_cmdList->ResourceBarrier(1, &desc); // �o���A�w����s
	}

	/// <summary>
	/// �t�F���X���쐬����B
	/// ���t�F���X��GPU���̏����������������ǂ�����m�邽�߂̎d�g�݁B�҂��߂̎d�g�݂ł͂Ȃ��B
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
	/// GPU���̏������I���̂�҂B
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


#pragma once

#include <iostream>
#include <vector>
#include <windows.h>
#include <crtdbg.h> // _ASSERT()�}�N��
#include <tchar.h> // _T() ��`

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

using namespace DirectX;
using namespace std;


class Sankaku {
private:
	//XMFLOAT3 mVertices[3];
	XMFLOAT3 mVertices[4];
	ID3D12Resource* mVertBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVbView = {};
	D3D12_INDEX_BUFFER_VIEW mIbView = {};
	ID3DBlob* mVsBlob = nullptr; // ���_�V�F�[�_��Blob (Binary Large Object)
	ID3DBlob* mPsBlob = nullptr; // �s�N�Z���V�F�[�_��Blob
	ID3DBlob* mRootSigBlob = nullptr; // ���[�g�V�O�l�`����Blob
	ID3D12RootSignature* mRootSig = nullptr;
	ID3D12PipelineState* mPipe = nullptr; // �O���t�B�b�N�X �p�C�v���C�� �X�e�[�g
	D3D12_INPUT_ELEMENT_DESC mVsLayout = {}; // ���_���̓��C�A�E�g

public:
	Sankaku() {
		// ���_�̏����͔����v���ɂȂ�悤�ɁB
		//mVertices[0] = XMFLOAT3{ -1.f, -1.f, 0.f }; // ����
		//mVertices[1] = XMFLOAT3{ -1.f,  1.f, 0.f }; // ����
		//mVertices[2] = XMFLOAT3{ 1.f, -1.f, 0.f }; // �E��
#if 0
		mVertices[0] = XMFLOAT3{ -0.5f, -0.7f, 0.f }; // ����
		mVertices[1] = XMFLOAT3{ 0.f,  0.9f, 0.f }; // ����
		mVertices[2] = XMFLOAT3{ 0.8f, -0.5f, 0.f }; // �E��
#else
		// �l�p
		mVertices[0] = XMFLOAT3{ -0.7f, -0.7f, 0.f };
		mVertices[1] = XMFLOAT3{ -0.7f, +0.7f, 0.f };
		mVertices[2] = XMFLOAT3{ +0.7f, +0.7f, 0.f };
		mVertices[3] = XMFLOAT3{ +0.7f, -0.7f, 0.f };
		//mVertices[4] = mVertices[0];
#endif
	}

	/// <summary>
	/// ���_�o�b�t�@�̍쐬
	/// </summary>
	/// <param name="_dev"></param>
	void MakeVertBuff(ID3D12Device* _dev)
	{
		MakeBuffResource(_dev, sizeof(mVertices), &mVertBuff);
	}

	void MakeBuffResource(ID3D12Device* _dev, UINT64 buffsize, ID3D12Resource** resource)
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD; // �q�[�v��ʁBMap����Ȃ�UPLOAD�B���Ȃ��Ȃ�DEFAULT�B
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // CPU�y�[�W���O�ݒ�
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // �������v�[�����ǂ���
		prop.CreationNodeMask = 0; // �P��A�_�v�^�[�Ȃ�0
		prop.VisibleNodeMask = 0; // �P��A�_�v�^�[�Ȃ�0

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // �o�b�t�@�[�Ƃ��Ďg���̂ŁB
		desc.Alignment = 0;
		desc.Width = buffsize;
		desc.Height = 1; // Width�őS���܂��Ȃ�
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN; // �摜�ł͂Ȃ��̂�UNKNOWN�f���C
		desc.SampleDesc.Count = 1; // �A���`�G�C���A�V���O���s���Ƃ��̃p�����[�^�B�s��Ȃ��ꍇ1�ł悢�B
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // ���������ŏ�����Ō�܂ŘA�����Ă��邱�Ƃ�����ROW_MAJOR�B
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT result = _dev->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, // �ǂݎ���p
			nullptr,
			IID_PPV_ARGS(resource)
		);
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// ���_�o�b�t�@�̉��z�A�h���X�ɒ��_�f�[�^����������
	/// </summary>
	void Map()
	{
		XMFLOAT3* vertMap = nullptr;
		mVertBuff->Map(
			0, // �T�u���\�[�X�B�~�j�}�b�v�ȂǂȂ�����0�B
			nullptr, // �͈͎w��Ȃ��B
			(void**)&vertMap
		);
		std::copy(std::begin(mVertices), std::end(mVertices), vertMap); // mVertices -> vertMap
		mVertBuff->Unmap(0, nullptr); // �����}�b�v���������Ă�����
	}

	/// <summary>
	/// ���_�o�b�t�@�[�r���[�E���_�C���f�b�N�X�o�b�t�@�[�r���[�̍쐬
	/// </summary>
	void MakeView(ID3D12Device* _dev)
	{
		mVbView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
		mVbView.SizeInBytes = sizeof(mVertices); // �S�o�C�g��
		mVbView.StrideInBytes = sizeof(mVertices[0]); // 1���_�̃o�C�g��

		// Index Buffer View
		unsigned short indices[] = { // �l�p�`�p�C���f�b�N�X
			0, 1, 2,
			0, 2, 3,
		};
		ID3D12Resource* idxBuff = nullptr;
		MakeBuffResource(_dev, sizeof(indices), &idxBuff);
		unsigned short *mappedIdx = nullptr;
		idxBuff->Map(0, nullptr, (void**)&mappedIdx);
		std::copy(std::begin(indices), std::end(indices), mappedIdx); // indices -> mappedIdx
		idxBuff->Unmap(0, nullptr);
		mIbView.BufferLocation = idxBuff->GetGPUVirtualAddress();
		mIbView.SizeInBytes = sizeof(indices);
		mIbView.Format = DXGI_FORMAT_R16_UINT; // unsinged short�^
	}

	/// <summary>
	/// ���_�V�F�[�_�[�̃R���p�C��
	/// </summary>
	void CompileVS()
	{
		Compile(_T("BasicVertexShader.hlsl"), "BasicVS", "vs_5_0", &mVsBlob);
	}
	/// <summary>
	/// �s�N�Z���V�F�[�_�[�̃R���p�C��
	/// </summary>
	void CompilePS()
	{
		Compile(_T("BasicPixelShader.hlsl"), "BasicPS", "ps_5_0", &mPsBlob);
	}

	void Compile(const TCHAR* filename, const char* entrypoint, const char* shaderver, ID3DBlob** blob)
	{
		ID3DBlob* errorBlob = nullptr; // �G���[���b�Z�[�W�p

		HRESULT result = D3DCompileFromFile(
			filename,
			nullptr, // �V�F�[�_�[�p�}�N��
			D3D_COMPILE_STANDARD_FILE_INCLUDE, // include�������
			entrypoint,
			shaderver,
			(D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION), // Compile Option
			0, // �V�F�[�_�[�t�@�C���̏ꍇ0�ɂ��邱�Ƃ�����
			blob,
			&errorBlob
		);
		if (result != S_OK) {
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				MessageBox(NULL, _T("�t�@�C������������Ȃ��炵��"), _T("Error"), MB_OK);
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
	/// ���_���C�A�E�g�\���̂̍쐬
	/// </summary>
	void LayoutVS()
	{
		mVsLayout.SemanticName = "POSITION"; // ���W�ł��邱�Ƃ��Ӗ�����
		mVsLayout.SemanticIndex = 0; // �����Z�}���e�B�N�X���̂Ƃ��͎g�p����
		mVsLayout.Format = DXGI_FORMAT_R32G32B32_FLOAT; // 32bit x 3 �̃f�[�^
		mVsLayout.InputSlot = 0; // �����̒��_�f�[�^�����킹�ĂP�̒��_�f�[�^��\���������Ƃ��Ɏg�p����
		mVsLayout.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // ���̃f�[�^�̏ꏊ. ����͎����玟�Ƀf�[�^������ł��邱�Ƃ�\���B
		mVsLayout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // �f�[�^�̓��e�Ƃ��āA1���_���Ƃɂ��̃��C�A�E�g�������Ă���B
		mVsLayout.InstanceDataStepRate = 0; // �C���X�^���V���O�̂Ƃ��A�P�x�ɕ`�悷��C���X�^���X�̐����w�肷��B

	}

	/// <summary>
	/// ���[�g�V�O�l�`���̍쐬
	/// </summary>
	/// <param name="_dev"></param>
	void MakeRootSignature(ID3D12Device* _dev)
	{
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // ���͒��_��񂪂����B�Ƃ�������

		ID3DBlob* errorBlob = nullptr; // �G���[���b�Z�[�W�p
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
	/// �O���t�B�b�N�X�p�C�v���C���X�e�[�g���쐬����
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
		desc.RasterizerState.MultisampleEnable = false; // �A���`�G�C���A�V���O�͍s��Ȃ��̂�false
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �w�ʃJ�����O ... �����Ȃ��w�ʂ͕`�悵�Ȃ���@
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ���g��h��Ԃ�
		desc.RasterizerState.DepthClipEnable = true; // �[�x�����̃N���b�s���O

		desc.BlendState.AlphaToCoverageEnable = false; // ���e�X�g�̗L����\��
		desc.BlendState.IndependentBlendEnable = false; // �����_�[�^�[�Q�b�g���ꂼ��Ɨ��ɐݒ肷�邩
		desc.BlendState.RenderTarget[0].BlendEnable = false; // �u�����h���邩
		desc.BlendState.RenderTarget[0].LogicOpEnable = false; // �_�����Z���邩
		desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;  // ���ׂĂ̗v�f���u�����h

		desc.InputLayout.pInputElementDescs = &mVsLayout;
		desc.InputLayout.NumElements = 1;

		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �g���C�A���O���X�g���b�v�̂Ƃ��Ɂu�؂藣���Ȃ����_�W���v�����̃C���f�b�N�X�Ő؂藣�����߂̎w��B�g����ʂ͂قڂȂ��B
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �\���v�f�͎O�p�`�ł���B

		desc.NumRenderTargets = 1; // �����_�[�^�[�Q�b�g��1��
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0~1�ɐ��K�����ꂽRGB

		desc.SampleDesc.Count = 1; // �A���`�G�C���A�V���O�̃T���v�����O��
		desc.SampleDesc.Quality = 0; // �Œ�i��

		desc.pRootSignature = mRootSig;


		HRESULT result = _dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPipe));
		_ASSERT(result == S_OK);
	}

	/// <summary>
	/// �r���[�|�[�g�ƃV�U�[��`�̐ݒ�
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

		//�V�U�[��`
		D3D12_RECT scissor = {};
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = scissor.left + Gamen::WINDOW_WIDTH;
		scissor.bottom = scissor.top + Gamen::WINDOW_HEIGHT;
		_cmdList->RSSetScissorRects(1, &scissor);
	}

	/// <summary>
	/// �O�p�`�̕`��
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->SetPipelineState(mPipe);
		_cmdList->SetGraphicsRootSignature(mRootSig);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // �g���C�A���O�����X�g
		//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // �g���C�A���O�� �X�g���b�v

		_cmdList->IASetIndexBuffer(&mIbView);// ���_�C���f�b�N�X

		_cmdList->IASetVertexBuffers(
			0, // �X���b�g�ԍ�
			1, // ���_�o�b�t�@�[�r���[�̐�
			&mVbView
		);
#if 0
		_cmdList->DrawInstanced(
			//3, // ���_��
			4, // ���_��
			1, // �C���X�^���X��
			0, // ���_�f�[�^�̃I�t�Z�b�g
			0 // �C���X�^���X�̃I�t�Z�b�g
		);
#endif
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
};


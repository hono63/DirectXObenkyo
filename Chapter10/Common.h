#pragma once

#include <iostream>
#include <vector>
#include <windows.h>
#include <crtdbg.h> // _ASSERT()�}�N��
#include <tchar.h> // _T() ��`
#include <stdlib.h>
#include <stdint.h>

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
	ID3DBlob* mRootSigBlob = nullptr; // ���[�g�V�O�l�`����Blob
	ID3DBlob* mVsBlob = nullptr; // ���_�V�F�[�_��Blob (Binary Large Object)
	ID3DBlob* mPsBlob = nullptr; // �s�N�Z���V�F�[�_��Blob
public:
	D3D12_INPUT_ELEMENT_DESC mLayout[2] = {}; // ���_���̓��C�A�E�g
	ID3D12RootSignature* mRootSig = nullptr;
	ID3D12PipelineState* mPipe = nullptr; // �O���t�B�b�N�X �p�C�v���C�� �X�e�[�g
	ID3D12DescriptorHeap* mDescHeap = nullptr;

	static void MakeBuffResource(ID3D12Device* _dev, UINT64 buffsize, ID3D12Resource** resource)
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

	static void MakeBuffResource256(ID3D12Device* _dev, UINT64 buffsize, ID3D12Resource** resource)
	{
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer((buffsize + 0x000000ff) & ~0x000000ff); // 256byte�A���C�����g

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

	static void Map(ID3D12Resource* resource, void* data, int datasize)
	{
		unsigned char* mapadr = nullptr;
		resource->Map(
			0, // �T�u���\�[�X�B�~�j�}�b�v�ȂǂȂ�����0�B
			nullptr, // �͈͎w��Ȃ��B
			(void**)&mapadr
		);
		memcpy(mapadr, data, datasize);
		resource->Unmap(0, nullptr); // �����}�b�v���������Ă�����
	}

	/// <summary>
	/// �f�B�X�N���v�^�q�[�v��� �e�N�X�`���p�V�F�[�_�[���\�[�X�r���[ & �萔�o�b�t�@�r���[ �����
	/// </summary>
	/// <param name="_dev"></param>
	void MakeDescriptors(ID3D12Device* _dev, ID3D12Resource* texbuff, ID3D12Resource* constbuff, ID3D12Resource* constbuff2)
	{
		// �܂��f�B�X�N���v�^�[�q�[�v�����
		D3D12_DESCRIPTOR_HEAP_DESC hpdesc = {};

		hpdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shader���猩����悤��
		hpdesc.NodeMask = 0;
		hpdesc.NumDescriptors = 2;
		hpdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �V�F�[�_�[���\�[�X�r���[�p

		HRESULT result = _dev->CreateDescriptorHeap(&hpdesc, IID_PPV_ARGS(&mDescHeap));
		_ASSERT(result == S_OK);

		// �V�F�[�_�[���\�[�X�r���[�����
		D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
		//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvdesc.Format = texbuff->GetDesc().Format;
		srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBA���ǂ̂悤�Ƀ}�b�s���O���邩�w��
		srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
		srvdesc.Texture2D.MipLevels = 1;

		auto heapHandle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
		_dev->CreateShaderResourceView(texbuff, &srvdesc, heapHandle);

		// �萔�o�b�t�@�r���[�����
		heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
		cbvdesc.BufferLocation = constbuff->GetGPUVirtualAddress();
		cbvdesc.SizeInBytes = constbuff->GetDesc().Width;
		_dev->CreateConstantBufferView(&cbvdesc, heapHandle);

		// �萔�o�b�t�@�r���[����� part2
#if 0
		heapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc2 = {};
		cbvdesc2.BufferLocation = constbuff2->GetGPUVirtualAddress();
		cbvdesc2.SizeInBytes = constbuff2->GetDesc().Width;
		_dev->CreateConstantBufferView(&cbvdesc2, heapHandle);
#endif
	}



	/// <summary>
	/// ���_�V�F�[�_�[�̃R���p�C��
	/// </summary>
	void CompileVS()
	{
		Compile(_T("BasicVS.hlsl"), "BasicVS", "vs_5_0", &mVsBlob);
	}

	/// <summary>
	/// �s�N�Z���V�F�[�_�[�̃R���p�C��
	/// </summary>
	void CompilePS()
	{
		Compile(_T("BasicPS.hlsl"), "BasicPS", "ps_5_0", &mPsBlob);
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
	/// ���[�g�V�O�l�`���̍쐬
	/// </summary>
	/// <param name="_dev"></param>
	void MakeRootSignature(ID3D12Device* _dev)
	{
		D3D12_ROOT_SIGNATURE_DESC rdesc = {};
		rdesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // ���͒��_��񂪂����B�Ƃ�������

		// �f�B�X�N���v�^�[�����W ... ����̃f�B�X�N���v�^�[���q�[�v��ŕ����ǂ�����ł��邩�w��
		D3D12_DESCRIPTOR_RANGE drange[3] = {};
		drange[0].NumDescriptors = 1;
		drange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // Shader Resource View
		drange[0].BaseShaderRegister = 0; // t0���W�X�^
		drange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // ����ł�
		drange[1].NumDescriptors = 1;
		drange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // Constant Buffer View
		drange[1].BaseShaderRegister = 0; // b0���W�X�^�B���W�X�^�[�ԍ��͎�ނ��قȂ�Εʂ̃��W�X�^�[�ɂȂ�B
		drange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // ����ł�

		// ���[�g�p�����[�^�i=�f�B�X�N���v�^�e�[�u���B�f�B�X�N���v�^�q�[�v�ƃV�F�[�_�[���W�X�^��R�t����B�j�̍쐬
		D3D12_ROOT_PARAMETER rparam[2] = {};
		rparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // �s�N�Z���V�F�[�_�[�E���_�V�F�[�_�ǂ��炩���������
		rparam[0].DescriptorTable.pDescriptorRanges = drange;
		rparam[0].DescriptorTable.NumDescriptorRanges = 2;

		// �f�B�X�N���v�^�[�����W�E���[�g�p�����[�^ part2 bone�p
		drange[2].NumDescriptors = 1;
		drange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // Constant Buffer View
		drange[2].BaseShaderRegister = 1; // b1���W�X�^�B
		drange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // ����ł�
		rparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // �s�N�Z���V�F�[�_�[�E���_�V�F�[�_�ǂ��炩���������
		rparam[1].DescriptorTable.pDescriptorRanges = &drange[2];
		rparam[1].DescriptorTable.NumDescriptorRanges = 1;

		rdesc.pParameters = rparam;
		rdesc.NumParameters = 2;

		// �T���v���[�iuv�l�ɂ���ăe�N�X�`���f�[�^����ǂ��F�����o�����j�̐ݒ�
		D3D12_STATIC_SAMPLER_DESC sdesc = {};
		sdesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // �{�[�_�[�͍�
		sdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // ���`���
		//sdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // �ŋߖT
		sdesc.MaxLOD = D3D12_FLOAT32_MAX; // �~�b�v�}�b�v�ő�l
		sdesc.MinLOD = 0.0f; // �~�b�v�}�b�v�ŏ��l
		sdesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		sdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // ���T���v�����O���Ȃ�

		rdesc.pStaticSamplers = &sdesc;
		rdesc.NumStaticSamplers = 1;

		// ���[�g�V�O�l�`���쐬
		ID3DBlob* errorBlob = nullptr; // �G���[���b�Z�[�W�p
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
	/// �O���t�B�b�N�X�p�C�v���C���X�e�[�g���쐬����
	/// </summary>
	/// <param name="_dev"></param>
	void MakePipeline(ID3D12Device* _dev, D3D12_INPUT_ELEMENT_DESC* layouts, size_t layoutnum)
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

		desc.InputLayout.pInputElementDescs = layouts;
		desc.InputLayout.NumElements = layoutnum;

		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �g���C�A���O���X�g���b�v�̂Ƃ��Ɂu�؂藣���Ȃ����_�W���v�����̃C���f�b�N�X�Ő؂藣�����߂̎w��B�g����ʂ͂قڂȂ��B
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �\���v�f�͎O�p�`�ł���B

		desc.NumRenderTargets = 1; // �����_�[�^�[�Q�b�g��1��
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0~1�ɐ��K�����ꂽRGB

		desc.SampleDesc.Count = 1; // �A���`�G�C���A�V���O�̃T���v�����O��
		desc.SampleDesc.Quality = 0; // �Œ�i��

		desc.DepthStencilState.DepthEnable = true; // �[�x�o�b�t�@��L���ɂ���
		desc.DepthStencilState.StencilEnable = false;
		desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // �s�N�Z���`�掞�ɐ[�x�o�b�t�@�ɐ[�x�l����������
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �[�x�������������̗p
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		desc.pRootSignature = mRootSig;


		HRESULT result = _dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPipe));
		_ASSERT(result == S_OK);
	}


	/// <summary>
	/// �r���[�|�[�g�ƃV�U�[��`�̐ݒ�
	/// �p�C�v���C���A���[�g�V�O�l�`���Ȃǂ��Z�b�g
	/// </summary>
	/// <param name="_cmdList"></param>
	void Setting(ID3D12Device* _dev, ID3D12GraphicsCommandList* _cmdList, ID3D12DescriptorHeap* cnstDescHeap)
	{
		// �r���[�|�[�g
		D3D12_VIEWPORT viewport = {};
		viewport.Width = WINDOW_WIDTH;
		viewport.Height = WINDOW_HEIGHT;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MaxDepth = 1.f;
		viewport.MinDepth = 0.f;
		_cmdList->RSSetViewports(1, &viewport);

		//�V�U�[��`
		D3D12_RECT scissor = {};
		scissor.left = 0;
		scissor.top = 0;
		scissor.right = scissor.left + WINDOW_WIDTH;
		scissor.bottom = scissor.top + WINDOW_HEIGHT;
		_cmdList->RSSetScissorRects(1, &scissor);
	
		// �p�C�v���C��
		_cmdList->SetPipelineState(mPipe);
		// ���[�g�V�O�l�`��
		_cmdList->SetGraphicsRootSignature(mRootSig);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // �g���C�A���O�����X�g
		//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // �g���C�A���O�� �X�g���b�v
		//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST); // �_�`

#if 0
		// SRV��CBV�𓯈ꃋ�[�g�p�����[�^�Ƃ��Ă���
		_cmdList->SetDescriptorHeaps(1, &mDescHeap);
		auto hHeap = mDescHeap->GetGPUDescriptorHandleForHeapStart();
		_cmdList->SetGraphicsRootDescriptorTable(0, hHeap); // ���[�g�p�����[�^�̃C���f�b�N�X�ƃf�B�X�N���v�^�[�q�[�v�̃A�h���X���֘A�t���B
#else
		_cmdList->SetDescriptorHeaps(1, &mDescHeap);
		auto hHeap = mDescHeap->GetGPUDescriptorHandleForHeapStart();
		_cmdList->SetGraphicsRootDescriptorTable(0, hHeap);
		// �萔�o�b�t�@2
		_cmdList->SetDescriptorHeaps(1, &cnstDescHeap);
		auto hHeap2 = cnstDescHeap->GetGPUDescriptorHandleForHeapStart();
		_cmdList->SetGraphicsRootDescriptorTable(1, hHeap2);
#endif
	}
};
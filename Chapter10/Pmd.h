#pragma once

#include "Common.h"
#include <map>
#include <unordered_map>
#include <timeapi.h>

using namespace DirectX;
using namespace std;


class CPmd {
private:
#pragma pack(1)
	struct PMDHeader_t {
		float versin;
		char model_name[20];
		char comment[256];
	};
	struct PMDVertex_t {
		XMFLOAT3 pos;
		XMFLOAT3 normal; // �@���x�N�g��
		XMFLOAT2 uv;
		uint16_t boneNo[2]; // �{�[���ԍ�
		uint8_t boneWeight; // �{�[���e���x
		uint8_t edgeFlg; // �֊s���t���O
	};
	struct PMDBone_t {
		char boneName[20];
		uint16_t parentNo;
		uint16_t nextNo;
		uint8_t type;
		uint16_t ikBoneNo; // IK: Inverse Kinematics
		XMFLOAT3 pos;
	};
	struct PMDMaterial_t {
		XMFLOAT3 diffuse; //�f�B�t���[�Y�F
		float alpha; // �f�B�t���[�Y��
		float specularity;//�X�y�L�����̋���(��Z�l)
		XMFLOAT3 specular; //�X�y�L�����F
		XMFLOAT3 ambient; //�A���r�G���g�F
		uint8_t toonIdx; //�g�D�[���ԍ�(��q)
		uint8_t edgeFlg;//�}�e���A�����̗֊s���t���O
		uint32_t indicesNum; //���̃}�e���A�������蓖����C���f�b�N�X��
		char texFilePath[20]; //�e�N�X�`���t�@�C����(�v���X�A���t�@�c��q)
	};//70�o�C�g�̂͂��c
	struct VMDMotion_t {
		char boneName[15];
		uint32_t frameNo;
		XMFLOAT3 location;
		XMFLOAT4 quaternion;
		uint8_t bezier[64]; // [4][4][4] �x�W�F��ԃp�����[�^
	};
#pragma pack()

	PMDHeader_t mHeader;
	vector<PMDVertex_t> mVertices;
	vector<uint16_t> mIndices;
	vector<PMDMaterial_t> mMaterials;
	vector<PMDBone_t> mBones;
	ID3D12Resource* mVertBuff = nullptr;
	ID3D12Resource* mIdxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVertView = {};
	D3D12_INDEX_BUFFER_VIEW mIdxView = {};
	uint32_t mVertNum = 0;
	uint32_t mIdxNum = 0;
	uint32_t mMaterialNum = 0;
	uint16_t mBoneNum = 0;
	CCommon mC;

	struct BoneNode_t {
		int32_t boneIdx;
		XMFLOAT3 startPos; // ��_
		XMFLOAT3 endPos; // ��[�_
		vector<BoneNode_t*> children;
	};

	vector<XMMATRIX> mBoneMats;
	map<string, BoneNode_t> mBoneNodeMap;

	// VMD data
	uint32_t mMotionDataNum = 0;
	vector<VMDMotion_t> mMotions;
	unordered_map<string, vector<VMDMotion_t>> mMotionMap;

public:
	D3D12_INPUT_ELEMENT_DESC mLayout[6] = {}; // ���_���̓��C�A�E�g
	static const size_t PMD_LAYOUT_NUM = 6;
	static const size_t PMD_VERTEX_SIZE = 38; // ���_�T�C�Y��38byte
	static const size_t PMD_MATERIAL_SIZE = 70;
	
	ID3D12Resource* mCnstBuff = nullptr; // �{�[���p �萔�o�b�t�@
	ID3D12DescriptorHeap* mDescHeap = nullptr; // �{�[���p�f�B�X�N���v�^�q�[�v

	CPmd(CCommon& cmd) : mC(cmd) {
		_ASSERT(PMD_VERTEX_SIZE == sizeof(PMDVertex_t));
		_ASSERT(PMD_MATERIAL_SIZE == sizeof(PMDMaterial_t));
		MakeVertexLayout();
	}

	/// <summary>
	/// PMD�t�@�C����ǂݍ���
	/// </summary>
	void ReadFile() 
	{
		char signature[3] = {}; // �擪�R������"pmd"
		FILE* fp = nullptr;
		fopen_s(&fp, "model/�����~�N.pmd", "rb");
		_ASSERT(fp != nullptr);
		
		fread(signature, sizeof(signature), 1, fp);
		fread(&mHeader, sizeof(mHeader), 1, fp);
		
		fread(&mVertNum, sizeof(mVertNum), 1, fp); // ���_��
		mVertices.resize(mVertNum); // ���_�� ���m��
		for (auto& vert : mVertices) {
			fread(&vert, PMD_VERTEX_SIZE, 1, fp); // ���_�f�[�^
		}

		fread(&mIdxNum, sizeof(mIdxNum), 1, fp); // �C���f�b�N�X��
		mIndices.resize(mIdxNum);
		fread(mIndices.data(), mIndices.size() * sizeof(uint16_t), 1, fp);

		fread(&mMaterialNum, sizeof(mMaterialNum), 1, fp); // �}�e���A����
		mMaterials.resize(mMaterialNum);
		for (auto& mate : mMaterials) {
			fread(&mate, PMD_MATERIAL_SIZE, 1, fp);
		}

		fread(&mBoneNum, sizeof(mBoneNum), 1, fp); // �{�[����
		mBones.resize(mBoneNum);
		fread(mBones.data(), mBones.size() * sizeof(PMDBone_t), 1, fp);

	
		fclose(fp);

		// VMD�t�@�C���ǂݍ���
		ReadVmd();

		// �ǂݍ��񂾃{�[����񂩂�A�{�[���m�[�h�}�b�v�쐬
		CreateBoneNodeMap();
	}

	/// <summary>
	/// VMD�t�@�C���i�|�[�Y�f�[�^�j���J��
	/// </summary>
	void ReadVmd()
	{
		FILE* fp = nullptr;
		//fopen_s(&fp, "model/pose.vmd", "rb");
		fopen_s(&fp, "model/swing.vmd", "rb");
		_ASSERT(fp != nullptr);

		fseek(fp, 50, SEEK_SET); // skip 50byte
		fread(&mMotionDataNum, sizeof(mMotionDataNum), 1, fp);
		mMotions.resize(mMotionDataNum);
		fread(mMotions.data(), mMotions.size() * sizeof(VMDMotion_t), 1, fp);

		fclose(fp);

		// �}�b�v������Ă���
		for (auto& mot : mMotions) {
			mMotionMap[mot.boneName].emplace_back(mot);
		}
	}

	/// <summary>
	/// �{�[���m�[�h�}�b�v�̍쐬�E�{�[���s�񏉊���
	/// </summary>
	void CreateBoneNodeMap()
	{
		vector<string> names;
		// �{�[���m�[�h�}�b�v�����
		int idx = 0;
		for (auto& bone : mBones) {
			names.emplace_back(bone.boneName);
			BoneNode_t& node = mBoneNodeMap[bone.boneName];
			node.boneIdx = idx++;
			node.startPos = bone.pos;
		}

		// �e�q�֌W�\�z
		for (auto& bone : mBones) {
			if (bone.parentNo >= mBones.size()) {
				continue; // ����
			}
			string parentName = names[bone.parentNo];
			mBoneNodeMap[parentName].children.emplace_back(&mBoneNodeMap[bone.boneName]);
		}

		// �{�[���s���������
		mBoneMats.resize(mBones.size());
		std::fill(mBoneMats.begin(), mBoneMats.end(), XMMatrixIdentity());
	}

	/// <summary>
	/// �ċA�֐��@�s���Ώۃm�[�h�Ƃ��̎q�m�[�h�B�ɏ�Z����
	/// </summary>
	/// <param name="node"></param>
	/// <param name="mat"></param>
	void RecursiveMulMat(BoneNode_t *node, const XMMATRIX &mat)
	{
		mBoneMats[node->boneIdx] *= mat;

		for (auto& child : node->children) {
			RecursiveMulMat(child, mBoneMats[node->boneIdx]);
		}
	}

	void ChangePos()
	{
#if 0
		{
			auto node = mBoneNodeMap["�E�r"];
			auto& pos = node.startPos;
			XMMATRIX mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * XMMatrixRotationZ(-XM_PIDIV4) * XMMatrixTranslation(pos.x, pos.y, pos.z);
			RecursiveMulMat(&node, mat);
		}
		{
			auto node = mBoneNodeMap["�E�Ђ�"];
			auto& pos = node.startPos;
			XMMATRIX mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * XMMatrixRotationZ(-XM_PIDIV4) * XMMatrixTranslation(pos.x, pos.y, pos.z);
			RecursiveMulMat(&node, mat);
		}
#else
		for (auto& mtn : mMotions) {
			auto node = mBoneNodeMap[mtn.boneName];
			auto& pos = node.startPos;
			XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&mtn.quaternion));
			XMMATRIX mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * rot * XMMatrixTranslation(pos.x, pos.y, pos.z);
			mBoneMats[node.boneIdx] = mat;
		}
		// ���[�g����ċA�v�Z
		RecursiveMulMat(&mBoneNodeMap["�Z���^�["], XMMatrixIdentity());
#endif
	}

	void MakeBuff(ID3D12Device* _dev)
	{
		mC.MakeBuffResource(_dev, mVertices.size() * PMD_VERTEX_SIZE, &mVertBuff);
		mC.MakeBuffResource(_dev, mIndices.size() * sizeof(uint16_t), &mIdxBuff);
		mC.MakeBuffResource256(_dev, mBoneMats.size() * sizeof(XMMATRIX), &mCnstBuff);
	}

	void Map()
	{
		ChangePos();
		mC.Map(mVertBuff, mVertices.data(), mVertices.size() * PMD_VERTEX_SIZE);
		mC.Map(mIdxBuff, mIndices.data(), mIndices.size() * sizeof(uint16_t));
		mC.Map(mCnstBuff, mBoneMats.data(), mBoneMats.size() * sizeof(XMMATRIX));
	}

	
	/// <summary>
	/// ���_�o�b�t�@�r���[
	/// �C���f�b�N�X�p�o�b�t�@�[�r���[
	/// �{�[���p�萔�o�b�t�@ �f�B�X�N���v�^�q�[�v�ƃr���[
	/// </summary>
	/// <param name="_dev"></param>
	void MakeView(ID3D12Device* _dev)
	{
		mVertView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
		mVertView.SizeInBytes = mVertices.size() * PMD_VERTEX_SIZE; // �S�o�C�g��
		mVertView.StrideInBytes = PMD_VERTEX_SIZE; // 1���_�̃o�C�g��

		mIdxView.BufferLocation = mIdxBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
		mIdxView.SizeInBytes = mIndices.size() * sizeof(uint16_t); // �S�o�C�g��
		mIdxView.Format = DXGI_FORMAT_R16_UINT;
		//mIdxView.StrideInBytes = sizeof(uint16_t); // 1�C���f�b�N�X�̃o�C�g��

#if 1
		// �{�[���p �f�B�X�N���v�^�[�q�[�v�����
		D3D12_DESCRIPTOR_HEAP_DESC hpdesc = {};
		hpdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shader���猩����悤��
		hpdesc.NodeMask = 0;
		hpdesc.NumDescriptors = 1;
		hpdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // CBV�p

		HRESULT result = _dev->CreateDescriptorHeap(&hpdesc, IID_PPV_ARGS(&mDescHeap));
		_ASSERT(result == S_OK);

		// �萔�o�b�t�@�r���[�����
		auto heapHandle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
		cbvdesc.BufferLocation = mCnstBuff->GetGPUVirtualAddress();
		cbvdesc.SizeInBytes = mCnstBuff->GetDesc().Width;
		_dev->CreateConstantBufferView(&cbvdesc, heapHandle);
#endif
	}

	/// <summary>
/// ���_���C�A�E�g�E�e�N�X�`��uv���W���C�A�E�g�\���̂̍쐬
/// </summary>
	void MakeVertexLayout()
	{
		// ���W���
		mLayout[0].SemanticName = "POSITION"; // ���W�ł��邱�Ƃ��Ӗ�����
		mLayout[0].SemanticIndex = 0; // �����Z�}���e�B�N�X���̂Ƃ��͎g�p����
		mLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // 32bit x 3 �̃f�[�^
		mLayout[0].InputSlot = 0; // �����̒��_�f�[�^�����킹�ĂP�̒��_�f�[�^��\���������Ƃ��Ɏg�p����
		mLayout[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // ���̃f�[�^�̏ꏊ. ����͎����玟�Ƀf�[�^������ł��邱�Ƃ�\���B
		mLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // �f�[�^�̓��e�Ƃ��āA1���_���Ƃɂ��̃��C�A�E�g�������Ă���B
		mLayout[0].InstanceDataStepRate = 0; // �C���X�^���V���O�̂Ƃ��A�P�x�ɕ`�悷��C���X�^���X�̐����w�肷��B
		// �@��
		mLayout[1].SemanticName = "NORMAL";
		mLayout[1].SemanticIndex = 0;
		mLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		mLayout[1].InputSlot = 0;
		mLayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[1].InstanceDataStepRate = 0;
		// uv
		mLayout[2].SemanticName = "TEXCORD";
		mLayout[2].SemanticIndex = 0;
		mLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT; // 32bit x 2 �̃f�[�^
		mLayout[2].InputSlot = 0;
		mLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[2].InstanceDataStepRate = 0;
		// �{�[���ԍ�
		mLayout[3].SemanticName = "BONE_NO";
		mLayout[3].SemanticIndex = 0;
		mLayout[3].Format = DXGI_FORMAT_R16G16_UINT; // 16bit x 2 �̃f�[�^
		mLayout[3].InputSlot = 0;
		mLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[3].InstanceDataStepRate = 0;
		// �E�F�C�g
		mLayout[4].SemanticName = "WEIGHT";
		mLayout[4].SemanticIndex = 0;
		mLayout[4].Format = DXGI_FORMAT_R8_UINT; // 8bit �̃f�[�^
		mLayout[4].InputSlot = 0;
		mLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[4].InstanceDataStepRate = 0;
		// �֊s���t���O
		mLayout[5].SemanticName = "EDGE_FLG";
		mLayout[5].SemanticIndex = 0;
		mLayout[5].Format = DXGI_FORMAT_R8_UINT; // 8bit �̃f�[�^
		mLayout[5].InputSlot = 0;
		mLayout[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[5].InstanceDataStepRate = 0;
	}

	/// <summary>
	/// pmd���f���`��
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->IASetIndexBuffer(&mIdxView);// ���_�C���f�b�N�X
		_cmdList->IASetVertexBuffers(
			0, // �X���b�g�ԍ�
			1, // ���_�o�b�t�@�[�r���[�̐�
			&mVertView
		);
		_cmdList->DrawIndexedInstanced(mIdxNum, 1, 0, 0, 0);
	}

private:
	DWORD mStartTime;
public:
	DWORD mFrameNo;
	void PlayAnime()
	{
		mStartTime = timeGetTime();
	}
	void MotionUpdate()
	{
		DWORD elapsed = timeGetTime() - mStartTime;
		mFrameNo = elapsed * (30.0f / 1000.f); // 30FPS
		mFrameNo %= 90;

		// �N���A���Ă����Ȃ��ƑO�t���[���̂��̂ɏd�˂�������Ă��������Ȃ�
		std::fill(mBoneMats.begin(), mBoneMats.end(), XMMatrixIdentity());

		// ���[�V�����f�[�^�X�V
		for (auto& mot : mMotionMap)
		{
			// ���v����t���[����T�� ��΂������Ă�[��
			auto motvec = mot.second;
			auto rit = std::find_if(motvec.rbegin(), motvec.rend(),
				[=](const VMDMotion_t& m) {return m.frameNo <= mFrameNo; }); // �����_���B�����ϐ����L���v�`�����Ă�B
			// ��v���郂�[�V�������Ȃ���Δ�΂�
			if (rit == motvec.rend()) continue;
			
			// �X�V
			auto node = mBoneNodeMap[mot.first];
			auto& pos = node.startPos;
			XMVECTOR quat = XMLoadFloat4(&(rit->quaternion)); // �N�H�[�^�j�I��
			// ���`hokan
			if (rit.base() != motvec.end()) {
				XMVECTOR quat2 = XMLoadFloat4(&(rit.base()->quaternion));
				float t = (float)(mFrameNo - rit->frameNo) / (rit.base()->frameNo - rit->frameNo);
				//quat = (1.0f - t) * quat + t * quat2;
				quat = XMQuaternionSlerp(quat, quat2, t); // ���ʐ��`��� Sphere Linier intERPolation
			}
			auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslation(pos.x, pos.y, pos.z); // �N�H�[�^�j�I���ɂ���]
			mBoneMats[node.boneIdx] = mat;
		}
		// ���[�g����ċA�v�Z
		RecursiveMulMat(&mBoneNodeMap["�Z���^�["], XMMatrixIdentity());

		// �{�[��data�X�V
		mC.Map(mCnstBuff, mBoneMats.data(), mBoneMats.size() * sizeof(XMMATRIX));
	}
};
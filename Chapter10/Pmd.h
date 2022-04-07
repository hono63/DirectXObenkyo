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
		XMFLOAT3 normal; // 法線ベクトル
		XMFLOAT2 uv;
		uint16_t boneNo[2]; // ボーン番号
		uint8_t boneWeight; // ボーン影響度
		uint8_t edgeFlg; // 輪郭線フラグ
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
		XMFLOAT3 diffuse; //ディフューズ色
		float alpha; // ディフューズα
		float specularity;//スペキュラの強さ(乗算値)
		XMFLOAT3 specular; //スペキュラ色
		XMFLOAT3 ambient; //アンビエント色
		uint8_t toonIdx; //トゥーン番号(後述)
		uint8_t edgeFlg;//マテリアル毎の輪郭線フラグ
		uint32_t indicesNum; //このマテリアルが割り当たるインデックス数
		char texFilePath[20]; //テクスチャファイル名(プラスアルファ…後述)
	};//70バイトのはず…
	struct VMDMotion_t {
		char boneName[15];
		uint32_t frameNo;
		XMFLOAT3 location;
		XMFLOAT4 quaternion;
		uint8_t bezier[64]; // [4][4][4] ベジェ補間パラメータ
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
		XMFLOAT3 startPos; // 基準点
		XMFLOAT3 endPos; // 先端点
		vector<BoneNode_t*> children;
	};

	vector<XMMATRIX> mBoneMats;
	map<string, BoneNode_t> mBoneNodeMap;

	// VMD data
	uint32_t mMotionDataNum = 0;
	vector<VMDMotion_t> mMotions;
	unordered_map<string, vector<VMDMotion_t>> mMotionMap;

public:
	D3D12_INPUT_ELEMENT_DESC mLayout[6] = {}; // 頂点入力レイアウト
	static const size_t PMD_LAYOUT_NUM = 6;
	static const size_t PMD_VERTEX_SIZE = 38; // 頂点サイズは38byte
	static const size_t PMD_MATERIAL_SIZE = 70;
	
	ID3D12Resource* mCnstBuff = nullptr; // ボーン用 定数バッファ
	ID3D12DescriptorHeap* mDescHeap = nullptr; // ボーン用ディスクリプタヒープ

	CPmd(CCommon& cmd) : mC(cmd) {
		_ASSERT(PMD_VERTEX_SIZE == sizeof(PMDVertex_t));
		_ASSERT(PMD_MATERIAL_SIZE == sizeof(PMDMaterial_t));
		MakeVertexLayout();
	}

	/// <summary>
	/// PMDファイルを読み込む
	/// </summary>
	void ReadFile() 
	{
		char signature[3] = {}; // 先頭３文字は"pmd"
		FILE* fp = nullptr;
		fopen_s(&fp, "model/初音ミク.pmd", "rb");
		_ASSERT(fp != nullptr);
		
		fread(signature, sizeof(signature), 1, fp);
		fread(&mHeader, sizeof(mHeader), 1, fp);
		
		fread(&mVertNum, sizeof(mVertNum), 1, fp); // 頂点数
		mVertices.resize(mVertNum); // 頂点数 を確保
		for (auto& vert : mVertices) {
			fread(&vert, PMD_VERTEX_SIZE, 1, fp); // 頂点データ
		}

		fread(&mIdxNum, sizeof(mIdxNum), 1, fp); // インデックス数
		mIndices.resize(mIdxNum);
		fread(mIndices.data(), mIndices.size() * sizeof(uint16_t), 1, fp);

		fread(&mMaterialNum, sizeof(mMaterialNum), 1, fp); // マテリアル数
		mMaterials.resize(mMaterialNum);
		for (auto& mate : mMaterials) {
			fread(&mate, PMD_MATERIAL_SIZE, 1, fp);
		}

		fread(&mBoneNum, sizeof(mBoneNum), 1, fp); // ボーン数
		mBones.resize(mBoneNum);
		fread(mBones.data(), mBones.size() * sizeof(PMDBone_t), 1, fp);

	
		fclose(fp);

		// VMDファイル読み込み
		ReadVmd();

		// 読み込んだボーン情報から、ボーンノードマップ作成
		CreateBoneNodeMap();
	}

	/// <summary>
	/// VMDファイル（ポーズデータ）を開く
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

		// マップを作っておく
		for (auto& mot : mMotions) {
			mMotionMap[mot.boneName].emplace_back(mot);
		}
	}

	/// <summary>
	/// ボーンノードマップの作成・ボーン行列初期化
	/// </summary>
	void CreateBoneNodeMap()
	{
		vector<string> names;
		// ボーンノードマップを作る
		int idx = 0;
		for (auto& bone : mBones) {
			names.emplace_back(bone.boneName);
			BoneNode_t& node = mBoneNodeMap[bone.boneName];
			node.boneIdx = idx++;
			node.startPos = bone.pos;
		}

		// 親子関係構築
		for (auto& bone : mBones) {
			if (bone.parentNo >= mBones.size()) {
				continue; // 無効
			}
			string parentName = names[bone.parentNo];
			mBoneNodeMap[parentName].children.emplace_back(&mBoneNodeMap[bone.boneName]);
		}

		// ボーン行列を初期化
		mBoneMats.resize(mBones.size());
		std::fill(mBoneMats.begin(), mBoneMats.end(), XMMatrixIdentity());
	}

	/// <summary>
	/// 再帰関数　行列を対象ノードとその子ノード達に乗算する
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
			auto node = mBoneNodeMap["右腕"];
			auto& pos = node.startPos;
			XMMATRIX mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * XMMatrixRotationZ(-XM_PIDIV4) * XMMatrixTranslation(pos.x, pos.y, pos.z);
			RecursiveMulMat(&node, mat);
		}
		{
			auto node = mBoneNodeMap["右ひじ"];
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
		// ルートから再帰計算
		RecursiveMulMat(&mBoneNodeMap["センター"], XMMatrixIdentity());
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
	/// 頂点バッファビュー
	/// インデックス用バッファービュー
	/// ボーン用定数バッファ ディスクリプタヒープとビュー
	/// </summary>
	/// <param name="_dev"></param>
	void MakeView(ID3D12Device* _dev)
	{
		mVertView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
		mVertView.SizeInBytes = mVertices.size() * PMD_VERTEX_SIZE; // 全バイト数
		mVertView.StrideInBytes = PMD_VERTEX_SIZE; // 1頂点のバイト数

		mIdxView.BufferLocation = mIdxBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
		mIdxView.SizeInBytes = mIndices.size() * sizeof(uint16_t); // 全バイト数
		mIdxView.Format = DXGI_FORMAT_R16_UINT;
		//mIdxView.StrideInBytes = sizeof(uint16_t); // 1インデックスのバイト数

#if 1
		// ボーン用 ディスクリプターヒープを作る
		D3D12_DESCRIPTOR_HEAP_DESC hpdesc = {};
		hpdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shaderから見えるように
		hpdesc.NodeMask = 0;
		hpdesc.NumDescriptors = 1;
		hpdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // CBV用

		HRESULT result = _dev->CreateDescriptorHeap(&hpdesc, IID_PPV_ARGS(&mDescHeap));
		_ASSERT(result == S_OK);

		// 定数バッファビューを作る
		auto heapHandle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
		cbvdesc.BufferLocation = mCnstBuff->GetGPUVirtualAddress();
		cbvdesc.SizeInBytes = mCnstBuff->GetDesc().Width;
		_dev->CreateConstantBufferView(&cbvdesc, heapHandle);
#endif
	}

	/// <summary>
/// 頂点レイアウト・テクスチャuv座標レイアウト構造体の作成
/// </summary>
	void MakeVertexLayout()
	{
		// 座標情報
		mLayout[0].SemanticName = "POSITION"; // 座標であることを意味する
		mLayout[0].SemanticIndex = 0; // 同じセマンティクス名のときは使用する
		mLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // 32bit x 3 のデータ
		mLayout[0].InputSlot = 0; // 複数の頂点データを合わせて１つの頂点データを表現したいときに使用する
		mLayout[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // そのデータの場所. 今回は次から次にデータが並んでいることを表す。
		mLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // データの内容として、1頂点ごとにこのレイアウトが入っている。
		mLayout[0].InstanceDataStepRate = 0; // インスタンシングのとき、１度に描画するインスタンスの数を指定する。
		// 法線
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
		mLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT; // 32bit x 2 のデータ
		mLayout[2].InputSlot = 0;
		mLayout[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[2].InstanceDataStepRate = 0;
		// ボーン番号
		mLayout[3].SemanticName = "BONE_NO";
		mLayout[3].SemanticIndex = 0;
		mLayout[3].Format = DXGI_FORMAT_R16G16_UINT; // 16bit x 2 のデータ
		mLayout[3].InputSlot = 0;
		mLayout[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[3].InstanceDataStepRate = 0;
		// ウェイト
		mLayout[4].SemanticName = "WEIGHT";
		mLayout[4].SemanticIndex = 0;
		mLayout[4].Format = DXGI_FORMAT_R8_UINT; // 8bit のデータ
		mLayout[4].InputSlot = 0;
		mLayout[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[4].InstanceDataStepRate = 0;
		// 輪郭線フラグ
		mLayout[5].SemanticName = "EDGE_FLG";
		mLayout[5].SemanticIndex = 0;
		mLayout[5].Format = DXGI_FORMAT_R8_UINT; // 8bit のデータ
		mLayout[5].InputSlot = 0;
		mLayout[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		mLayout[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		mLayout[5].InstanceDataStepRate = 0;
	}

	/// <summary>
	/// pmdモデル描画
	/// </summary>
	/// <param name="_cmdList"></param>
	void Draw(ID3D12GraphicsCommandList* _cmdList)
	{
		_cmdList->IASetIndexBuffer(&mIdxView);// 頂点インデックス
		_cmdList->IASetVertexBuffers(
			0, // スロット番号
			1, // 頂点バッファービューの数
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

		// クリアしておかないと前フレームのものに重ねがけされておかしくなる
		std::fill(mBoneMats.begin(), mBoneMats.end(), XMMatrixIdentity());

		// モーションデータ更新
		for (auto& mot : mMotionMap)
		{
			// 合致するフレームを探す りばぁすいてれーた
			auto motvec = mot.second;
			auto rit = std::find_if(motvec.rbegin(), motvec.rend(),
				[=](const VMDMotion_t& m) {return m.frameNo <= mFrameNo; }); // ラムダ式。自動変数をキャプチャしてる。
			// 一致するモーションがなければ飛ばす
			if (rit == motvec.rend()) continue;
			
			// 更新
			auto node = mBoneNodeMap[mot.first];
			auto& pos = node.startPos;
			XMVECTOR quat = XMLoadFloat4(&(rit->quaternion)); // クォータニオン
			// 線形hokan
			if (rit.base() != motvec.end()) {
				XMVECTOR quat2 = XMLoadFloat4(&(rit.base()->quaternion));
				float t = (float)(mFrameNo - rit->frameNo) / (rit.base()->frameNo - rit->frameNo);
				//quat = (1.0f - t) * quat + t * quat2;
				quat = XMQuaternionSlerp(quat, quat2, t); // 球面線形補間 Sphere Linier intERPolation
			}
			auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * XMMatrixRotationQuaternion(quat) * XMMatrixTranslation(pos.x, pos.y, pos.z); // クォータニオンによる回転
			mBoneMats[node.boneIdx] = mat;
		}
		// ルートから再帰計算
		RecursiveMulMat(&mBoneNodeMap["センター"], XMMatrixIdentity());

		// ボーンdata更新
		mC.Map(mCnstBuff, mBoneMats.data(), mBoneMats.size() * sizeof(XMMATRIX));
	}
};
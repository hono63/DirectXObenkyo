#pragma once

#include "Common.h"

using namespace DirectX;
using namespace std;


class CPmd {
private:
	vector<uint8_t> mVertices;
	vector<uint16_t> mIndices;
	ID3D12Resource* mVertBuff = nullptr;
	ID3D12Resource* mIdxBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVertView = {};
	D3D12_INDEX_BUFFER_VIEW mIdxView = {};
	uint32_t mVertNum = 0;
	uint32_t mIdxNum = 0;
	CCommon mC;
public:
	D3D12_INPUT_ELEMENT_DESC mLayout[6] = {}; // 頂点入力レイアウト
	static const size_t PMD_LAYOUT_NUM = 6;
	static const size_t PMD_VERTEX_SIZE = 38; // 頂点サイズは38byte
	
	struct PMDHeader_t {
		float versin;
		char model_name[20];
		char comment[256];
	};
	PMDHeader_t mHeader;

	struct PMDVertex_t {
		XMFLOAT3 pos;
		XMFLOAT3 normal; // 法線ベクトル
		XMFLOAT2 uv;
		uint16_t boneNo[2]; // ボーン番号
		uint8_t boneWeight; // ボーン影響度
		uint8_t edgeFlg; // 輪郭線フラグ
	};

	CPmd(CCommon& cmd) : mC(cmd) {}

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
		mVertices.resize(mVertNum * PMD_VERTEX_SIZE); // 頂点数 x 38byte を確保
		fread(mVertices.data(), mVertices.size(), 1, fp); // 頂点データ
		
		fread(&mIdxNum, sizeof(mIdxNum), 1, fp); // インデックス数
		mIndices.resize(mIdxNum);
		fread(mIndices.data(), mIndices.size() * sizeof(uint16_t), 1, fp);

		fclose(fp);
	}

	void MakeBuff(ID3D12Device* _dev)
	{
		mC.MakeBuffResource(_dev, mVertices.size(), &mVertBuff);
		mC.MakeBuffResource(_dev, mIndices.size() * sizeof(uint16_t), &mIdxBuff);
	}

	void Map()
	{
		mC.Map(mVertBuff, mVertices.data(), mVertices.size());
		mC.Map(mIdxBuff, mIndices.data(), mIndices.size() * sizeof(uint16_t));
	}

	void MakeView()
	{
		mVertView.BufferLocation = mVertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
		mVertView.SizeInBytes = mVertices.size(); // 全バイト数
		mVertView.StrideInBytes = PMD_VERTEX_SIZE; // 1頂点のバイト数

		mIdxView.BufferLocation = mIdxBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
		mIdxView.SizeInBytes = mIndices.size() * sizeof(uint16_t); // 全バイト数
		mIdxView.Format = DXGI_FORMAT_R16_UINT;
		//mIdxView.StrideInBytes = sizeof(uint16_t); // 1インデックスのバイト数
	}

	/// <summary>
/// 頂点レイアウト・テクスチャuv座標レイアウト構造体の作成
/// </summary>
	void MakeLayout()
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
};
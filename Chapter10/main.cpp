
#include "Common.h"
#include "Gamen.h"
#include "Sankaku.h"
#include "Texture.h"
#include "Const.h"
#include "Pmd.h"
#include "Depth.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#ifdef _DEBUG
#pragma comment(lib, "DirectXTexd.lib")
#else
#pragma comment(lib, "DirectXTex.lib")
#endif
#pragma comment(lib, "winmm.lib") // timeGetTime()用


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



// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// メイン関数
#ifdef _DEBUG
int main(int argc, char** argv)
#else
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
#endif
{
	OutputDebugString(_T("Hello DirectX World!!\n"));

	CCommon common;

	Gamen gamen;
	HWND hwnd = gamen.SetupWindow(_T("DirectX ちゃぷたー6"), (WNDPROC)WindowProcedure);
	gamen.SetUpDirectX();

#if 0
	Sankaku sank(common);
	sank.MakeVertBuff(gamen.m_dev);
	sank.MakeIndexBuff(gamen.m_dev);
	sank.MapVertex();
	sank.MapIndex();
	sank.MakeView(gamen.m_dev);
#endif

	CTexture tex(common);
	tex.ReadFile();
	tex.MakeBuff(gamen.m_dev);
	tex.WriteSubresource();

	ConstBuf cnst(common);
	cnst.MakeBuff(gamen.m_dev);
	cnst.Map();

#if 1
	CPmd pmd(common);
	pmd.ReadFile();
	pmd.MakeBuff(gamen.m_dev);
	pmd.Map();
	pmd.MakeView(gamen.m_dev);
#endif

	CDepth depth(common);
	depth.MakeBuff(gamen.m_dev);
	depth.MakeDescriptor(gamen.m_dev);

	common.MakeDescriptors(gamen.m_dev, tex.mTexBuffRes, cnst.mBuf, pmd.mCnstBuff);
	common.CompileVS();
	common.CompilePS();
	common.MakeRootSignature(gamen.m_dev);
	common.MakePipeline(gamen.m_dev, pmd.mLayout, pmd.PMD_LAYOUT_NUM);
	
	ShowWindow(hwnd, SW_SHOW);

	// ゲームループ
	MSG msg = {};
	clock_t pretime = clock();
	int count = 0;
	float clearColor[4] = { 1.f, 1.f, 1.f, 1.0f }; // RGBA
	pmd.PlayAnime();
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) { // アプリ終了
			break;
		}

		common.Setting(gamen.m_dev, gamen.m_cmdList, pmd.mDescHeap);
		depth.Clear(gamen.m_cmdList);
		gamen.Render(clearColor, depth.GetDescHandle());
		//sank.Draw(gamen.m_cmdList);
		pmd.MotionUpdate();
		pmd.Draw(gamen.m_cmdList);
		//cnst.Kaiten();
		gamen.Execute();

		// 時間計測
		double t = (double)(clock() - pretime) / CLOCKS_PER_SEC;
		pretime = clock();
		if (count % 10 == 0) {
			DebugString(_T("%.2f sec (%.2f FPS)  Frame:%d \r"), t, 1.0 / t, pmd.mFrameNo);
		}
	}
	DebugString(_T("\n"));

	DebugString(_T("END.\n"));

	return 0;
}

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <tchar.h> // _T() 定義
#include <stdlib.h>
#include <time.h>

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "Gamen.h"
#include "Sankaku.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#ifdef _DEBUG
#pragma comment(lib, "DirectXTexd.lib")
#else
#pragma comment(lib, "DirectXTex.lib")
#endif

using namespace DirectX;
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

	Gamen gamen;
	HWND hwnd = gamen.SetupWindow(_T("DirectX ちゃぷたー4"), (WNDPROC)WindowProcedure);
	gamen.SetUpDirectX();

	Sankaku sank;
	sank.MakeVertBuff(gamen.m_dev);
	sank.MapVertex();
	sank.MakeView(gamen.m_dev);
	
	sank.ReadTex();
	sank.MakeTexBuff(gamen.m_dev);
	sank.WriteTex();
	sank.MakeShaderResourceView(gamen.m_dev);
	
	sank.CompileVS();
	sank.CompilePS();
	sank.MakeLayout();
	sank.MakeRootSignature(gamen.m_dev);
	sank.MakePipeline(gamen.m_dev);
	
	ShowWindow(hwnd, SW_SHOW);

	// ゲームループ
	MSG msg = {};
	clock_t pretime = clock();
	int count = 0;
	float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f }; // RGBA
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) { // アプリ終了
			break;
		}

		gamen.Render(clearColor);
		sank.ViewPort(gamen.m_cmdList);
		sank.Draw(gamen.m_cmdList);
		gamen.Execute();

		double t = (double)(clock() - pretime) / CLOCKS_PER_SEC;
		pretime = clock();
		if (count % 10 == 0) {
			DebugString(_T("%.2f sec (%.2f FPS)    \r"), t, 1.0 / t);
		}
	}
	DebugString(_T("\n"));

	DebugString(_T("END.\n"));

	return 0;
}
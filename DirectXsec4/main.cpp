﻿
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

#include "Gamen.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

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



class Sankaku {
public:
	XMFLOAT3 vertices[3];
	Sankaku() {
		vertices[0] = XMFLOAT3{ -1.f, -1.f, 0.f }; // 左下
		vertices[1] = XMFLOAT3{ -1.f,  1.f, 0.f }; // 左上
		vertices[2] = XMFLOAT3{  1.f, -1.f, 0.f }; // 右下
	}
};

// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// メイン関数
#ifdef _DEBUG
int main()
#else
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
#endif
{
	OutputDebugString(_T("Hello DirectX World!!\n"));

	Gamen gamen;

	WNDCLASSEX w = {}; // ウィンドウクラスの生成
	HWND hwnd = gamen.SetupWindow(w, (WNDPROC)WindowProcedure, _T("DirectX ちゃぷたー4"));

	gamen.SetUpDirectX();
	
	ShowWindow(hwnd, SW_SHOW);


	// ゲームループ
	MSG msg = {};
	clock_t pretime = clock();
	int count = 0;
	float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) { // アプリ終了
			break;
		}

		clearColor[0] += (float)(rand() % 11 - 5) / 500.0f;
		clearColor[1] += (float)(rand() % 11 - 5) / 500.0f;
		clearColor[2] += (float)(rand() % 11 - 5) / 500.0f;
		gamen.Proc(clearColor);

		double t = (double)(clock() - pretime) / CLOCKS_PER_SEC;
		pretime = clock();
		if (count % 10 == 0) {
			DebugString(_T("%.2f sec (%.2f FPS)    \r"), t, 1.0 / t);
		}
	}
	DebugString(_T("\n"));
	// クラスの登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);


	DebugString(_T("END.\n"));

	return 0;
}
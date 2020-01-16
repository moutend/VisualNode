#include <cpplogger/cpplogger.h>
#include <windows.h>

#include <d2d1.h>
#include <wincodec.h>
#include <wincodecsdk.h>

#include <strsafe.h>

#include "context.h"
#include "highlightloop.h"
#include "util.h"

extern Logger::Logger *Log;

ID2D1Factory *pD2d1Factory{};
ID2D1HwndRenderTarget *pRenderTarget{};

LRESULT CALLBACK highlightWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
  switch (uMsg) {
  case WM_DISPLAYCHANGE: {
    Log->Info(L"WM_DISPLAYCHANGE received", GetCurrentThreadId(), __LONGFILE__);
  } break;
  case WM_CREATE: {
    Log->Info(L"WM_CREATE received", GetCurrentThreadId(), __LONGFILE__);
    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 128,
                               LWA_COLORKEY | LWA_ALPHA);
    CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
    HRESULT hr =
        D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to call D2D1CreateFactory", GetCurrentThreadId(),
                __LONGFILE__);
      break;
    }

    D2D1_SIZE_U pixelSize = {static_cast<UINT32>(createStruct->cx),
                             static_cast<UINT32>(createStruct->cy)};
    D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties =
        D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndRenderTargetProperties =
        D2D1::HwndRenderTargetProperties(hWnd, pixelSize);
    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 64, LWA_COLORKEY);
    hr = pD2d1Factory->CreateHwndRenderTarget(
        renderTargetProperties, hwndRenderTargetProperties, &pRenderTarget);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to call ID2D1Factory::CreateHwndRenderTarget",
                GetCurrentThreadId(), __LONGFILE__);
      break;
    }

    ShowWindow(hWnd, SW_SHOW);
  } break;
  case WM_DESTROY: {
    Log->Info(L"WM_DESTROY received", GetCurrentThreadId(), __LONGFILE__);
    SafeRelease(&pRenderTarget);
    SafeRelease(&pD2d1Factory);
    PostQuitMessage(0);
  } break;
  case WM_PAINT: {
    Log->Info(L"WM_PAINT received", GetCurrentThreadId(), __LONGFILE__);

    D2D1_SIZE_F targetSize = pRenderTarget->GetSize();
    PAINTSTRUCT paint;
    BeginPaint(hWnd, &paint);
    pRenderTarget->BeginDraw();

    D2D1_COLOR_F blackColor = {0.0f, 0.0f, 0.0f, 1.0f};
    pRenderTarget->Clear(blackColor);

    { // @@@begin
      ID2D1SolidColorBrush *pBrush{};
      pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f),
                                           &pBrush);

      if (pBrush != nullptr) {
        D2D1_POINT_2F center =
            D2D1::Point2F(targetSize.width / 2, targetSize.height / 2);
        D2D1_ROUNDED_RECT roundRect =
            D2D1::RoundedRect(D2D1::RectF(static_cast<float>(center.x),
                                          static_cast<float>(center.y),
                                          static_cast<float>(center.x + 100),
                                          static_cast<float>(center.y + 100)),
                              16.0f, 16.0f);
        pRenderTarget->DrawRoundedRectangle(&roundRect, pBrush, 8.0f);
        pBrush->Release();
      }

      ID2D1SolidColorBrush *pBrush2{};
      pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
                                           &pBrush2);

      if (pBrush2 != nullptr) {
        D2D1_POINT_2F center2 =
            D2D1::Point2F(targetSize.width / 2, targetSize.height / 2);
        D2D1_ROUNDED_RECT roundRect2 =
            D2D1::RoundedRect(D2D1::RectF(static_cast<float>(center2.x - 150),
                                          static_cast<float>(center2.y - 150),
                                          static_cast<float>(center2.x + 100),
                                          static_cast<float>(center2.y + 100)),
                              16.0f, 16.0f);
        pRenderTarget->DrawRoundedRectangle(&roundRect2, pBrush2, 8.0f);
        pBrush2->Release();
      }
    } // @@@end
    pRenderTarget->EndDraw();
    EndPaint(hWnd, &paint);
  }
    return false;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

struct HighlightPaintLoopContext {
  HANDLE QuitEvent = nullptr;
  HANDLE PaintEvent = nullptr;
  HWND TargetWindow = nullptr;
  bool IsActive = true;
  float Left = 0.0f;
  float Top = 0.0f;
  float Width = 50.0f;
  float Height = 50.0f;
  float Radius = 10.0f;
  float BorderThickness = 5.0f;
};

DWORD WINAPI highlightPaintLoop(LPVOID context) {
  Log->Info(L"Start highlight paint loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  HighlightPaintLoopContext *ctx =
      static_cast<HighlightPaintLoopContext *>(context);

  if (ctx == nullptr) {
    return E_FAIL;
  }

  while (ctx->IsActive) {
    HANDLE waitArray[2] = {ctx->QuitEvent, ctx->PaintEvent};
    DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);

    if (waitResult == WAIT_OBJECT_0 + 0) {
      ctx->IsActive = false;
      continue;
    }
    if (pRenderTarget == nullptr) {
      Log->Warn(L"Direct2D rendering is not available", GetCurrentThreadId(),
                __LONGFILE__);
      continue;
    }
    if (ctx->TargetWindow == nullptr) {
      Log->Warn(L"Highlight window is not created yet", GetCurrentThreadId(),
                __LONGFILE__);
      continue;
    }

    D2D1_SIZE_F targetSize = pRenderTarget->GetSize();

    InvalidateRect(ctx->TargetWindow, nullptr, true);
    HDC hDC = GetDC(ctx->TargetWindow);

    pRenderTarget->BeginDraw();

    D2D1_COLOR_F blackColor = {0.0f, 0.0f, 0.0f, 1.0f};
    pRenderTarget->Clear(blackColor);

    ID2D1SolidColorBrush *pBrush{};
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
                                         &pBrush);

    if (pBrush != nullptr) {
      D2D1_ROUNDED_RECT roundRect = D2D1::RoundedRect(
          D2D1::RectF(ctx->Left, ctx->Top, ctx->Width, ctx->Height),
          ctx->Radius, ctx->Radius);
      pRenderTarget->DrawRoundedRectangle(&roundRect, pBrush,
                                          ctx->BorderThickness);
      pBrush->Release();
    }

    pRenderTarget->EndDraw();
    ReleaseDC(ctx->TargetWindow, hDC);
  }
  if (FAILED(SendMessage(ctx->TargetWindow, WM_DESTROY, 0, 0))) {
    Log->Fail(L"Failed to send message", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  Log->Info(L"End highlight paint loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  return S_OK;
}

DWORD WINAPI highlightLoop(LPVOID context) {
  Log->Info(L"Start highlight loop thread", GetCurrentThreadId(), __LONGFILE__);

  HighlightLoopContext *ctx = static_cast<HighlightLoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to initialize COM", GetCurrentThreadId(), __LONGFILE__);
    return hr;
  }

  HighlightPaintLoopContext *highlightPaintLoopCtx =
      new HighlightPaintLoopContext;

  highlightPaintLoopCtx->QuitEvent = ctx->QuitEvent;
  highlightPaintLoopCtx->PaintEvent = ctx->PaintEvent;

  HANDLE highlightPaintLoopThread =
      CreateThread(nullptr, 0, highlightPaintLoop,
                   static_cast<void *>(highlightPaintLoopCtx), 0, nullptr);

  if (highlightPaintLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  while (highlightPaintLoopCtx->IsActive) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    wchar_t *str = new wchar_t[256]{};

    HRESULT hr =
        StringCbPrintfW(str, 255, L"Screen=(%d,%d)", screenWidth, screenHeight);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to get system metrics", GetCurrentThreadId(),
                __LONGFILE__);
      return hr;
    }

    Log->Info(str, GetCurrentThreadId(), __LONGFILE__);

    WNDCLASSEX wndClass;
    HINSTANCE hInstance;
    HWND hWnd;
    MSG msg;

    char windowName[] = "ScreenReaderX Highlight Rectangle";
    char className[] = "MainWindowClass";
    char *menuName{};

    hInstance = GetModuleHandle(nullptr);
    menuName = MAKEINTRESOURCE(nullptr);

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = highlightWindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = menuName;
    wndClass.lpszClassName = className;
    wndClass.hIconSm = nullptr;

    if (!RegisterClassEx(&wndClass)) {
      Log->Fail(L"Failed to call RegisterClassEx", GetCurrentThreadId(),
                __LONGFILE__);
      return E_FAIL;
    }

    hWnd = CreateWindowEx(
        WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        wndClass.lpszClassName, windowName, WS_POPUP, 0, 0, screenWidth,
        screenHeight, nullptr, nullptr, hInstance, nullptr);

    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    highlightPaintLoopCtx->TargetWindow = hWnd;

    while (GetMessage(&msg, nullptr, 0, 0) != 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // msg.wParam;
  }

  WaitForSingleObject(highlightPaintLoopThread, INFINITE);
  SafeCloseHandle(&highlightPaintLoopThread);

  CoUninitialize();

  Log->Info(L"End highlight loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}

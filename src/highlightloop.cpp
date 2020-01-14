#include <cpplogger/cpplogger.h>
#include <windows.h>

#include <d2d1.h>
#include <wincodec.h>
#include <wincodecsdk.h>

#include "context.h"
#include "highlightloop.h"
#include "util.h"

extern Logger::Logger *Log;

ID2D1Factory *pD2d1Factory{};
ID2D1HwndRenderTarget *pRenderTarget{};

LRESULT CALLBACK highlightWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE: {
    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 128,
                               LWA_COLORKEY | LWA_ALPHA);
    CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
    HRESULT hr =
        D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);

    if (FAILED(hr)) {
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
      break;
    }

    ShowWindow(hWnd, SW_SHOW);
  } break;
  case WM_DESTROY: {
    SafeRelease(&pRenderTarget);
    SafeRelease(&pD2d1Factory);
    PostQuitMessage(0);
  } break;
  case WM_PAINT: {
    D2D1_SIZE_F targetSize = pRenderTarget->GetSize();
    PAINTSTRUCT paint;
    BeginPaint(hWnd, &paint);
    pRenderTarget->BeginDraw();
    D2D1_COLOR_F blackColor = {1.0f, 0.0f, 0.0f, 1.0f};
    pRenderTarget->Clear(blackColor);

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

    pRenderTarget->EndDraw();
    EndPaint(hWnd, &paint);
  }
    return false;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

struct HighlightPaintLoopContext {
  HANDLE QuitEvent = nullptr;
  HWND TargetWindow = nullptr;
};

DWORD WINAPI highlightPaintLoop(LPVOID context) {
  Log->Info(L"Start highlight paint loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  HighlightPaintLoopContext *ctx =
      static_cast<HighlightPaintLoopContext *>(context);

  if (ctx == nullptr) {
    return E_FAIL;
  }

  HWND hWnd = ctx->TargetWindow;
  float x{};
  float y{};
  bool isActive{true};

  while (isActive) {
    HANDLE waitArray[1] = {ctx->QuitEvent};
    DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, 0);

    if (waitResult == WAIT_OBJECT_0 + 0) {
      isActive = false;
      continue;
    }

    Sleep(100);

    if (pRenderTarget == nullptr) {
      continue;
    }

    D2D1_SIZE_F targetSize = pRenderTarget->GetSize();
    // PAINTSTRUCT paint;

    InvalidateRect(hWnd, nullptr, true);
    HDC hDC = GetDC(hWnd);
    pRenderTarget->BeginDraw();
    D2D1_COLOR_F blackColor = {1.0f, 0.0f, 0.0f, 1.0f};
    pRenderTarget->Clear(blackColor);

    ID2D1SolidColorBrush *pBrush{};
    pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.75f, 0.75f, 0.75f, 1.0f), &pBrush);

    if (pBrush != nullptr) {
      D2D1_POINT_2F center =
          D2D1::Point2F(targetSize.width / 2, targetSize.height / 2);
      D2D1_ROUNDED_RECT roundRect = D2D1::RoundedRect(
          D2D1::RectF(x, y, x + 80.0f, y + 80.0f), 16.0f, 16.0f);
      pRenderTarget->DrawRoundedRectangle(&roundRect, pBrush, 8.0f);
      pBrush->Release();
    }

    pRenderTarget->EndDraw();
    ReleaseDC(hWnd, hDC);

    x = x > 400.0f ? 0.0f : x + 5.0f;
    y = y > 300.0f ? 0.0f : y + 5.0f;
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
    return E_FAIL;
  }

  hWnd = CreateWindowEx(
      WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
      wndClass.lpszClassName, windowName, WS_POPUP, CW_USEDEFAULT,
      CW_USEDEFAULT, 640, 480, nullptr, nullptr, hInstance, nullptr);

  SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  HighlightPaintLoopContext *highlightPaintLoopCtx =
      new HighlightPaintLoopContext;

  highlightPaintLoopCtx->TargetWindow = hWnd;
  highlightPaintLoopCtx->QuitEvent = ctx->QuitEvent;

  HANDLE highlightPaintLoopThread =
      CreateThread(nullptr, 0, highlightPaintLoop,
                   static_cast<void *>(highlightPaintLoopCtx), 0, nullptr);

  if (highlightPaintLoopThread == nullptr) {
    return E_FAIL;
  }
  while (GetMessage(&msg, nullptr, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  CoUninitialize();

  Log->Info(L"End highlight loop thread", GetCurrentThreadId(), __LONGFILE__);

  return msg.wParam;
}

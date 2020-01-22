#include <cstring>
#include <windows.h>

#include <d2d1.h>
#include <dwrite.h>
#include <strsafe.h>

#include "context.h"
#include "textrender.h"
#include "util.h"

TextViewerLoopContext *tvlCtx{};

int windowWidth{};
int windowHeight{};

ID2D1Factory *pTextViewerD2d1Factory{};
IDWriteFactory *pTextViewerDWFactory{};
ID2D1HwndRenderTarget *pTextViewerRenderTarget{};

HRESULT drawTextViewer() {
  D2D1_COLOR_F redColor = {1.0f, 0.0f, 0.0f, 1.0f};
  pTextViewerRenderTarget->Clear(redColor);

  ID2D1SolidColorBrush *pBackgroundBrush{};
  pTextViewerRenderTarget->CreateSolidColorBrush(
      D2D1::ColorF(0.125f, 0.125f, 0.125f, 1.0f), &pBackgroundBrush);

  if (pBackgroundBrush == nullptr) {
    return E_FAIL;
  }

  ID2D1SolidColorBrush *pBorderBrush{};
  pTextViewerRenderTarget->CreateSolidColorBrush(
      D2D1::ColorF(0.875f, 0.875f, 0.875f, 1.0f), &pBorderBrush);

  if (pBorderBrush == nullptr) {
    return E_FAIL;
  }

  D2D1_ROUNDED_RECT roundRect = D2D1::RoundedRect(
      D2D1::RectF(4.0f, 4.0f, windowWidth - 8.0f, windowHeight - 8.0f), 8.0f,
      8.0f);

  pTextViewerRenderTarget->FillRoundedRectangle(&roundRect, pBackgroundBrush);
  pBackgroundBrush->Release();

  pTextViewerRenderTarget->DrawRoundedRectangle(&roundRect, pBorderBrush, 2.0f);
  pBorderBrush->Release();

  ID2D1SolidColorBrush *pBrush{};
  pTextViewerRenderTarget->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::White), &pBrush);

  IDWriteTextFormat *pTextFormat{};

  pTextViewerDWFactory->CreateTextFormat(
      L"Meiryo", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, 24, L"", &pTextFormat);

  if (pBrush != nullptr && pTextFormat != nullptr) {
    pTextViewerRenderTarget->DrawText(
        tvlCtx->TextToDraw, std::wcslen(tvlCtx->TextToDraw), pTextFormat,
        &D2D1::RectF(32, 32, 300, 64), pBrush);
  }

  SafeRelease(&pTextFormat);
  SafeRelease(&pBrush);

  return S_OK;
}

LRESULT CALLBACK textViewerWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE: {
    CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
                                   &pTextViewerD2d1Factory);

    if (FAILED(hr)) {
      break;
    }

    D2D1_SIZE_U pixelSize = {static_cast<UINT32>(createStruct->cx),
                             static_cast<UINT32>(createStruct->cy)};

    D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties =
        D2D1::RenderTargetProperties();

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndRenderTargetProperties =
        D2D1::HwndRenderTargetProperties(hWnd, pixelSize);

    hr = pTextViewerD2d1Factory->CreateHwndRenderTarget(
        renderTargetProperties, hwndRenderTargetProperties,
        &pTextViewerRenderTarget);

    if (FAILED(hr)) {
      break;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown **>(&pTextViewerDWFactory));

    if (FAILED(hr)) {
      break;
    }

    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hWnd, SW_SHOW);
  } break;
  case WM_DISPLAYCHANGE: {
    DestroyWindow(hWnd);
  } break;
  case WM_DESTROY: {
    SafeRelease(&pTextViewerDWFactory);
    SafeRelease(&pTextViewerRenderTarget);
    SafeRelease(&pTextViewerD2d1Factory);
    ::PostQuitMessage(0);
  } break;
  case WM_CLOSE: {
    // Ignore this message.
  }
    return 0;
  case WM_PAINT: {
    D2D1_SIZE_F targetSize = pTextViewerRenderTarget->GetSize();
    PAINTSTRUCT paintStruct;
    BeginPaint(hWnd, &paintStruct);
    pTextViewerRenderTarget->BeginDraw();

    if (FAILED(drawTextViewer())) {
      // todo
    }

    pTextViewerRenderTarget->EndDraw();
    EndPaint(hWnd, &paintStruct);
  }
    return 0;
  }

  return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

struct TextViewerPaintLoopContext {
  HANDLE QuitEvent = nullptr;
  HANDLE PaintEvent = nullptr;
  HWND TargetWindow = nullptr;
  bool IsActive = true;
};

DWORD WINAPI textViewerPaintLoop(LPVOID context) {
  TextViewerPaintLoopContext *ctx =
      static_cast<TextViewerPaintLoopContext *>(context);

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
    if (ctx->TargetWindow == nullptr) {
      continue;
    }
    if (pTextViewerRenderTarget == nullptr ||
        pTextViewerD2d1Factory == nullptr) {
      continue;
    }

    InvalidateRect(ctx->TargetWindow, nullptr, true);
    HDC hDC = GetDC(ctx->TargetWindow);
    pTextViewerRenderTarget->BeginDraw();

    if (FAILED(drawTextViewer())) {
      // todo
    }

    pTextViewerRenderTarget->EndDraw();
    ReleaseDC(ctx->TargetWindow, hDC);
  }
  if (FAILED(SendMessage(ctx->TargetWindow, WM_DESTROY, 0, 0))) {
    return E_FAIL;
  }

  return S_OK;
}

DWORD WINAPI textViewerLoop(LPVOID context) {
  tvlCtx = static_cast<TextViewerLoopContext *>(context);

  if (tvlCtx == nullptr) {
    return E_FAIL;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    return hr;
  }

  TextViewerPaintLoopContext *textViewerPaintLoopCtx =
      new TextViewerPaintLoopContext;

  textViewerPaintLoopCtx->QuitEvent = tvlCtx->QuitEvent;
  textViewerPaintLoopCtx->PaintEvent = tvlCtx->PaintEvent;

  HANDLE textViewerPaintLoopThread =
      CreateThread(nullptr, 0, textViewerPaintLoop,
                   static_cast<void *>(textViewerPaintLoopCtx), 0, nullptr);

  if (textViewerPaintLoopThread == nullptr) {
    return E_FAIL;
  }

  const int defaultWindowWidth{640};
  const int defaultWindowHeight{80};

  while (textViewerPaintLoopCtx->IsActive) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    windowWidth =
        screenWidth > defaultWindowWidth ? defaultWindowWidth : screenWidth;
    windowHeight = defaultWindowHeight;

    int windowLeft =
        screenWidth > (windowWidth - 32) ? screenWidth - windowWidth - 32 : 0;
    int windowTop = screenHeight - windowHeight - 48;
    wchar_t *buffer = new wchar_t[256]{};

    HRESULT hr = StringCbPrintfW(buffer, 255,
                                 L"Screen: {width:%d, height:%d}, Window: "
                                 L"{left:%d, top:%d, width:%d, height:%d}",
                                 screenWidth, screenHeight, windowLeft,
                                 windowTop, windowWidth, windowHeight);

    if (FAILED(hr)) {
      break;
    }

    delete[] buffer;
    buffer = nullptr;

    WNDCLASSEX wndClass;
    HINSTANCE hInstance;
    HWND hWnd;
    MSG msg;

    char windowName[] = "ScreenReaderX TextViewer Window";
    char className[] = "MainTextViewerWindowClass";
    char *menuName{};

    hInstance = GetModuleHandle(nullptr);
    menuName = MAKEINTRESOURCE(nullptr);

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = textViewerWindowProc;
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
      break;
    }

    hWnd = CreateWindowEx(
        WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        wndClass.lpszClassName, windowName, WS_POPUP, windowLeft, windowTop,
        windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    textViewerPaintLoopCtx->TargetWindow = hWnd;

    while (GetMessage(&msg, nullptr, 0, 0) != 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    if (UnregisterClassA(className, hInstance) == 0) {
      break;
    }
  }

  WaitForSingleObject(textViewerPaintLoopThread, INFINITE);
  SafeCloseHandle(&textViewerPaintLoopThread);

  CoUninitialize();

  return S_OK;
}

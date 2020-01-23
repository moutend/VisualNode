#include <cpplogger/cpplogger.h>
#include <cstring>
#include <windows.h>

#include <d2d1.h>
#include <dwrite.h>
#include <strsafe.h>

#include "context.h"
#include "textviewerloop.h"
#include "util.h"

extern Logger::Logger *Log;

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
  pTextViewerRenderTarget->DrawRoundedRectangle(&roundRect, pBorderBrush, 2.0f);

  SafeRelease(&pBackgroundBrush);
  SafeRelease(&pBorderBrush);

  return S_OK;
}

HRESULT drawText(wchar_t *text) {
  HRESULT hr{};

  size_t bufferLen{36};
  wchar_t *buffer = new wchar_t[37]{};

  if (std::wcslen(text) <= 32) {
    bufferLen = std::wcslen(text);
    std::wmemcpy(buffer, text, bufferLen);
  } else {
    wchar_t *b = new wchar_t[33]{};
    std::wmemcpy(b, text, 32);

    hr = StringCbPrintfW(buffer, 36, L"%s ...", b);

    delete[] b;
    b = nullptr;
  }
  if (FAILED(hr)) {
    return hr;
  }

  ID2D1SolidColorBrush *pBrush{};

  pTextViewerRenderTarget->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::White), &pBrush);

  if (pBrush == nullptr) {
    return E_FAIL;
  }

  IDWriteTextFormat *pTextFormat{};

  pTextViewerDWFactory->CreateTextFormat(
      L"Meiryo", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, 16, L"", &pTextFormat);

  if (pTextFormat == nullptr) {
    return E_FAIL;
  }

  pTextViewerRenderTarget->DrawText(
      buffer, bufferLen, pTextFormat,
      &D2D1::RectF(32, 32, windowWidth - 32, windowHeight - 32), pBrush);

  SafeRelease(&pTextFormat);
  SafeRelease(&pBrush);

  wchar_t *message = new wchar_t[256]{};

  hr = StringCbPrintfW(message, 255, L"Showing '%s'", buffer);

  if (FAILED(hr)) {
    return hr;
  }

  Log->Info(message, GetCurrentThreadId(), __LONGFILE__);

  delete[] buffer;
  buffer = nullptr;

  return S_OK;
}

LRESULT CALLBACK textViewerWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE: {
    Log->Info(L"Received WM_CREATE", GetCurrentThreadId(), __LONGFILE__);

    CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
                                   &pTextViewerD2d1Factory);

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

    hr = pTextViewerD2d1Factory->CreateHwndRenderTarget(
        renderTargetProperties, hwndRenderTargetProperties,
        &pTextViewerRenderTarget);

    if (FAILED(hr)) {
      Log->Fail(L"Failed to call CreateHwndRenderTarget", GetCurrentThreadId(),
                __LONGFILE__);
      break;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown **>(&pTextViewerDWFactory));

    if (FAILED(hr)) {
      Log->Fail(L"Failed to call DWriteCreateFactory", GetCurrentThreadId(),
                __LONGFILE__);
      break;
    }

    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hWnd, SW_SHOW);
  } break;
  case WM_DISPLAYCHANGE: {
    Log->Info(L"Received WM_DISPLAYCHANGE", GetCurrentThreadId(), __LONGFILE__);

    DestroyWindow(hWnd);
  } break;
  case WM_DESTROY: {
    Log->Info(L"Received WM_DESTROY", GetCurrentThreadId(), __LONGFILE__);

    SafeRelease(&pTextViewerDWFactory);
    SafeRelease(&pTextViewerRenderTarget);
    SafeRelease(&pTextViewerD2d1Factory);
    ::PostQuitMessage(0);
  } break;
  case WM_CLOSE: {
    Log->Info(L"Received WM_CLOSE", GetCurrentThreadId(), __LONGFILE__);

    // Ignore this message.
  }
    return 0;
  case WM_PAINT: {
    Log->Info(L"Received WM_PAINT", GetCurrentThreadId(), __LONGFILE__);

    if (pTextViewerRenderTarget == nullptr ||
        pTextViewerD2d1Factory == nullptr) {
      Log->Fail(L"Direct2D painting is not available", GetCurrentThreadId(),
                __LONGFILE__);
      break;
    }

    D2D1_SIZE_F targetSize = pTextViewerRenderTarget->GetSize();
    PAINTSTRUCT paintStruct;
    BeginPaint(hWnd, &paintStruct);
    pTextViewerRenderTarget->BeginDraw();

    HRESULT hr = drawTextViewer();

    if (FAILED(hr)) {
      Log->Fail(L"Failed to paint text viewer", GetCurrentThreadId(),
                __LONGFILE__);
    } else {
      hr = drawText(tvlCtx->TextToDraw);
    }
    if (FAILED(hr)) {
      Log->Fail(L"Failed to paint text", GetCurrentThreadId(), __LONGFILE__);
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
      Log->Fail(L"Text viewer window is not created", GetCurrentThreadId(),
                __LONGFILE__);
      continue;
    }
    if (pTextViewerRenderTarget == nullptr ||
        pTextViewerD2d1Factory == nullptr) {
      Log->Fail(L"Direct2D painting is not available", GetCurrentThreadId(),
                __LONGFILE__);
      continue;
    }

    InvalidateRect(ctx->TargetWindow, nullptr, true);
    HDC hDC = GetDC(ctx->TargetWindow);
    pTextViewerRenderTarget->BeginDraw();

    HRESULT hr = drawTextViewer();

    if (FAILED(hr)) {
      Log->Fail(L"Failed to paint text viewer", GetCurrentThreadId(),
                __LONGFILE__);
    } else {
      hr = drawText(tvlCtx->TextToDraw);
    }
    if (FAILED(hr)) {
      Log->Fail(L"Failed to paint text", GetCurrentThreadId(), __LONGFILE__);
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
  Log->Info(L"Start text viewer loop thread", GetCurrentThreadId(),
            __LONGFILE__);

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

    Log->Info(buffer, GetCurrentThreadId(), __LONGFILE__);

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

    hWnd = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_COMPOSITED | WS_EX_LAYERED |
                              WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                          wndClass.lpszClassName, windowName, WS_POPUP,
                          windowLeft, windowTop, windowWidth, windowHeight,
                          nullptr, nullptr, hInstance, nullptr);

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

  Log->Info(L"End text viewer loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}

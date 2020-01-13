#include <windows.h>

#include <d2d1.h>
#include <wincodec.h>
#include <wincodecsdk.h>

#include "util.h"

ID2D1Factory *pD2d1Factory{};
ID2D1HwndRenderTarget *pRenderTarget{};

LRESULT CALLBACK mainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE: {
    CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
    HRESULT hr =
        D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2d1Factory);

    if (FAILED(hr)) {
      break;
    }

    D2D1_SIZE_U pixelSize = {createStruct->cx, createStruct->cy};
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
    D2D1_COLOR_F blackColor = {1.0f, 1.0f, 1.0f, 1.0f};
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

    pRenderTarget->EndDraw();
    EndPaint(hWnd, &paint);
  }
    return false;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int main(Platform::Array<Platform::String ^> ^ args) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    return hr;
  }

  WNDCLASSEX wndClass;
  HINSTANCE hInstance;
  HWND hWnd;
  MSG msg;

  char windowName[] = "Rendering Rounded Rectangle";
  char className[] = "MainWindowClass";
  char *menuName{};

  hInstance = GetModuleHandle(nullptr);
  menuName = MAKEINTRESOURCE(nullptr);

  wndClass.cbSize = sizeof(WNDCLASSEX);
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = mainWindowProc;
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
    return -1;
  }

  hWnd = CreateWindowEx(
      WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
      wndClass.lpszClassName, windowName, WS_POPUP, CW_USEDEFAULT,
      CW_USEDEFAULT, 640, 480, nullptr, nullptr, hInstance, nullptr);

  SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  while (GetMessage(&msg, nullptr, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  CoUninitialize();

  return msg.wParam;
}

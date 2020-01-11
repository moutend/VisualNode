// #include <locale.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include <gdiplus.h>
#include <wingdi.h>

void drawRoundRect(Gdiplus::Graphics &graphics, Gdiplus::RectF *pRectF,
                   Gdiplus::Pen *pPen) {
  Gdiplus::REAL startAngle = 180;
  Gdiplus::REAL sweepAngle = 90;
  Gdiplus::REAL radius = 48;
  Gdiplus::REAL left = pRectF->X;
  Gdiplus::REAL top = pRectF->Y;
  Gdiplus::REAL right = pRectF->GetRight();
  Gdiplus::REAL bottom = pRectF->GetBottom();
  Gdiplus::GraphicsPath graphicsPath;

  graphicsPath.AddArc(left, top, radius, radius, startAngle, sweepAngle);

  startAngle += 90;

  graphicsPath.AddArc(right - radius, top, radius, radius, startAngle,
                      sweepAngle);

  startAngle += 90;

  graphicsPath.AddArc(right - radius, bottom - radius, radius, radius,
                      startAngle, sweepAngle);

  startAngle += 90;

  graphicsPath.AddArc(left, bottom - radius, radius, radius, startAngle,
                      sweepAngle);

  graphicsPath.CloseAllFigures();

  graphics.DrawPath(pPen, &graphicsPath);
}

LRESULT CALLBACK mainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE:
    CREATESTRUCT *tpCreateSt = (CREATESTRUCT *)lParam;
    ShowWindow(hWnd, SW_SHOW);
    break;
  case WM_DESTROY: {
    PostQuitMessage(0);
  } break;
  case WM_PAINT:
    PAINTSTRUCT tPaintStruct;
    HDC hDC = BeginPaint(hWnd, &tPaintStruct);
    Gdiplus::Graphics oGraphics(hDC);
    Gdiplus::Pen oPen(Gdiplus::Color(255, 0, 0), 3);
    Gdiplus::RectF oRectF(10, 10, 600, 420);
    DrawRoundRect(oGraphics, &oRectF, &oPen);
    EndPaint(hWnd, &tPaintStruct);
    return false;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int main(Platform::Array<Platform::String ^> ^ args) {
  // setlocale(LC_ALL, "Japanese");
  WNDCLASSEX wndClass;
  HINSTANCE hInstance;
  HWND hWnd;
  MSG msg;

  wchar_t className[] = L"MainWindowClass";
  wchar_t windowName[] = L"Rendering Rounded Rectangle";
  wchar_t *menuName{};

  ULONG_PTR gdiplusToken{};
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);

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

  hWnd = CreateWindowEx(0, wndClass.lpszClassName, windowName,
                        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640,
                        480, nullptr, nullptr, hInstance, nullptr);

  while (GetMessage(&msg, nullptr, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Gdiplus::GdiplusShutdown(gdiplusToken);

  return msg.wParam;
}

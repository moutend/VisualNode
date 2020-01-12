#include <windows.h>

#include <gdiplus.h>
#include <wingdi.h>

void drawRoundRect(Gdiplus::Graphics &graphics, Gdiplus::RectF *pRectF,
                   Gdiplus::Pen *pPen) {
  Gdiplus::REAL startAngle = 180;
  Gdiplus::REAL sweepAngle = 90;
  Gdiplus::REAL radius = 24;
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
  case WM_CREATE: {
    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 64, LWA_COLORKEY);
    ShowWindow(hWnd, SW_SHOW);
  } break;
  case WM_DESTROY: {
    PostQuitMessage(0);
  } break;
  case WM_PAINT: {
    PAINTSTRUCT paint;
    HDC hDC = BeginPaint(hWnd, &paint);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
    SelectObject(hDC, hBrush);
    ExtFloodFill(hDC, 0, 0, RGB(255, 255, 255), FLOODFILLSURFACE);
    Gdiplus::Graphics graphics(hDC);
    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 255), 8);
    Gdiplus::RectF rectF(0, 0, 50, 50);
    drawRoundRect(graphics, &rectF, &pen);
    EndPaint(hWnd, &paint);
  }
    return false;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

struct AnimationLoopContext {
  HWND TargetWindow = nullptr;
};

DWORD WINAPI animationLoop(LPVOID context) {
  AnimationLoopContext *ctx = static_cast<AnimationLoopContext *>(context);

  if (ctx == nullptr) {
    return E_FAIL;
  }

  int x{};
  int y{};
  HWND hWnd = ctx->TargetWindow;

  while (true) {
    HDC hDC = GetDC(hWnd);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
    SelectObject(hDC, hBrush);
    ExtFloodFill(hDC, 0, 0, RGB(255, 255, 255), FLOODFILLSURFACE);
    Gdiplus::Graphics graphics(hDC);
    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 255), 8);
    Gdiplus::RectF rectF(x, y, 50, 50);
    drawRoundRect(graphics, &rectF, &pen);
    ReleaseDC(hWnd, hDC);

    x = x > 500 ? 0 : x = +10;
    y = y > 300 ? 0 : y + 10;

    Sleep(200);
  }
  return S_OK;
}

int main(Platform::Array<Platform::String ^> ^ args) {
  WNDCLASSEX wndClass;
  HINSTANCE hInstance;
  HWND hWnd;
  MSG msg;

  char windowName[] = "Rendering Rounded Rectangle";
  char className[] = "MainWindowClass";
  char *menuName{};

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

  hWnd = CreateWindowEx(
      WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
      wndClass.lpszClassName, windowName, WS_POPUP, CW_USEDEFAULT,
      CW_USEDEFAULT, 640, 480, nullptr, nullptr, hInstance, nullptr);

  SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

  AnimationLoopContext *animationLoopContext = new AnimationLoopContext;
  animationLoopContext->TargetWindow = hWnd;

  HANDLE animationThread =
      CreateThread(nullptr, 0, animationLoop,
                   static_cast<void *>(animationLoopContext), 0, nullptr);

  if (animationThread == nullptr) {
    return -1;
  }
  while (GetMessage(&msg, nullptr, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Gdiplus::GdiplusShutdown(gdiplusToken);

  return msg.wParam;
}

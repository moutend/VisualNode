#include <cpplogger/cpplogger.h>
#include <windows.h>

#include "context.h"
#include "textviewerloop.h"
#include "util.h"

extern Logger::Logger *Log;

DWORD WINAPI textViewerLoop(LPVOID context) {
  Log->Info(L"Start text viewer loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  TextViewerLoopContext *ctx = static_cast<TextViewerLoopContext *>(context);

  if (ctx == nullptr) {
    Log->Fail(L"Failed to obtain ctx", GetCurrentThreadId(), __LONGFILE__);
    return E_FAIL;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  if (FAILED(hr)) {
    Log->Fail(L"Failed to initialize COM", GetCurrentThreadId(), __LONGFILE__);
    return hr;
  }

  CoUninitialize();

  Log->Info(L"End text viewer loop thread", GetCurrentThreadId(), __LONGFILE__);

  return S_OK;
}

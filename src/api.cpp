#include <cpplogger/cpplogger.h>
#include <cstring>
#include <mutex>
#include <windows.h>

#include <strsafe.h>

#include "api.h"
#include "context.h"
#include "highlightloop.h"
#include "logloop.h"
#include "util.h"

extern Logger::Logger *Log;

bool isActive{false};
std::mutex apiMutex;

LogLoopContext *logLoopCtx{nullptr};
HighlightLoopContext *highlightLoopCtx{nullptr};

HANDLE logLoopThread{nullptr};
HANDLE highlightLoopThread{nullptr};

void __stdcall Setup(int32_t *code, int32_t logLevel) {
  std::lock_guard<std::mutex> lock(apiMutex);

  if (code == nullptr) {
    return;
  }
  if (isActive) {
    Log->Warn(L"Already initialized", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  isActive = true;

  Log = new Logger::Logger(L"VisualNode", L"v0.1.0-develop", 4096);

  Log->Info(L"Setup VisualNode", GetCurrentThreadId(), __LONGFILE__);

  logLoopCtx = new LogLoopContext();

  logLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (logLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Create log loop thread", GetCurrentThreadId(), __LONGFILE__);

  logLoopThread = CreateThread(nullptr, 0, logLoop,
                               static_cast<void *>(logLoopCtx), 0, nullptr);

  if (logLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  highlightLoopCtx = new HighlightLoopContext();

  highlightLoopCtx->HighlightRectangle = new HighlightRectangle;
  highlightLoopCtx->QuitEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (highlightLoopCtx->QuitEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }
  highlightLoopCtx->PaintEvent =
      CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

  if (highlightLoopCtx->PaintEvent == nullptr) {
    Log->Fail(L"Failed to create event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Create highlight loop thread", GetCurrentThreadId(),
            __LONGFILE__);

  highlightLoopThread =
      CreateThread(nullptr, 0, highlightLoop,
                   static_cast<void *>(highlightLoopCtx), 0, nullptr);

  if (highlightLoopThread == nullptr) {
    Log->Fail(L"Failed to create thread", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  Log->Info(L"Complete setup VisualNode", GetCurrentThreadId(), __LONGFILE__);
}

void __stdcall Teardown(int32_t *code) {
  std::lock_guard<std::mutex> lock(apiMutex);

  if (code == nullptr) {
    return;
  }
  if (!isActive) {
    *code = -1;
    return;
  }

  Log->Info(L"Teardown VisualNode", GetCurrentThreadId(), __LONGFILE__);

  if (highlightLoopThread == nullptr) {
    goto END_HIGHLIGHTLOOP_CLEANUP;
  }
  if (!SetEvent(highlightLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  WaitForSingleObject(highlightLoopThread, INFINITE);
  SafeCloseHandle(&highlightLoopThread);

  SafeCloseHandle(&(highlightLoopCtx->QuitEvent));

  delete highlightLoopCtx->HighlightRect;
  highlightLoopCtx->HighlightRect = nullptr;

  delete highlightLoopCtx;
  highlightLoopCtx = nullptr;

  Log->Info(L"Delete highlight loop thread", GetCurrentThreadId(),
            __LONGFILE__);

END_HIGHLIGHTLOOP_CLEANUP:

  Log->Info(L"Complete teardown AudioNode", GetCurrentThreadId(), __LONGFILE__);

  if (logLoopThread == nullptr) {
    goto END_LOGLOOP_CLEANUP;
  }
  if (!SetEvent(logLoopCtx->QuitEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }

  WaitForSingleObject(logLoopThread, INFINITE);
  SafeCloseHandle(&logLoopThread);

  SafeCloseHandle(&(logLoopCtx->QuitEvent));

  delete logLoopCtx;
  logLoopCtx = nullptr;

END_LOGLOOP_CLEANUP:

  isActive = false;
}

void __stdcall SetHighlightRectangle(int32_t *code, HighlightRectangle *rect) {
  std::lock_guard<std::mutex> lock(apiMutex);

  if (code == nullptr || rect == nullptr) {
    return;
  }
  if (!isActive) {
    *code = -1;
    return;
  }

  Log->Info(L"Called SetHighlightRectangle()", GetCurrentThreadId(),
            __LONGFILE__);

  highlightLoopCtx->HighlightRect->Left = rect->Left;
  highlightLoopCtx->HighlightRect->Top = rect->Top;
  highlightLoopCtx->HighlightRect->Width = rect->Width;
  highlightLoopCtx->HighlightRect->Height = rect->Height;
  highlightLoopCtx->HighlightRect->Radius = rect->Radius;
  highlightLoopCtx->HighlightRect->BorderThickness = rect->BorderThickness;

  if (!SetEvent(highlightLoopCtx->PaintEvent)) {
    Log->Fail(L"Failed to send event", GetCurrentThreadId(), __LONGFILE__);
    *code = -1;
    return;
  }
}

void __stdcall ClearHighlightRectangle(int32_t *code) {
  std::lock_guard<std::mutex> lock(apiMutex);

  if (code == nullptr) {
    return;
  }
  if (!isActive) {
    *code = -1;
    return;
  }

  Log->Info(L"Called ClearHighlightRectangle()", GetCurrentThreadId(),
            __LONGFILE__);

  // TODO: implement me!
}

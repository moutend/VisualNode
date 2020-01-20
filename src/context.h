#pragma once

#include <windows.h>

#include "types.h"

struct LogLoopContext {
  HANDLE QuitEvent = nullptr;
};

struct HighlightLoopContext {
  HANDLE QuitEvent = nullptr;
  HANDLE PaintEvent = nullptr;
  HighlightRectangle *HighlightRect = nullptr;
};

struct TextViewerLoopContext {
  HANDLE QuitEvent = nullptr;
  HANDLE PaintEvent = nullptr;
  wchar_t *Text = nullptr;
};

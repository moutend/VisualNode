#pragma once

#include <windows.h>

#include "types.h"

struct LogLoopContext {
  HANDLE QuitEvent = nullptr;
};

struct HighlightLoopContext {
  HANDLE QuitEvent = nullptr;
};

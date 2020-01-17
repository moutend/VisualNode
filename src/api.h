#pragma once

#include <cstdint>

#include "types.h"

#define export __declspec(dllexport)

extern "C" {
export void __stdcall Setup(int32_t *code, int32_t logLevel);
export void __stdcall Teardown(int32_t *code);

export void __stdcall SetHighlightRectangle(int32_t *code,
                                            HighlightRectangle *rect);
export void __stdcall ClearHighlightRectangle(int32_t *code);
}

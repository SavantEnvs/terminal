// mayhem shim: MSVC CRT ISA levels. Row.cpp selects its AVX2 path by
// __isa_available; the shim reports the x86-64 baseline (SSE2), see win32_shim.cpp.
#pragma once
#define __ISA_AVAILABLE_X86 0
#define __ISA_AVAILABLE_SSE2 1
#define __ISA_AVAILABLE_SSE42 2
#define __ISA_AVAILABLE_AVX 3
#define __ISA_AVAILABLE_ENFSTRG 4
#define __ISA_AVAILABLE_AVX2 5

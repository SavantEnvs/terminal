// mayhem shim: replaces src/inc/LibraryIncludes.h for the Linux/clang fuzz build.
// Pulls in the STL, GSL, chromium safe_math and TIL like the original, but swaps
// windows.h/WIL/WRL/TraceLogging for the minimal portable stand-ins in mayhem_win32.h.
#pragma once
#include <climits>
#include <cwchar>
#include <cwctype>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <deque>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory_resource>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <utility>
#include <vector>

#include "mayhem_win32.h"

#include <gsl/narrow>
#include <gsl/util>
#include <gsl/pointers>
#include <gsl/span>

#include <base/numerics/safe_math.h>

#include <fmt/compile.h>
#include <fmt/xchar.h>
// On Linux std::filesystem::path::native() is a narrow std::string, and utils.cpp's
// WSL command-line helper (not reachable from any fuzz target) formats it into a wide
// compiled format string, which fmt rejects. This more specialized overload widens it.
namespace fmt
{
    template<typename S, typename A, typename B, std::enable_if_t<fmt::is_compiled_string<S>::value, int> = 0>
    std::wstring format(const S& fmtStr, const std::string& narrow, A&& a, B&& b)
    {
        const std::wstring wide(narrow.begin(), narrow.end());
        return fmt::format(fmt::runtime(fmt::basic_string_view<wchar_t>(fmtStr)), wide, std::forward<A>(a), std::forward<B>(b));
    }
}

#ifndef BLOCK_TIL
#include "til.h"
#endif

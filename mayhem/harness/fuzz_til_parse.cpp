// Fuzz target: TIL string helpers from src/inc/til/string.h — til::parse_unsigned /
// til::parse_signed (used for VT and OSC parameters), split_iterator, and the
// clean_filename / visualize_control_codes transforms — over both narrow and wide
// strings. The first byte selects the base (0 = auto-detect, 2..36) and the separator.
#include "../../src/terminal/parser/precomp.h"

#include <til/string.h>

template<typename T>
static void exercise(std::basic_string_view<T> str, int base, T sep)
{
    if (const auto u = til::parse_unsigned<uint64_t>(str, base))
    {
        (void)*u;
    }
    if (const auto u = til::parse_unsigned<uint16_t>(str, base))
    {
        (void)*u;
    }
    if (const auto s = til::parse_signed<int64_t>(str, base))
    {
        (void)*s;
    }
    if (const auto s = til::parse_signed<int32_t>(str, base))
    {
        (void)*s;
    }
    for (const auto& tok : til::split_iterator{ str, sep })
    {
        (void)til::parse_unsigned<size_t>(tok, 10);
        (void)til::parse_signed<int8_t>(tok, base);
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 2)
    {
        return -1;
    }
    static constexpr int bases[] = { 0, 2, 8, 10, 16, 36, 1, 37 };
    const int base = bases[data[0] & 7];
    const char sep = static_cast<char>(data[1]);
    const std::string_view narrow{ reinterpret_cast<const char*>(data + 2), size - 2 };
    exercise<char>(narrow, base, sep);

    std::wstring wide;
    wide.reserve(narrow.size() / 2);
    for (size_t i = 0; i + 1 < narrow.size(); i += 2)
    {
        wide.push_back(static_cast<wchar_t>(static_cast<uint8_t>(narrow[i]) | (static_cast<uint8_t>(narrow[i + 1]) << 8)));
    }
    exercise<wchar_t>(wide, base, static_cast<wchar_t>(sep));
    (void)til::clean_filename(wide);
    (void)til::visualize_control_codes(std::wstring_view{ wide });
    return 0;
}

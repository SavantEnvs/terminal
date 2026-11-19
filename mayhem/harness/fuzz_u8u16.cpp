// Fuzz target: til::u8u16 / til::u16u8 (src/inc/til/u8u16convert.h), the chunked
// UTF-8 <-> UTF-16 conversion with partial-sequence state that every byte of ConPTY /
// connection output goes through. The input is split into chunks (sizes from the
// first byte) and converted with a persistent u8state, then back with a u16state.
#include "../../src/terminal/parser/precomp.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
    {
        return -1;
    }
    const size_t chunk = 1 + (data[0] & 0x1f);
    const std::string_view in{ reinterpret_cast<const char*>(data + 1), size - 1 };

    til::u8state s8{};
    std::wstring wide, part;
    for (size_t i = 0; i < in.size(); i += chunk)
    {
        if (SUCCEEDED(til::u8u16(in.substr(i, chunk), part, s8)))
        {
            wide += part;
        }
    }

    til::u16state s16{};
    std::string narrow, part8;
    const std::wstring_view w{ wide };
    for (size_t i = 0; i < w.size(); i += chunk)
    {
        if (SUCCEEDED(til::u16u8(w.substr(i, chunk), part8, s16)))
        {
            narrow += part8;
        }
    }

    // One-shot conversions, as used for whole strings.
    std::wstring oneShot;
    (void)til::u8u16(in, oneShot);
    std::string back;
    (void)til::u16u8(oneShot, back);
    return 0;
}

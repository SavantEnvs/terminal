// Fuzz target: CodepointWidthDetector (src/types/CodepointWidthDetector.cpp) — grapheme
// cluster segmentation and column widths that every printed string goes through.
// Input bytes are UTF-16LE code units (unpaired surrogates included). The first byte
// picks the measurement mode and ambiguous width; the string is walked forwards with
// GraphemeNext and backwards with GraphemePrev.
#include "../../src/types/precomp.h"

#include "../../src/types/inc/CodepointWidthDetector.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
    {
        return -1;
    }
    static constexpr TextMeasurementMode modes[] = { TextMeasurementMode::Graphemes, TextMeasurementMode::Wcswidth, TextMeasurementMode::Console };
    auto& cwd = CodepointWidthDetector::Singleton();
    // The detector is a process-wide singleton: reset it for every input.
    cwd.Reset(modes[(data[0] & 3) % 3]);
    cwd.SetAmbiguousWidth((data[0] & 4) ? 2 : 1);

    std::wstring text;
    for (size_t i = 1; i + 1 < size; i += 2)
    {
        text.push_back(static_cast<wchar_t>(data[i] | (data[i + 1] << 8)));
    }
    const std::wstring_view view{ text };

    GraphemeState fwd{};
    size_t total = 0;
    while (cwd.GraphemeNext(fwd, view))
    {
        total += static_cast<size_t>(fwd.width);
    }
    GraphemeState back{};
    while (cwd.GraphemePrev(back, view))
    {
        total += static_cast<size_t>(back.width);
    }
    cwd.Reset(TextMeasurementMode::Graphemes);
    return static_cast<int>(total & 0);
}

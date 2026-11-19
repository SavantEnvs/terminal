// Fuzz target: Windows Terminal's VT *input* parser (InputStateMachineEngine) — the
// sequences a terminal sends back to conhost through ConPTY: CSI/SS3 keys, SGR mouse,
// win32-input-mode, focus events, DA1/DSR responses. UTF-8 input is converted with
// til::u8u16 and fed through StateMachine in chunks sized by the first byte.
#include "vt_dispatch.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
    {
        return -1;
    }
    const size_t chunk = 1 + (data[0] & 0x3f) * 16;
    const auto text = mayhem::Utf8ToUtf16(data + 1, size - 1);

    mayhem::EventLog log;
    auto engine = std::make_unique<Microsoft::Console::VirtualTerminal::InputStateMachineEngine>(
        std::make_unique<mayhem::InputDispatch>(log));
    if (data[0] & 0x40)
    {
        engine->CaptureNextCursorPositionReport();
    }
    // Use the explicit (engine, isEngineForInput) constructor: the templated one detects
    // the input engine with std::is_same_v<T, class InputStateMachineEngine>, which
    // clang resolves to a different (class-scoped) type, so it would parse as output.
    Microsoft::Console::VirtualTerminal::StateMachine sm{ std::unique_ptr<Microsoft::Console::VirtualTerminal::IStateMachineEngine>{ std::move(engine) }, true };

    std::wstring_view rest{ text };
    while (!rest.empty())
    {
        const auto n = std::min(chunk, rest.size());
        try
        {
            sm.ProcessString(rest.substr(0, n));
        }
        catch (const wil::ResultException&)
        {
            // Upstream signals rejected input with THROW_HR (e.g. FontBuffer's row limit);
            // hosts catch it around ProcessString. Anything else still crashes.
        }
        rest.remove_prefix(n);
    }
    return 0;
}

// Fuzz target: Windows Terminal's VT *output* parser — the byte stream any program
// (or remote host) writes to the terminal. UTF-8 input is converted with til::u8u16,
// then fed through StateMachine + OutputStateMachineEngine (CSI/OSC/DCS/ESC parsing,
// OSC 4/10/11 colour specs, OSC 52 base64, OSC 8 hyperlinks, til::parse_*), with
// DECDLD soft fonts and DECDMAC macros handed to upstream's FontBuffer/MacroBuffer.
// The first byte picks the chunk size the string is split into, so sequences that
// straddle ProcessString calls (as with a streaming pty) are exercised too.
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
    auto dispatch = std::make_unique<mayhem::OutputDispatch>(log);
    auto* d = dispatch.get();
    auto engine = std::make_unique<Microsoft::Console::VirtualTerminal::OutputStateMachineEngine>(std::move(dispatch));
    Microsoft::Console::VirtualTerminal::StateMachine sm{ std::move(engine) };
    d->SetStateMachine(&sm);
    sm.SetParserMode(Microsoft::Console::VirtualTerminal::StateMachine::Mode::AcceptC1, (data[0] & 0x40) != 0);

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

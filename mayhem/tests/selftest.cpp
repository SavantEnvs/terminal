// Known-answer tests for the Windows Terminal code the fuzz targets build on Linux:
// the VT output and input parsers (with the recording dispatches from
// mayhem/harness/vt_dispatch.hpp), OSC colour/clipboard/hyperlink parsing,
// DECDLD/DECDMAC, til::parse_*, til::u8u16/u16u8 and CodepointWidthDetector.
// The expectations follow upstream's own TAEF tests (src/terminal/parser/ut_parser,
// src/inc/test/*) and the VT specs. Prints one PASS/FAIL line per check and a final
// "SUMMARY passed=<n> failed=<n>" line that mayhem/test.sh parses.
#include "../harness/vt_dispatch.hpp"

#include "../../src/types/inc/CodepointWidthDetector.hpp"
#include <til/string.h>

static int g_pass = 0, g_fail = 0;

static void check(const char* name, const std::string& actual, const std::string& expected)
{
    if (actual == expected)
    {
        ++g_pass;
        std::printf("PASS %s\n", name);
    }
    else
    {
        ++g_fail;
        std::printf("FAIL %s\n  expected: %s\n  actual:   %s\n", name, expected.c_str(), actual.c_str());
    }
}

// The parser may hand a run of printable text to the dispatch as one PrintString or as
// several Print/PrintString calls (on Linux, where wchar_t is 32-bit, upstream's SSE2
// scan in Utils::FindActionableControlCharacter splits runs). Merge consecutive ones
// into a single Text(...) line so the expectations only depend on what was printed.
static std::string mergePrints(const std::string& log)
{
    std::string result, text;
    size_t pos = 0;
    while (pos < log.size())
    {
        const auto nl = log.find('\n', pos);
        const auto line = log.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.rfind("Print(", 0) == 0 || line.rfind("PrintString(", 0) == 0)
        {
            const auto open = line.find('(');
            text += line.substr(open + 1, line.size() - open - 2);
            continue;
        }
        if (!text.empty())
        {
            result += "Text(" + text + ")\n";
            text.clear();
        }
        result += line + "\n";
    }
    if (!text.empty())
    {
        result += "Text(" + text + ")\n";
    }
    return result;
}

static std::string out(std::string_view utf8, bool c1 = false)
{
    mayhem::EventLog log;
    auto dispatch = std::make_unique<mayhem::OutputDispatch>(log);
    auto* d = dispatch.get();
    Microsoft::Console::VirtualTerminal::StateMachine sm{ std::make_unique<Microsoft::Console::VirtualTerminal::OutputStateMachineEngine>(std::move(dispatch)) };
    d->SetStateMachine(&sm);
    sm.SetParserMode(Microsoft::Console::VirtualTerminal::StateMachine::Mode::AcceptC1, c1);
    sm.ProcessString(mayhem::Utf8ToUtf16(reinterpret_cast<const uint8_t*>(utf8.data()), utf8.size()));
    return mergePrints(log.text);
}

static std::string in(std::string_view utf8)
{
    mayhem::EventLog log;
    // Explicit isEngineForInput (see mayhem/harness/fuzz_vt_input.cpp).
    Microsoft::Console::VirtualTerminal::StateMachine sm{ std::unique_ptr<Microsoft::Console::VirtualTerminal::IStateMachineEngine>{ std::make_unique<Microsoft::Console::VirtualTerminal::InputStateMachineEngine>(std::make_unique<mayhem::InputDispatch>(log)) }, true };
    sm.ProcessString(mayhem::Utf8ToUtf16(reinterpret_cast<const uint8_t*>(utf8.data()), utf8.size()));
    return log.text;
}

template<typename T>
static std::string opt(const std::optional<T>& v)
{
    return v ? std::to_string(*v) : std::string("none");
}

static std::string widths(std::wstring_view text, TextMeasurementMode mode)
{
    auto& cwd = CodepointWidthDetector::Singleton();
    cwd.Reset(mode);
    GraphemeState s{};
    std::string r;
    // GraphemeNext returns false once it reached the end, but still reports that last cluster.
    bool more;
    do
    {
        more = cwd.GraphemeNext(s, text);
        if (s.len)
        {
            r += std::to_string(s.len) + ":" + std::to_string(s.width) + " ";
        }
    } while (more);
    cwd.Reset(TextMeasurementMode::Graphemes);
    return r;
}

int main()
{
    // ---- VT output parser ---------------------------------------------------------
    check("print", out("hi"), "Text(hi)\n");
    check("cup", out("\x1b[5;10H"), "CUP(5,10)\n");
    check("cup-default", out("\x1b[H"), "CUP(1,1)\n");
    check("cuu-cud", out("\x1b[3A\x1b[B"), "CUU(3)\nCUD(1)\n");
    check("sgr", out("\x1b[1;38;5;196m"), "SGR(1;38;5;196)\n");
    check("sgr-subparams", out("\x1b[38:2::10:20:30m"), "SGR(38:2:-1:10:20:30)\n");
    // DECTCEM is DEC private mode 25, encoded as 0x1000000 | 25.
    check("decset", out("\x1b[?25l"), "RM(16777241)\n");
    check("decstbm", out("\x1b[2;20r"), "DECSTBM(2,20)\n");
    check("ed-el", out("\x1b[2J\x1b[K"), "ED(2)\nEL(0)\n");
    check("da1", out("\x1b[c"), "DA1\n");
    check("title", out("\x1b]2;hello\x07"), "Title(hello)\n");
    check("title-st", out("\x1b]0;abc\x1b\\"), "Title(abc)\n");
    check("osc4-rgb", out("\x1b]4;1;rgb:ff/80/00\x07"), "SetColor(1,0080ff)\n");
    check("osc4-name", out("\x1b]4;2;red\x07"), "SetColor(2,0000ff)\n");
    check("osc4-hex", out("\x1b]4;3;#102030\x07"), "SetColor(3,302010)\n");
    check("osc4-bad", out("\x1b]4;3;nocolor\x07"), "");
    check("osc52", out("\x1b]52;c;aGVsbG8=\x07"), "Clipboard(hello)\n");
    check("osc8", out("\x1b]8;id=1;http://x.test/\x07link\x1b]8;;\x07"), "Hyperlink(http://x.test/|1)\nText(link)\nEndHyperlink\n");
    check("c1-off", out("\xc2\x9b" "5A"), "Text(5A)\n");
    check("c1-on", out("\xc2\x9b" "5A", true), "CUU(5)\n");
    check("utf8", out("\xe2\x82\xac"), "Text(\\u20ac)\n");
    check("decdld", out("\x1bP1;1;1;0;0;2;0;0{ @??~~~~~~/???????\x1b\\"), "DECDLD(10x10,960,1bb6b680)\n");
    check("decdmac", out("\x1bP1;0;0!zAB\x1b\\\x1b[1*z"), "DECINVM(1)\nText(AB)\n");

    // ---- VT input parser ------------------------------------------------------------
    check("in-char", in("a"), "Key(1,vk=65,ch=97,mod=0)\nKey(0,vk=65,ch=97,mod=0)\n");
    check("in-up", in("\x1b[A"), "Key(1,vk=38,ch=0,mod=100)\nKey(0,vk=38,ch=0,mod=100)\n");
    // Modifiers are synthesized as separate Ctrl down/up events around the key.
    check("in-ctrl-up", in("\x1b[1;5A"), "Key(1,vk=17,ch=0,mod=8)\nKey(1,vk=38,ch=0,mod=108)\nKey(0,vk=38,ch=0,mod=108)\nKey(0,vk=17,ch=0,mod=0)\n");
    check("in-f1", in("\x1bOP"), "Key(1,vk=112,ch=0,mod=0)\nKey(0,vk=112,ch=0,mod=0)\n");
    check("in-sgr-mouse", in("\x1b[<0;10;5M"), "Mouse(9,4,btn=1,mod=0,flags=0)\n");
    check("in-focus", in("\x1b[I\x1b[O"), "Focus(1)\nFocus(0)\n");
    check("in-w32im", in("\x1b[65;30;97;1;0;1_"), "Key(1,vk=65,ch=97,mod=0)\n");
    check("in-da1", in("\x1b[?61;4;6;7c"), "DA1(d0)\n");

    // ---- til::parse_* ------------------------------------------------------------------
    check("parse-dec", opt(til::parse_unsigned<uint32_t>(std::string_view{ "12345" }, 10)), "12345");
    check("parse-hex-auto", opt(til::parse_unsigned<uint32_t>(std::string_view{ "0x1F" }, 0)), "31");
    check("parse-oct-auto", opt(til::parse_unsigned<uint32_t>(std::string_view{ "017" }, 0)), "15");
    check("parse-overflow", opt(til::parse_unsigned<uint8_t>(std::string_view{ "256" }, 10)), "none");
    check("parse-neg", opt(til::parse_signed<int32_t>(std::wstring_view{ L"-42" }, 10)), "-42");
    check("parse-min", opt(til::parse_signed<int8_t>(std::string_view{ "-128" }, 10)), "-128");
    check("parse-bad", opt(til::parse_unsigned<uint32_t>(std::string_view{ "12a" }, 10)), "none");
    check("parse-empty", opt(til::parse_unsigned<uint32_t>(std::string_view{ "" }, 10)), "none");

    // ---- til::u8u16 / u16u8 ------------------------------------------------------------
    {
        til::u8state st{};
        std::wstring a, b;
        (void)til::u8u16(std::string_view{ "\xf0\x9f\x98" }, a, st);
        (void)til::u8u16(std::string_view{ "\x80!" }, b, st);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%zu|%zu:%04x,%04x,%04x", a.size(), b.size(), b.size() > 0 ? (unsigned)b[0] : 0u, b.size() > 1 ? (unsigned)b[1] : 0u, b.size() > 2 ? (unsigned)b[2] : 0u);
        check("u8u16-partial", buf, "0|3:d83d,de00,0021");
    }
    {
        std::wstring w;
        (void)til::u8u16(std::string_view{ "a\xff" "b" }, w);
        check("u8u16-invalid", mayhem::EventLog::narrow(w), "a\\ufffdb");
        std::string n;
        (void)til::u16u8(std::wstring{ L'x', static_cast<wchar_t>(0xd800), L'y' }, n);
        check("u16u8-lone-surrogate", n, "x\xef\xbf\xbdy");
    }

    // ---- CodepointWidthDetector -----------------------------------------------------
    check("width-ascii", widths(L"ab", TextMeasurementMode::Graphemes), "1:1 1:1 ");
    check("width-cjk", widths(std::wstring{ static_cast<wchar_t>(0x4e00) }, TextMeasurementMode::Graphemes), "1:2 ");
    check("width-emoji-pair", widths(std::wstring{ static_cast<wchar_t>(0xd83d), static_cast<wchar_t>(0xde00) }, TextMeasurementMode::Graphemes), "2:2 ");
    check("width-combining", widths(std::wstring{ L'e', static_cast<wchar_t>(0x0301) }, TextMeasurementMode::Graphemes), "2:1 ");
    check("width-wcswidth-zwj", widths(std::wstring{ static_cast<wchar_t>(0xd83d), static_cast<wchar_t>(0xdc68), static_cast<wchar_t>(0x200d), static_cast<wchar_t>(0xd83d), static_cast<wchar_t>(0xdc69) }, TextMeasurementMode::Graphemes), "5:2 ");

    std::printf("SUMMARY passed=%d failed=%d\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}

// Dispatch implementations that sit behind Windows Terminal's VT parser in the
// fuzz targets and the self-test. They record what the parser decoded and, for the
// DCS strings that carry their own grammar (DECDLD soft fonts, DECDMAC macros),
// feed the data into upstream's FontBuffer / MacroBuffer exactly like
// AdaptDispatch::DownloadDRCS / DefineMacro / InvokeMacro do.
#pragma once

#include "../../src/terminal/adapter/precomp.h"

#include "../../src/terminal/parser/stateMachine.hpp"
#include "../../src/terminal/parser/OutputStateMachineEngine.hpp"
#include "../../src/terminal/parser/InputStateMachineEngine.hpp"
#include "../../src/terminal/parser/ascii.hpp"
#include "../../src/terminal/adapter/FontBuffer.hpp"
#include "../../src/terminal/adapter/MacroBuffer.hpp"

#include <cstdio>
#include <string>

namespace mayhem
{
    using namespace Microsoft::Console::VirtualTerminal;

    // Appends "name(args)\n" lines; capped so a long fuzz input can't grow it unbounded.
    struct EventLog
    {
        std::string text;
        void add(const char* fmt, ...) __attribute__((format(printf, 2, 3)))
        {
            if (text.size() > (1u << 16))
            {
                return;
            }
            char buf[256];
            va_list ap;
            va_start(ap, fmt);
            const int n = std::vsnprintf(buf, sizeof(buf), fmt, ap);
            va_end(ap);
            if (n > 0)
            {
                text.append(buf, std::min<size_t>(static_cast<size_t>(n), sizeof(buf) - 1));
                text.push_back('\n');
            }
        }
        static std::string narrow(std::wstring_view s)
        {
            std::string out;
            for (const auto ch : s)
            {
                const auto c = static_cast<uint32_t>(ch);
                if (c >= 0x20 && c < 0x7f)
                {
                    out.push_back(static_cast<char>(c));
                }
                else
                {
                    char esc[16];
                    std::snprintf(esc, sizeof(esc), "\\u%04x", c);
                    out += esc;
                }
            }
            return out;
        }
    };

    class OutputDispatch final : public TermDispatch
    {
    public:
        explicit OutputDispatch(EventLog& log) noexcept : _log(log) {}
        void SetStateMachine(StateMachine* sm) noexcept { _stateMachine = sm; }

        void Print(const wchar_t wch) override { _log.add("Print(%s)", EventLog::narrow({ &wch, 1 }).c_str()); }
        void PrintString(const std::wstring_view s) override { _log.add("PrintString(%s)", EventLog::narrow(s).c_str()); }
        void UnknownSequence() noexcept override { _log.add("Unknown"); }
        void CursorUp(const VTInt d) override { _log.add("CUU(%d)", d); }
        void CursorDown(const VTInt d) override { _log.add("CUD(%d)", d); }
        void CursorForward(const VTInt d) override { _log.add("CUF(%d)", d); }
        void CursorBackward(const VTInt d) override { _log.add("CUB(%d)", d); }
        void CursorPosition(const VTInt line, const VTInt column) override { _log.add("CUP(%d,%d)", line, column); }
        void EraseInDisplay(const DispatchTypes::EraseType t) override { _log.add("ED(%d)", static_cast<int>(t)); }
        void EraseInLine(const DispatchTypes::EraseType t) override { _log.add("EL(%d)", static_cast<int>(t)); }
        void SetGraphicsRendition(const VTParameters options) override
        {
            std::string s;
            for (size_t i = 0; i < options.size(); ++i)
            {
                s += (i ? ";" : "") + std::to_string(options.at(i).value_or(-1));
                const auto subs = options.subParamsFor(i);
                for (size_t j = 0; j < subs.size(); ++j)
                {
                    s += ":" + std::to_string(subs.at(j).value_or(-1));
                }
            }
            _log.add("SGR(%s)", s.c_str());
        }
        void SetMode(const DispatchTypes::ModeParams m) override { _log.add("SM(%d)", static_cast<int>(m)); }
        void ResetMode(const DispatchTypes::ModeParams m) override { _log.add("RM(%d)", static_cast<int>(m)); }
        void SetWindowTitle(std::wstring_view title) override { _log.add("Title(%s)", EventLog::narrow(title).c_str()); }
        void SetColorTableEntry(const size_t index, const DWORD color) override { _log.add("SetColor(%zu,%06x)", index, color); }
        void SetClipboard(wil::zwstring_view content) override { _log.add("Clipboard(%s)", EventLog::narrow(content).c_str()); }
        void AddHyperlink(const std::wstring_view uri, const std::wstring_view params) override
        {
            _log.add("Hyperlink(%s|%s)", EventLog::narrow(uri).c_str(), EventLog::narrow(params).c_str());
        }
        void EndHyperlink() override { _log.add("EndHyperlink"); }
        void DeviceAttributes() override { _log.add("DA1"); }
        void SetTopBottomScrollingMargins(const VTInt top, const VTInt bottom) override { _log.add("DECSTBM(%d,%d)", top, bottom); }

        StringHandler DownloadDRCS(const VTInt fontNumber,
                                   const VTParameter startChar,
                                   const DispatchTypes::DrcsEraseControl eraseControl,
                                   const DispatchTypes::DrcsCellMatrix cellMatrix,
                                   const DispatchTypes::DrcsFontSet fontSet,
                                   const DispatchTypes::DrcsFontUsage fontUsage,
                                   const VTParameter cellHeight,
                                   const DispatchTypes::CharsetSize charsetSize) override
        {
            _font = std::make_unique<FontBuffer>();
            auto ok = fontNumber <= 1;
            ok = ok && _font->SetEraseControl(eraseControl);
            ok = ok && _font->SetAttributes(cellMatrix, cellHeight, fontSet, fontUsage);
            ok = ok && _font->SetStartChar(startChar, charsetSize);
            if (!ok)
            {
                return nullptr;
            }
            return [this](const wchar_t ch) {
                if (ch != AsciiChars::ESC)
                {
                    try
                    {
                        _font->AddSixelData(ch);
                    }
                    catch (...)
                    {
                        return false;
                    }
                }
                else if (_font->FinalizeSixelData())
                {
                    const auto cell = _font->GetCellSize();
                    const auto bits = _font->GetBitPattern();
                    uint32_t sum = 0;
                    for (const auto row : bits)
                    {
                        sum = sum * 31 + row;
                    }
                    _log.add("DECDLD(%dx%d,%zu,%08x)", cell.width, cell.height, bits.size(), sum);
                }
                return true;
            };
        }

        StringHandler DefineMacro(const VTInt macroId,
                                  const DispatchTypes::MacroDeleteControl deleteControl,
                                  const DispatchTypes::MacroEncoding encoding) override
        {
            if (!_macros)
            {
                _macros = std::make_shared<MacroBuffer>();
            }
            if (_macros->InitParser(macroId, deleteControl, encoding))
            {
                return [this](const wchar_t ch) { return _macros->ParseDefinition(ch); };
            }
            return nullptr;
        }

        void InvokeMacro(const VTInt macroId) override
        {
            _log.add("DECINVM(%d)", macroId);
            if (_macros && _stateMachine)
            {
                const auto macros = _macros;
                auto& sm = *_stateMachine;
                sm.OnCsiComplete([=, &sm]() { macros->InvokeMacro(macroId, sm); });
            }
        }

    private:
        EventLog& _log;
        StateMachine* _stateMachine = nullptr;
        std::unique_ptr<FontBuffer> _font;
        std::shared_ptr<MacroBuffer> _macros;
    };

    class InputDispatch final : public IInteractDispatch
    {
    public:
        explicit InputDispatch(EventLog& log) noexcept : _log(log) {}

        bool IsVtInputEnabled() const override { return false; }
        void SetDeviceAttributes(til::enumset<DeviceAttribute, uint64_t> attributes) override
        {
            _log.add("DA1(%llx)", static_cast<unsigned long long>(attributes.bits()));
        }
        void WriteInput(const std::span<const INPUT_RECORD>& events) override
        {
            for (const auto& ev : events)
            {
                record(ev);
            }
        }
        void WriteCtrlKey(const INPUT_RECORD& ev) override { record(ev); }
        void WriteString(std::wstring_view s) override { _log.add("String(%s)", EventLog::narrow(s).c_str()); }
        void WriteStringRaw(std::wstring_view s) override { _log.add("Raw(%s)", EventLog::narrow(s).c_str()); }
        void WindowManipulation(DispatchTypes::WindowManipulationType f, VTParameter a, VTParameter b) override
        {
            _log.add("Win(%d,%d,%d)", static_cast<int>(f), a.value_or(-1), b.value_or(-1));
        }
        void MoveCursor(VTInt row, VTInt col) override { _log.add("Move(%d,%d)", row, col); }
        void FocusChanged(bool focused) override { _log.add("Focus(%d)", focused ? 1 : 0); }

    private:
        void record(const INPUT_RECORD& ev)
        {
            if (ev.EventType == KEY_EVENT)
            {
                const auto& k = ev.Event.KeyEvent;
                _log.add("Key(%d,vk=%u,ch=%u,mod=%x)", k.bKeyDown, k.wVirtualKeyCode, static_cast<unsigned>(k.uChar.UnicodeChar), k.dwControlKeyState);
            }
            else if (ev.EventType == MOUSE_EVENT)
            {
                const auto& m = ev.Event.MouseEvent;
                _log.add("Mouse(%d,%d,btn=%x,mod=%x,flags=%x)", m.dwMousePosition.X, m.dwMousePosition.Y, m.dwButtonState, m.dwControlKeyState, m.dwEventFlags);
            }
            else
            {
                _log.add("Event(%u)", ev.EventType);
            }
        }
        EventLog& _log;
    };

    // VT output arrives as UTF-8 (from a pty / ConPTY pipe) and is converted with
    // til::u8u16 before it reaches the parser, which is what this helper does.
    inline std::wstring Utf8ToUtf16(const uint8_t* data, size_t size)
    {
        std::wstring out;
        til::u8state state{};
        (void)til::u8u16({ reinterpret_cast<const char*>(data), size }, out, state);
        return out;
    }
}

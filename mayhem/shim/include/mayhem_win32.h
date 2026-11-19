// mayhem shim: the small subset of <windows.h>, SAL, WIL and TraceLogging that the
// portable parts of Windows Terminal (til, the VT parser, color parsing) use, so that
// they build unmodified with clang on Linux. Nothing here re-implements terminal logic:
// it only provides Win32 types/constants, WIL's error-handling macros (mapped to C++
// exceptions, like WIL's own THROW_* do), and no-op TraceLogging.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cwchar>
#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>
#include <exception>
#include <type_traits>
#include <utility>

// ---- compiler / SAL -------------------------------------------------------
#define __declspec(...)
#define __forceinline inline __attribute__((always_inline))
#define __assume(x) ((void)0)
#define sealed final
#define WINAPI
#define APIENTRY
#define CALLBACK
#define NTAPI
#define STDMETHODCALLTYPE
#define _In_
#define _In_z_
#define _In_opt_
#define _In_reads_(x)
#define _In_reads_bytes_(x)
#define _In_range_(a, b)
#define _Out_
#define _Out_opt_
#define _Out_writes_(x)
#define _Inout_
#define _Inout_opt_
#define _Ret_maybenull_
#define _Success_(x)
#define _Null_terminated_
#define _Field_size_(x)
#define _Check_return_
#define _Must_inspect_result_
#define _Pre_satisfies_(x)
#define _Post_satisfies_(x)
#define _Analysis_assume_(x)
#define _Printf_format_string_
#define _Acquires_lock_(x)
#define _Releases_lock_(x)
#define _Requires_lock_held_(x)
#define _Notnull_
#define _When_(a, b)
#define UNREFERENCED_PARAMETER(x) ((void)(x))
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _WINDEF_ 1
#define _DDK_INCLUDED 1

// ---- basic Win32 types ----------------------------------------------------
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t DWORD64;
typedef int32_t BOOL;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef uint32_t UINT;
typedef int32_t INT;
typedef int16_t SHORT;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef uint16_t USHORT;
typedef uint8_t UCHAR;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef size_t SIZE_T;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef void* HANDLE;
typedef void* PVOID;
typedef void* LPVOID;
typedef const wchar_t* LPCWSTR;
typedef const wchar_t* PCWSTR;
typedef wchar_t* LPWSTR;
typedef wchar_t* PWSTR;
typedef const char* LPCSTR;
typedef const char* PCSTR;
typedef int32_t HRESULT;
typedef int32_t NTSTATUS;
typedef DWORD COLORREF;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

#define INFINITE 0xFFFFFFFF
#define COMMON_LVB_LEADING_BYTE 0x0100
#define COMMON_LVB_TRAILING_BYTE 0x0200
#define COMMON_LVB_GRID_HORIZONTAL 0x0400
#define COMMON_LVB_GRID_LVERTICAL 0x0800
#define COMMON_LVB_GRID_RVERTICAL 0x1000
#define COMMON_LVB_REVERSE_VIDEO 0x4000
#define COMMON_LVB_UNDERSCORE 0x8000
#define FOREGROUND_BLUE 0x0001
#define FOREGROUND_GREEN 0x0002
#define FOREGROUND_RED 0x0004
#define FOREGROUND_INTENSITY 0x0008
#define BACKGROUND_BLUE 0x0010
#define BACKGROUND_GREEN 0x0020
#define BACKGROUND_RED 0x0040
#define BACKGROUND_INTENSITY 0x0080
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
typedef uintptr_t DWORD_PTR;

#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define E_UNEXPECTED ((HRESULT)0x8000FFFFL)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_ABORT ((HRESULT)0x80004004L)
#define E_POINTER ((HRESULT)0x80004003L)
#define E_BOUNDS ((HRESULT)0x8000000BL)
#define ERROR_SUCCESS 0L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_INSUFFICIENT_BUFFER 122L
#define ERROR_NO_UNICODE_TRANSLATION 1113L
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT)(((x) & 0x0000FFFF) | (7 << 16) | 0x80000000)))
#define ERROR_UNHANDLED_EXCEPTION 574L
#define ERROR_INVALID_DATA 13L
#define ERROR_ARITHMETIC_OVERFLOW 534L
#ifndef _ITERATOR_DEBUG_LEVEL
#define _ITERATOR_DEBUG_LEVEL 0
#endif
#define INTSAFE_E_ARITHMETIC_OVERFLOW ((HRESULT)0x80070216L)

// ---- misc Win32 structs / APIs (implemented in mayhem/shim/win32_shim.cpp) ---
typedef struct _GUID { uint32_t Data1; uint16_t Data2; uint16_t Data3; uint8_t Data4[8]; } GUID;
inline bool operator==(const GUID& a, const GUID& b) noexcept { return std::memcmp(&a, &b, sizeof(GUID)) == 0; }
typedef struct _OVERLAPPED { ULONG_PTR Internal; ULONG_PTR InternalHigh; DWORD Offset; DWORD OffsetHigh; HANDLE hEvent; } OVERLAPPED, *LPOVERLAPPED;
#define CP_UTF8 65001
#define MB_ERR_INVALID_CHARS 0x00000008
#define WC_ERR_INVALID_CHARS 0x00000080
#define LOCALE_NAME_USER_DEFAULT nullptr
#define LINGUISTIC_IGNORECASE 0x00000010
#define NORM_IGNORECASE 0x00000001
#define FIND_FROMSTART 0x00400000
#define CSTR_LESS_THAN 1
#define CSTR_EQUAL 2
#define CSTR_GREATER_THAN 3
int MultiByteToWideChar(UINT codePage, DWORD flags, const char* src, int srcLen, wchar_t* dst, int dstLen) noexcept;
int WideCharToMultiByte(UINT codePage, DWORD flags, const wchar_t* src, int srcLen, char* dst, int dstLen, const char* defChar, BOOL* usedDef) noexcept;
int CompareStringOrdinal(const wchar_t* a, int aLen, const wchar_t* b, int bLen, BOOL ignoreCase) noexcept;
int CompareStringEx(const wchar_t* locale, DWORD flags, const wchar_t* a, int aLen, const wchar_t* b, int bLen, void*, void*, LPARAM) noexcept;
int FindNLSStringEx(const wchar_t* locale, DWORD flags, const wchar_t* src, int srcLen, const wchar_t* find, int findLen, int* foundLen, void*, void*, LPARAM) noexcept;
DWORD GetLastError() noexcept;
UINT GetDoubleClickTime() noexcept;
BOOL WaitOnAddress(volatile void* address, void* compareAddress, SIZE_T size, DWORD ms) noexcept;
void WakeByAddressSingle(void* address) noexcept;
void WakeByAddressAll(void* address) noexcept;
void SetLastError(DWORD) noexcept;

typedef struct tagPOINT { LONG x; LONG y; } POINT;
typedef struct tagSIZE { LONG cx; LONG cy; } SIZE;
typedef struct tagRECT { LONG left; LONG top; LONG right; LONG bottom; } RECT;

typedef struct tagRGBQUAD { BYTE rgbBlue; BYTE rgbGreen; BYTE rgbRed; BYTE rgbReserved; } RGBQUAD;
typedef void* HKL;
typedef void* HWND;
#define LOCALE_NAME_INVARIANT L""
#define LCMAP_LOWERCASE 0x00000100
int LCMapStringEx(const wchar_t* locale, DWORD flags, const wchar_t* src, int srcLen, wchar_t* dst, int dstLen, void*, void*, LPARAM) noexcept;
HWND GetForegroundWindow() noexcept;
DWORD GetWindowThreadProcessId(HWND hwnd, DWORD* pid) noexcept;
HKL GetKeyboardLayout(DWORD thread) noexcept;
UINT MapVirtualKeyExW(UINT code, UINT type, HKL hkl) noexcept;
int ToUnicodeEx(UINT vk, UINT scan, const BYTE* keyState, wchar_t* buf, int cch, UINT flags, HKL hkl) noexcept;
SHORT VkKeyScanExW(WCHAR ch, HKL hkl) noexcept;
SHORT GetKeyState(int vk) noexcept;
HRESULT SizeTToInt(size_t v, int* out) noexcept;
HRESULT IntToSizeT(int v, size_t* out) noexcept;
HRESULT SizeTToShort(size_t v, SHORT* out) noexcept;
HRESULT IntToShort(int v, SHORT* out) noexcept;
HRESULT SizeTMult(size_t a, size_t b, size_t* out) noexcept;
HRESULT SizeTAdd(size_t a, size_t b, size_t* out) noexcept;
HRESULT IntAdd(int a, int b, int* out) noexcept;
HRESULT IntSub(int a, int b, int* out) noexcept;
HRESULT IntMult(int a, int b, int* out) noexcept;
HRESULT ShortAdd(SHORT a, SHORT b, SHORT* out) noexcept;
HRESULT ShortSub(SHORT a, SHORT b, SHORT* out) noexcept;
HRESULT LongToShort(LONG v, SHORT* out) noexcept;
HRESULT ULongToInt(ULONG v, int* out) noexcept;
HRESULT UIntAdd(UINT a, UINT b, UINT* out) noexcept;

#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_MOUSEWHEEL 0x020A
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP 0x020C
#define WM_XBUTTONDBLCLK 0x020D
#define WM_MOUSEHWHEEL 0x020E
#define WHEEL_DELTA 120

// ---- console types --------------------------------------------------------
typedef struct _COORD { SHORT X; SHORT Y; } COORD, *PCOORD;
typedef struct _SMALL_RECT { SHORT Left; SHORT Top; SHORT Right; SHORT Bottom; } SMALL_RECT;
typedef struct _KEY_EVENT_RECORD {
    BOOL bKeyDown;
    WORD wRepeatCount;
    WORD wVirtualKeyCode;
    WORD wVirtualScanCode;
    union { WCHAR UnicodeChar; CHAR AsciiChar; } uChar;
    DWORD dwControlKeyState;
} KEY_EVENT_RECORD;
typedef struct _MOUSE_EVENT_RECORD { COORD dwMousePosition; DWORD dwButtonState; DWORD dwControlKeyState; DWORD dwEventFlags; } MOUSE_EVENT_RECORD;
typedef struct _WINDOW_BUFFER_SIZE_RECORD { COORD dwSize; } WINDOW_BUFFER_SIZE_RECORD;
typedef struct _MENU_EVENT_RECORD { UINT dwCommandId; } MENU_EVENT_RECORD;
typedef struct _FOCUS_EVENT_RECORD { BOOL bSetFocus; } FOCUS_EVENT_RECORD;
typedef struct _INPUT_RECORD {
    WORD EventType;
    union {
        KEY_EVENT_RECORD KeyEvent;
        MOUSE_EVENT_RECORD MouseEvent;
        WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent;
    } Event;
} INPUT_RECORD;
#define KEY_EVENT 0x0001
#define MOUSE_EVENT 0x0002
#define WINDOW_BUFFER_SIZE_EVENT 0x0004
#define MENU_EVENT 0x0008
#define FOCUS_EVENT 0x0010
#define RIGHT_ALT_PRESSED 0x0001
#define LEFT_ALT_PRESSED 0x0002
#define RIGHT_CTRL_PRESSED 0x0004
#define LEFT_CTRL_PRESSED 0x0008
#define SHIFT_PRESSED 0x0010
#define NUMLOCK_ON 0x0020
#define SCROLLLOCK_ON 0x0040
#define CAPSLOCK_ON 0x0080
#define ENHANCED_KEY 0x0100
#define FROM_LEFT_1ST_BUTTON_PRESSED 0x0001
#define RIGHTMOST_BUTTON_PRESSED 0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED 0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED 0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED 0x0010
#define MOUSE_MOVED 0x0001
#define DOUBLE_CLICK 0x0002
#define MOUSE_WHEELED 0x0004
#define MOUSE_HWHEELED 0x0008
#define MAPVK_VK_TO_VSC 0
#define MAPVK_VSC_TO_VK 1
#define MAPVK_VK_TO_CHAR 2
#define MAPVK_VSC_TO_VK_EX 3

typedef struct _CHAR_INFO { union { WCHAR UnicodeChar; CHAR AsciiChar; } Char; WORD Attributes; } CHAR_INFO;
#define COMMON_LVB_SBCSDBCS 0x0300

// Virtual-key codes (winuser.h)
#define VK_PACKET 0xE7
#define VK_VOLUME_MUTE 0xAD
#define VK_VOLUME_DOWN 0xAE
#define VK_VOLUME_UP 0xAF
#define VK_MEDIA_NEXT_TRACK 0xB0
#define VK_MEDIA_PREV_TRACK 0xB1
#define VK_MEDIA_STOP 0xB2
#define VK_MEDIA_PLAY_PAUSE 0xB3
#define VK_BROWSER_BACK 0xA6
#define VK_BROWSER_FORWARD 0xA7
#define VK_BROWSER_REFRESH 0xA8
#define VK_BROWSER_STOP 0xA9
#define VK_BROWSER_SEARCH 0xAA
#define VK_BROWSER_FAVORITES 0xAB
#define VK_BROWSER_HOME 0xAC
#define VK_LAUNCH_MAIL 0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1 0xB6
#define VK_LAUNCH_APP2 0xB7
#define VK_OEM_8 0xDF
#define VK_PROCESSKEY 0xE5
#define VK_KANA 0x15
#define VK_HANGUL 0x15
#define VK_KANJI 0x19
#define VK_CONVERT 0x1C
#define VK_NONCONVERT 0x1D
#define VK_SLEEP 0x5F
#define VK_XBUTTON1 0x05
#define VK_XBUTTON2 0x06
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_CANCEL 0x03
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SELECT 0x29
#define VK_PRINT 0x2A
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_F13 0x7C
#define VK_F14 0x7D
#define VK_F15 0x7E
#define VK_F16 0x7F
#define VK_F17 0x80
#define VK_F18 0x81
#define VK_F19 0x82
#define VK_F20 0x83
#define VK_F21 0x84
#define VK_F22 0x85
#define VK_F23 0x86
#define VK_F24 0x87
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_OEM_1 0xBA
#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE
#define VK_OEM_102 0xE2

// ---- enum flag operators (winnt.h DEFINE_ENUM_FLAG_OPERATORS) --------------
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE)                                                                                                         \
    extern "C++" {                                                                                                                                    \
    inline constexpr ENUMTYPE operator|(ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) | ((std::underlying_type_t<ENUMTYPE>)b)); } \
    inline ENUMTYPE& operator|=(ENUMTYPE& a, ENUMTYPE b) noexcept { return a = a | b; }                                                              \
    inline constexpr ENUMTYPE operator&(ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) & ((std::underlying_type_t<ENUMTYPE>)b)); } \
    inline ENUMTYPE& operator&=(ENUMTYPE& a, ENUMTYPE b) noexcept { return a = a & b; }                                                              \
    inline constexpr ENUMTYPE operator~(ENUMTYPE a) noexcept { return ENUMTYPE(~((std::underlying_type_t<ENUMTYPE>)a)); }                           \
    inline constexpr ENUMTYPE operator^(ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type_t<ENUMTYPE>)a) ^ ((std::underlying_type_t<ENUMTYPE>)b)); } \
    inline ENUMTYPE& operator^=(ENUMTYPE& a, ENUMTYPE b) noexcept { return a = a ^ b; }                                                              \
    }

// ---- WIL flag helpers -----------------------------------------------------
#define WI_IsFlagSet(val, flag) ((((val) & (flag)) == (flag)))
#define WI_IsFlagClear(val, flag) (((val) & (flag)) == static_cast<decltype((val) & (flag))>(0))
#define WI_IsAnyFlagSet(val, flags) (((val) & (flags)) != static_cast<decltype((val) & (flags))>(0))
#define WI_AreAllFlagsSet(val, flags) ((((val) & (flags)) == (flags)))
#define WI_AreAllFlagsClear(val, flags) (((val) & (flags)) == static_cast<decltype((val) & (flags))>(0))
#define WI_SetFlag(var, flag) ((var) |= (flag))
#define WI_SetAllFlags(var, flags) ((var) |= (flags))
#define WI_ClearFlag(var, flag) ((var) &= ~(flag))
#define WI_ClearAllFlags(var, flags) ((var) &= ~(flags))
#define WI_SetFlagIf(var, flag, cond) do { if (cond) { WI_SetFlag(var, flag); } } while (0)
#define WI_ClearFlagIf(var, flag, cond) do { if (cond) { WI_ClearFlag(var, flag); } } while (0)
#define WI_UpdateFlag(var, flag, cond) do { if (cond) { WI_SetFlag(var, flag); } else { WI_ClearFlag(var, flag); } } while (0)
#define WI_ToggleFlag(var, flag) ((var) ^= (flag))
#define WI_EnumValue(e) static_cast<std::underlying_type_t<decltype(e)>>(e)
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#define WI_NOEXCEPT noexcept
#define WI_ASSERT(x) ((void)0)

// ---- WIL error handling: throw/return like WIL, logging is a no-op --------
namespace wil
{
    struct ResultException : std::exception
    {
        HRESULT hr;
        explicit ResultException(HRESULT h) noexcept : hr(h) {}
        const char* what() const noexcept override { return "wil::ResultException"; }
        HRESULT GetErrorCode() const noexcept { return hr; }
    };
    [[noreturn]] inline void details_throw(HRESULT hr) { throw ResultException(hr); }
    inline HRESULT ResultFromCaughtException() noexcept
    {
        try { throw; }
        catch (const ResultException& e) { return e.hr; }
        catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
        catch (...) { return E_FAIL; }
    }

    template<typename F>
    struct scope_exit_t
    {
        F f;
        bool active = true;
        explicit scope_exit_t(F&& fn) : f(std::move(fn)) {}
        scope_exit_t(scope_exit_t&& o) noexcept : f(std::move(o.f)), active(o.active) { o.active = false; }
        ~scope_exit_t() { if (active) f(); }
        void release() noexcept { active = false; }
        void reset() { if (active) { active = false; f(); } }
    };
    template<typename F>
    scope_exit_t<F> scope_exit(F&& f) { return scope_exit_t<F>(std::forward<F>(f)); }

    struct zwstring_view : std::wstring_view
    {
        using std::wstring_view::wstring_view;
        zwstring_view(const std::wstring& s) noexcept : std::wstring_view(s) {}
        const wchar_t* c_str() const noexcept { return data(); }
    };

    template<typename T = std::wstring, typename... Args>
    T str_printf(const wchar_t* fmt, Args... args)
    {
        wchar_t buf[512];
        const int n = std::swprintf(buf, 512, fmt, args...);
        return n < 0 ? T{} : T(buf, static_cast<size_t>(n));
    }
    template<typename T = std::wstring, typename... Args>
    T str_printf_failfast(const wchar_t* fmt, Args... args) { return str_printf<T>(fmt, args...); }

    // RAII handle wrapper; nothing in the fuzzed code opens handles on Linux.
    template<typename H = HANDLE>
    struct unique_any_handle
    {
        H h{};
        unique_any_handle() = default;
        explicit unique_any_handle(H v) noexcept : h(v) {}
        unique_any_handle(unique_any_handle&& o) noexcept : h(std::exchange(o.h, H{})) {}
        unique_any_handle& operator=(unique_any_handle&& o) noexcept { h = std::exchange(o.h, H{}); return *this; }
        H get() const noexcept { return h; }
        H release() noexcept { return std::exchange(h, H{}); }
        void reset(H v = H{}) noexcept { h = v; }
        H* put() noexcept { return &h; }
        H* addressof() noexcept { return &h; }
        H* operator&() noexcept { return &h; }
        explicit operator bool() const noexcept { return h != H{}; }
    };
    using unique_hfile = unique_any_handle<HANDLE>;
    using unique_handle = unique_any_handle<HANDLE>;
}

#define THROW_HR(hr) ::wil::details_throw(hr)
#define THROW_HR_MSG(hr, ...) ::wil::details_throw(hr)
#define THROW_HR_IF(hr, cond) do { if (cond) { ::wil::details_throw(hr); } } while (0)
#define THROW_HR_IF_MSG(hr, cond, ...) THROW_HR_IF(hr, cond)
#define THROW_HR_IF_NULL(hr, p) do { if ((p) == nullptr) { ::wil::details_throw(hr); } } while (0)
#define THROW_IF_FAILED(x) do { const HRESULT __hr = (x); if (FAILED(__hr)) { ::wil::details_throw(__hr); } } while (0)
#define THROW_IF_FAILED_MSG(x, ...) THROW_IF_FAILED(x)
#define THROW_LAST_ERROR() ::wil::details_throw(E_FAIL)
#define THROW_LAST_ERROR_IF(cond) do { if (cond) { ::wil::details_throw(E_FAIL); } } while (0)
#define THROW_LAST_ERROR_IF_NULL(p) THROW_LAST_ERROR_IF((p) == nullptr)
#define THROW_WIN32(e) ::wil::details_throw(HRESULT_FROM_WIN32(e))
#define THROW_WIN32_IF(e, cond) do { if (cond) { THROW_WIN32(e); } } while (0)
#define THROW_IF_WIN32_ERROR(e) do { const auto __e = (e); if (__e != 0) { THROW_WIN32(__e); } } while (0)
#define THROW_IF_NULL_ALLOC(p) do { if ((p) == nullptr) { throw std::bad_alloc(); } } while (0)
#define THROW_IF_NTSTATUS_FAILED(x) do { if ((x) < 0) { ::wil::details_throw(E_FAIL); } } while (0)
#define THROW_IF_WIN32_BOOL_FALSE(x) do { if (!(x)) { ::wil::details_throw(E_FAIL); } } while (0)
#define RETURN_HR(hr) return (hr)
#define RETURN_HR_IF(hr, cond) do { if (cond) { return (hr); } } while (0)
#define RETURN_HR_IF_NULL(hr, p) do { if ((p) == nullptr) { return (hr); } } while (0)
#define RETURN_IF_FAILED(x) do { const HRESULT __hr = (x); if (FAILED(__hr)) { return __hr; } } while (0)
#define RETURN_LAST_ERROR_IF(cond) do { if (cond) { return E_FAIL; } } while (0)
#define RETURN_WIN32(e) return HRESULT_FROM_WIN32(e)
#define RETURN_IF_WIN32_BOOL_FALSE(x) do { if (!(x)) { return E_FAIL; } } while (0)
#define SUCCEEDED_LOG(x) SUCCEEDED(x)
#define FAILED_LOG(x) FAILED(x)
#define LOG_HR(hr) ((void)(hr))
#define LOG_HR_MSG(hr, ...) ((void)(hr))
#define LOG_IF_FAILED(x) ((void)(x))
#define LOG_HR_IF(hr, cond) ((void)(cond))
#define LOG_LAST_ERROR() ((void)0)
#define LOG_LAST_ERROR_IF(cond) ((void)(cond))
#define LOG_CAUGHT_EXCEPTION() ((void)0)
#define LOG_CAUGHT_EXCEPTION_MSG(...) ((void)0)
#define FAIL_FAST() __builtin_trap()
#define FAIL_FAST_HR(hr) __builtin_trap()
#define FAIL_FAST_IF(cond) do { if (cond) { __builtin_trap(); } } while (0)
#define FAIL_FAST_IF_FAILED(x) do { if (FAILED(x)) { __builtin_trap(); } } while (0)
#define FAIL_FAST_LAST_ERROR_IF(cond) FAIL_FAST_IF(cond)
#define FAIL_FAST_CAUGHT_EXCEPTION() __builtin_trap()
#define CATCH_RETURN() catch (...) { return ::wil::ResultFromCaughtException(); }
#define CATCH_LOG() catch (...) { }
#define CATCH_LOG_RETURN_FALSE() catch (...) { return false; }
#define CATCH_FAIL_FAST() catch (...) { __builtin_trap(); }
#define CATCH_THROW_NORMALIZED() catch (...) { throw; }

// ---- TraceLogging: compiled out ------------------------------------------
#define TRACELOGGING_DECLARE_PROVIDER(h) extern int h
#define TRACELOGGING_DEFINE_PROVIDER(h, ...) int h = 0
#define TraceLoggingWrite(...) ((void)0)
#define TraceLoggingRegister(h) ((void)0)
#define TraceLoggingUnregister(h) ((void)0)
#define TraceLoggingProviderEnabled(...) false
#define WINEVENT_LEVEL_VERBOSE 5

// mayhem shim: definitions for the Win32 functions declared in shim/include.
//
// * MultiByteToWideChar / WideCharToMultiByte implement CP_UTF8 the way Windows does:
//   UTF-16 code units (one per wchar_t), U+FFFD for each maximal invalid subpart,
//   a "measure only" mode when the output length is 0, and failure with
//   ERROR_INSUFFICIENT_BUFFER when the output is too small. til::u8u16 builds on them.
// * The VK/keyboard helpers give the input parser a fixed US layout.
// * Everything else is reachable only from Windows-only code paths (pipes, tokens,
//   ICU search, GUIDs) and fails cleanly.
#include "LibraryIncludes.h"
#include "mayhem_nt.h"

#include <sys/mman.h>
#include <unistd.h>

static thread_local DWORD g_lastError = 0;
DWORD GetLastError() noexcept { return g_lastError; }
void SetLastError(DWORD e) noexcept { g_lastError = e; }


// ---- UTF-8 <-> UTF-16 --------------------------------------------------------
static size_t utf8_decode_one(const unsigned char* s, size_t n, uint32_t* cp) noexcept
{
    // Returns the number of bytes consumed; *cp = 0xFFFD for an invalid maximal subpart.
    const unsigned char c = s[0];
    if (c < 0x80) { *cp = c; return 1; }
    size_t need;
    uint32_t v;
    unsigned char lo = 0x80, hi = 0xBF;
    if (c >= 0xC2 && c <= 0xDF) { need = 1; v = c & 0x1F; }
    else if (c >= 0xE0 && c <= 0xEF) { need = 2; v = c & 0x0F; if (c == 0xE0) lo = 0xA0; if (c == 0xED) hi = 0x9F; }
    else if (c >= 0xF0 && c <= 0xF4) { need = 3; v = c & 0x07; if (c == 0xF0) lo = 0x90; if (c == 0xF4) hi = 0x8F; }
    else { *cp = 0xFFFD; return 1; }
    size_t i = 1;
    for (; i <= need; ++i)
    {
        if (i >= n) { *cp = 0xFFFD; return i; }
        const unsigned char cc = s[i];
        if (cc < lo || cc > hi) { *cp = 0xFFFD; return i; }
        lo = 0x80; hi = 0xBF;
        v = (v << 6) | (cc & 0x3F);
    }
    *cp = v;
    return i;
}

int MultiByteToWideChar(UINT codePage, DWORD flags, const char* src, int srcLen, wchar_t* dst, int dstLen) noexcept
{
    if (codePage != CP_UTF8 || src == nullptr || srcLen == 0 || dstLen < 0 || (dstLen > 0 && dst == nullptr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    const size_t n = srcLen < 0 ? std::strlen(src) + 1 : static_cast<size_t>(srcLen);
    const auto* s = reinterpret_cast<const unsigned char*>(src);
    size_t out = 0;
    for (size_t i = 0; i < n;)
    {
        uint32_t cp;
        i += utf8_decode_one(s + i, n - i, &cp);
        if (cp == 0xFFFD && (flags & MB_ERR_INVALID_CHARS))
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
        const size_t units = cp >= 0x10000 ? 2 : 1;
        if (dstLen > 0)
        {
            if (out + units > static_cast<size_t>(dstLen))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            if (units == 2)
            {
                dst[out] = static_cast<wchar_t>(0xD800 + ((cp - 0x10000) >> 10));
                dst[out + 1] = static_cast<wchar_t>(0xDC00 + ((cp - 0x10000) & 0x3FF));
            }
            else
            {
                dst[out] = static_cast<wchar_t>(cp);
            }
        }
        out += units;
    }
    return static_cast<int>(out);
}

int WideCharToMultiByte(UINT codePage, DWORD flags, const wchar_t* src, int srcLen, char* dst, int dstLen, const char* defChar, BOOL* usedDef) noexcept
{
    (void)defChar;
    if (usedDef) *usedDef = FALSE;
    if (codePage != CP_UTF8 || src == nullptr || srcLen == 0 || dstLen < 0 || (dstLen > 0 && dst == nullptr))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    size_t n = 0;
    if (srcLen < 0) { while (src[n]) ++n; ++n; }
    else n = static_cast<size_t>(srcLen);
    size_t out = 0;
    for (size_t i = 0; i < n; ++i)
    {
        uint32_t cp = static_cast<uint32_t>(src[i]) & 0xFFFF;
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < n && (static_cast<uint32_t>(src[i + 1]) & 0xFC00) == 0xDC00)
        {
            cp = 0x10000 + ((cp - 0xD800) << 10) + ((static_cast<uint32_t>(src[i + 1]) & 0xFFFF) - 0xDC00);
            ++i;
        }
        else if (cp >= 0xD800 && cp <= 0xDFFF)
        {
            if (flags & WC_ERR_INVALID_CHARS)
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                return 0;
            }
            cp = 0xFFFD;
        }
        unsigned char buf[4];
        size_t len;
        if (cp < 0x80) { buf[0] = static_cast<unsigned char>(cp); len = 1; }
        else if (cp < 0x800) { buf[0] = 0xC0 | (cp >> 6); buf[1] = 0x80 | (cp & 0x3F); len = 2; }
        else if (cp < 0x10000) { buf[0] = 0xE0 | (cp >> 12); buf[1] = 0x80 | ((cp >> 6) & 0x3F); buf[2] = 0x80 | (cp & 0x3F); len = 3; }
        else { buf[0] = 0xF0 | (cp >> 18); buf[1] = 0x80 | ((cp >> 12) & 0x3F); buf[2] = 0x80 | ((cp >> 6) & 0x3F); buf[3] = 0x80 | (cp & 0x3F); len = 4; }
        if (dstLen > 0)
        {
            if (out + len > static_cast<size_t>(dstLen))
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            std::memcpy(dst + out, buf, len);
        }
        out += len;
    }
    return static_cast<int>(out);
}

// ---- string comparison (ordinal; no locale tables on Linux) ---------------------
static wchar_t fold(wchar_t c, bool ic) noexcept { return (ic && c >= L'a' && c <= L'z') ? c - 32 : c; }
int CompareStringOrdinal(const wchar_t* a, int aLen, const wchar_t* b, int bLen, BOOL ignoreCase) noexcept
{
    const size_t an = aLen < 0 ? std::char_traits<wchar_t>::length(a) : aLen;
    const size_t bn = bLen < 0 ? std::char_traits<wchar_t>::length(b) : bLen;
    for (size_t i = 0; i < std::min(an, bn); ++i)
    {
        const auto x = fold(a[i], ignoreCase), y = fold(b[i], ignoreCase);
        if (x != y) return x < y ? CSTR_LESS_THAN : CSTR_GREATER_THAN;
    }
    return an == bn ? CSTR_EQUAL : (an < bn ? CSTR_LESS_THAN : CSTR_GREATER_THAN);
}
int CompareStringEx(const wchar_t*, DWORD, const wchar_t* a, int aLen, const wchar_t* b, int bLen, void*, void*, LPARAM) noexcept
{
    return CompareStringOrdinal(a, aLen, b, bLen, TRUE);
}
int FindNLSStringEx(const wchar_t*, DWORD, const wchar_t* src, int srcLen, const wchar_t* find, int findLen, int* foundLen, void*, void*, LPARAM) noexcept
{
    for (int i = 0; i + findLen <= srcLen; ++i)
    {
        if (CompareStringOrdinal(src + i, findLen, find, findLen, TRUE) == CSTR_EQUAL)
        {
            if (foundLen) *foundLen = findLen;
            return i;
        }
    }
    return -1;
}
int LCMapStringEx(const wchar_t*, DWORD flags, const wchar_t* src, int srcLen, wchar_t* dst, int dstLen, void*, void*, LPARAM) noexcept
{
    if (srcLen < 0 || dstLen < srcLen) return 0;
    for (int i = 0; i < srcLen; ++i)
        dst[i] = (flags & LCMAP_LOWERCASE) && src[i] >= L'A' && src[i] <= L'Z' ? src[i] + 32 : src[i];
    return srcLen;
}
namespace til
{
    bool equals_insensitive_ascii(const std::string& a, const std::wstring& b) noexcept
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (fold(static_cast<wchar_t>(a[i]), true) != fold(b[i], true)) return false;
        return true;
    }
}

// ---- keyboard: fixed US layout for the VT input parser -----------------------------
UINT GetDoubleClickTime() noexcept { return 500; }
extern "C" SHORT OneCoreSafeVkKeyScanW(WCHAR ch)
{
    if (ch >= L'a' && ch <= L'z') return static_cast<SHORT>(ch - L'a' + 'A');
    if (ch >= L'A' && ch <= L'Z') return static_cast<SHORT>(0x100 | ch);
    if (ch >= L'0' && ch <= L'9') return static_cast<SHORT>(ch);
    if (ch == L' ') return VK_SPACE;
    if (ch == L'\r') return VK_RETURN;
    if (ch == L'\t') return VK_TAB;
    if (ch == L'\b') return VK_BACK;
    if (ch == 0x1b) return VK_ESCAPE;
    return -1;
}
extern "C" UINT OneCoreSafeMapVirtualKeyW(UINT code, UINT type)
{
    if (type == MAPVK_VK_TO_CHAR)
    {
        if (code >= 'A' && code <= 'Z') return code;
        if (code >= '0' && code <= '9') return code;
        if (code == VK_SPACE) return L' ';
        return 0;
    }
    if (type == MAPVK_VK_TO_VSC) return code & 0x7F;
    return 0;
}
extern "C" SHORT OneCoreSafeGetKeyState(int) { return 0; }
SHORT GetKeyState(int) noexcept { return 0; }
HWND GetForegroundWindow() noexcept { return nullptr; }
DWORD GetWindowThreadProcessId(HWND, DWORD*) noexcept { return 0; }
HKL GetKeyboardLayout(DWORD) noexcept { return nullptr; }
UINT MapVirtualKeyExW(UINT code, UINT type, HKL) noexcept { return OneCoreSafeMapVirtualKeyW(code, type); }
int ToUnicodeEx(UINT, UINT, const BYTE*, wchar_t*, int, UINT, HKL) noexcept { return 0; }
SHORT VkKeyScanExW(WCHAR ch, HKL) noexcept { return OneCoreSafeVkKeyScanW(ch); }

// ---- WaitOnAddress: single-threaded harnesses never block -------------------------
BOOL WaitOnAddress(volatile void* address, void* compareAddress, SIZE_T size, DWORD) noexcept
{
    return std::memcmp(const_cast<void*>(address), compareAddress, size) != 0;
}
void WakeByAddressSingle(void*) noexcept {}
void WakeByAddressAll(void*) noexcept {}

// ---- Windows-only paths of src/types/utils.cpp (unreachable from the harnesses) ----
int swprintf_s(wchar_t* buf, size_t n, const wchar_t* fmt, ...) noexcept
{
    va_list ap;
    va_start(ap, fmt);
    const int r = std::vswprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}
int wcscpy_s(wchar_t* dst, size_t n, const wchar_t* src) noexcept
{
    const size_t len = std::char_traits<wchar_t>::length(src);
    if (len + 1 > n) return 34; // ERANGE
    std::char_traits<wchar_t>::copy(dst, src, len + 1);
    return 0;
}
int memcpy_s(void* dst, size_t dstSize, const void* src, size_t n) noexcept
{
    if (n > dstSize) return 34;
    std::memcpy(dst, src, n);
    return 0;
}
HRESULT IIDFromString(const wchar_t*, GUID*) noexcept { return CO_E_CLASSSTRING; }
HRESULT CoCreateGuid(GUID*) noexcept { return E_NOTIMPL; }
BOOL CreatePipe(PHANDLE, PHANDLE, void*, DWORD) noexcept { return FALSE; }
extern "C" NTSTATUS NtCreateFile(PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG) noexcept { return static_cast<NTSTATUS>(0xC0000002); }
extern "C" NTSTATUS NtCreateNamedPipeFile(PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, PLARGE_INTEGER) noexcept { return static_cast<NTSTATUS>(0xC0000002); }
extern "C" NTSTATUS NtQueryInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS) noexcept { return static_cast<NTSTATUS>(0xC0000002); }
DWORD WaitForSingleObjectEx(HANDLE, DWORD, BOOL) noexcept { return 0xFFFFFFFF; }
NTSTATUS BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE*, PUCHAR, ULONG, PUCHAR, ULONG, ULONG) noexcept { return static_cast<NTSTATUS>(0xC0000002); }
NTSTATUS BCryptHashData(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG) noexcept { return static_cast<NTSTATUS>(0xC0000002); }
NTSTATUS BCryptFinishHash(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG) noexcept { return static_cast<NTSTATUS>(0xC0000002); }
HANDLE GetCurrentProcessToken() noexcept { return nullptr; }
BOOL GetFileAttributesExW(const wchar_t*, GET_FILEEX_INFO_LEVELS, void*) noexcept { return FALSE; }
HRESULT PathCreateFromUrlW(const wchar_t*, wchar_t*, DWORD*, DWORD) noexcept { return E_NOTIMPL; }
BOOL VerifyVersionInfoW(OSVERSIONINFOEXW*, DWORD, DWORDLONG) noexcept { return FALSE; }
namespace wil
{
    bool test_token_membership(void*, const SID_IDENTIFIER_AUTHORITY&, DWORD, DWORD) { throw ResultException(E_NOTIMPL); }
    HRESULT GetSystemDirectoryW(std::wstring&) noexcept { return E_NOTIMPL; }
}
UBreakIterator* ubrk_open(int, const char*, const UChar*, int32_t, UErrorCode* status) noexcept { *status = U_UNSUPPORTED_ERROR; return nullptr; }
int32_t ubrk_next(UBreakIterator*) noexcept { return UBRK_DONE; }
int32_t ubrk_first(UBreakIterator*) noexcept { return 0; }
void ubrk_close(UBreakIterator*) noexcept {}

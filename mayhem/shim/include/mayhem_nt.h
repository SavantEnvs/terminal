// mayhem shim: declarations for the Windows-only helpers in src/types/utils.cpp
// (NT pipes, BCrypt, tokens, shell paths, ICU break iterator) so that file compiles
// as-is. None of these are reachable from the fuzz harnesses; their stub definitions
// in mayhem/shim/win32_shim.cpp simply report failure.
#pragma once
#include "mayhem_win32.h"
#include <emmintrin.h>
#include <filesystem>

typedef uint64_t DWORDLONG;
typedef uint8_t* PUCHAR;
typedef HANDLE* PHANDLE;
typedef void* BCRYPT_HASH_HANDLE;
typedef void* BCRYPT_ALG_HANDLE;
typedef union _LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; } u; LONGLONG QuadPart; } LARGE_INTEGER, *PLARGE_INTEGER;
typedef struct _UNICODE_STRING { USHORT Length; USHORT MaximumLength; PWSTR Buffer; } UNICODE_STRING, *PUNICODE_STRING;
typedef struct _OBJECT_ATTRIBUTES { ULONG Length; HANDLE RootDirectory; PUNICODE_STRING ObjectName; ULONG Attributes; PVOID SecurityDescriptor; PVOID SecurityQualityOfService; } OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef struct _IO_STATUS_BLOCK { union { NTSTATUS Status; PVOID Pointer; }; ULONG_PTR Information; } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
typedef int FILE_INFORMATION_CLASS;
typedef struct _WIN32_FILE_ATTRIBUTE_DATA { DWORD dwFileAttributes; } WIN32_FILE_ATTRIBUTE_DATA;
typedef struct _OSVERSIONINFOEXW { DWORD dwOSVersionInfoSize; DWORD dwMajorVersion; DWORD dwMinorVersion; DWORD dwBuildNumber; DWORD dwPlatformId; WCHAR szCSDVersion[128]; WORD wServicePackMajor; WORD wServicePackMinor; WORD wSuiteMask; BYTE wProductType; BYTE wReserved; } OSVERSIONINFOEXW;
typedef enum _TOKEN_ELEVATION_TYPE { TokenElevationTypeDefault = 1, TokenElevationTypeFull, TokenElevationTypeLimited } TOKEN_ELEVATION_TYPE;
typedef struct _TOKEN_ELEVATION { DWORD TokenIsElevated; } TOKEN_ELEVATION;
typedef struct _SID_IDENTIFIER_AUTHORITY { BYTE Value[6]; } SID_IDENTIFIER_AUTHORITY;
enum GET_FILEEX_INFO_LEVELS { GetFileExInfoStandard };

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define STATUS_PENDING ((DWORD)0x00000103L)
#define WAIT_OBJECT_0 0
#define HRESULT_FROM_NT(x) ((HRESULT)((x) | 0x10000000))
#define CO_E_CLASSSTRING ((HRESULT)0x800401F3L)
#define OBJ_CASE_INSENSITIVE 0x00000040L
#define PIPE_ACCESS_INBOUND 0x00000001
#define PIPE_ACCESS_OUTBOUND 0x00000002
#define PIPE_ACCESS_DUPLEX 0x00000003
#define SYNCHRONIZE 0x00100000L
#define GENERIC_READ 0x80000000L
#define GENERIC_WRITE 0x40000000L
#define FILE_READ_ATTRIBUTES 0x0080
#define FILE_WRITE_ATTRIBUTES 0x0100
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define FILE_OPEN 0x00000001
#define FILE_CREATE 0x00000002
#define FILE_SYNCHRONOUS_IO_ALERT 0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_NON_DIRECTORY_FILE 0x00000040
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define URL_UNESCAPE_AS_UTF8 0x00040000
#define VER_BUILDNUMBER 0x0000004
#define VER_GREATER_EQUAL 3
#define VER_SET_CONDITION(m, t, c) ((m) |= 0)
#define SECURITY_NT_AUTHORITY { { 0, 0, 0, 0, 0, 5 } }
#define SECURITY_BUILTIN_DOMAIN_RID 0x00000020L
#define DOMAIN_ALIAS_RID_ADMINS 0x00000220L
#define BCRYPT_SHA1_ALG_HANDLE ((BCRYPT_ALG_HANDLE)0x00000031)
#define IS_HIGH_SURROGATE(wch) (((wch) >= 0xd800) && ((wch) <= 0xdbff))
#define IS_LOW_SURROGATE(wch) (((wch) >= 0xdc00) && ((wch) <= 0xdfff))
#define RTL_CONSTANT_STRING(s) { (USHORT)(sizeof(s) - sizeof((s)[0])), (USHORT)sizeof(s), (PWSTR)(s) }

int swprintf_s(wchar_t* buf, size_t n, const wchar_t* fmt, ...) noexcept;
int wcscpy_s(wchar_t* dst, size_t n, const wchar_t* src) noexcept;
int memcpy_s(void* dst, size_t dstSize, const void* src, size_t n) noexcept;
HRESULT IIDFromString(const wchar_t* str, GUID* out) noexcept;
HRESULT CoCreateGuid(GUID* out) noexcept;
BOOL CreatePipe(PHANDLE r, PHANDLE w, void* sa, DWORD size) noexcept;
extern "C" NTSTATUS NtCreateFile(PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG) noexcept;
DWORD WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable) noexcept;
NTSTATUS BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE*, PUCHAR, ULONG, PUCHAR, ULONG, ULONG) noexcept;
NTSTATUS BCryptHashData(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG) noexcept;
NTSTATUS BCryptFinishHash(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG) noexcept;
HANDLE GetCurrentProcessToken() noexcept;
BOOL GetFileAttributesExW(const wchar_t* path, GET_FILEEX_INFO_LEVELS level, void* data) noexcept;
HRESULT PathCreateFromUrlW(const wchar_t* url, wchar_t* path, DWORD* len, DWORD flags) noexcept;
BOOL VerifyVersionInfoW(OSVERSIONINFOEXW* info, DWORD mask, DWORDLONG cond) noexcept;

namespace wil
{
    using unique_bcrypt_hash = unique_any_handle<BCRYPT_HASH_HANDLE>;
    template<typename T>
    T get_token_information(HANDLE) { throw ResultException(E_NOTIMPL); }
    bool test_token_membership(void* token, const SID_IDENTIFIER_AUTHORITY& authority, DWORD a, DWORD b);
    HRESULT GetSystemDirectoryW(std::wstring& out) noexcept;
    template<typename T>
    T ExpandEnvironmentStringsW(const wchar_t*) { throw ResultException(E_NOTIMPL); }
}

// On Linux std::filesystem::path::native() is a narrow string; the WSL path helper
// in utils.cpp compares it against a wide one.
namespace til
{
    bool equals_insensitive_ascii(const std::string& a, const std::wstring& b) noexcept;
}

inline unsigned char _BitScanForward(unsigned long* index, unsigned long mask) noexcept
{
    if (!mask) return 0;
    *index = static_cast<unsigned long>(__builtin_ctzl(mask));
    return 1;
}

#include "mayhem_icu.h"

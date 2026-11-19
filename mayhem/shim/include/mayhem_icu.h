// mayhem shim: the ICU C API surface named by Windows Terminal (Windows ships ICU as
// <icu.h> with UChar == wchar_t). It is only used by text search / URL detection
// (TextBuffer::SearchText) and emoji detection in Utils, none of which the VT stream
// reaches; the stubs in win32_shim.cpp fail with U_UNSUPPORTED_ERROR.
#pragma once
#include <cstdint>
#include <memory>

typedef uint16_t UChar;
typedef int8_t UBool;
typedef int UErrorCode;
struct UText { int64_t chunkNativeStart; };
typedef struct UBreakIterator UBreakIterator;
typedef struct URegularExpression URegularExpression;
#define U_CALLCONV
#define U_ZERO_ERROR 0
#define U_ILLEGAL_ARGUMENT_ERROR 1
#define U_MEMORY_ALLOCATION_ERROR 7
#define U_BUFFER_OVERFLOW_ERROR 15
#define U_UNSUPPORTED_ERROR 16
#define U_FAILURE(x) ((x) > U_ZERO_ERROR)
#define U_SUCCESS(x) ((x) <= U_ZERO_ERROR)
#define UBRK_CHARACTER 0
#define UBRK_DONE (-1)
#define UREGEX_CASE_INSENSITIVE 2
#define UREGEX_MULTILINE 8
#define UREGEX_LITERAL 16
UBreakIterator* ubrk_open(int type, const char* locale, const UChar* text, int32_t len, UErrorCode* status) noexcept;
int32_t ubrk_next(UBreakIterator* bi) noexcept;
int32_t ubrk_first(UBreakIterator* bi) noexcept;
void ubrk_close(UBreakIterator* bi) noexcept;
UText* utext_close(UText* ut) noexcept;
URegularExpression* uregex_open(const char16_t* pattern, int32_t len, uint32_t flags, void* pe, UErrorCode* status) noexcept;
void uregex_close(URegularExpression* re) noexcept;
void uregex_setTimeLimit(URegularExpression* re, int32_t limit, UErrorCode* status) noexcept;
void uregex_setStackLimit(URegularExpression* re, int32_t limit, UErrorCode* status) noexcept;
void uregex_setUText(URegularExpression* re, UText* text, UErrorCode* status) noexcept;
UBool uregex_find(URegularExpression* re, int32_t start, UErrorCode* status) noexcept;
UBool uregex_findNext(URegularExpression* re, UErrorCode* status) noexcept;
int64_t uregex_start64(URegularExpression* re, int32_t group, UErrorCode* status) noexcept;
int64_t uregex_end64(URegularExpression* re, int32_t group, UErrorCode* status) noexcept;

namespace wistd
{
    using std::unique_ptr;
}
namespace wil
{
    template<typename F, F fn>
    struct function_deleter
    {
        template<typename T>
        void operator()(T* p) const noexcept { fn(p); }
    };
    template<typename T, typename F, F close>
    struct unique_struct : T
    {
        unique_struct() noexcept : T{} {}
        unique_struct(unique_struct&& o) noexcept : T(static_cast<T&>(o)) { static_cast<T&>(o) = T{}; }
        unique_struct& operator=(unique_struct&& o) noexcept { static_cast<T&>(*this) = static_cast<T&>(o); return *this; }
        ~unique_struct() = default;
    };
}

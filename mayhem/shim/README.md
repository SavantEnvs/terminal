# Linux build shim for Windows Terminal's portable core

Upstream builds only with MSBuild/MSVC. `mayhem/build.sh` compiles upstream's own `.cpp`
files unmodified with clang on Linux; this directory supplies what `<windows.h>` and friends
would:

- `include/LibraryIncludes.h` replaces `src/inc/LibraryIncludes.h` (it is found first on the
  include path): the STL, GSL, chromium `safe_math`, `{fmt}` and TIL, without WIL/WRL.
- `include/mayhem_win32.h`: Win32 types and constants, SAL as empty macros, WIL's
  `THROW_*`/`RETURN_*`/`WI_*` macros mapped to C++ exceptions/flag operations, and
  TraceLogging compiled out. `mayhem_nt.h` / `mayhem_icu.h` declare the NT, BCrypt, shell and
  ICU calls that `src/types/utils.cpp` and the text-search code name (never reached by a
  target). The remaining `include/*.h` files are one-line forwarders to these.
- `win32_shim.cpp`: definitions. `MultiByteToWideChar`/`WideCharToMultiByte` implement CP_UTF8
  with Windows semantics (one UTF-16 code unit per `wchar_t`, U+FFFD per invalid maximal
  subpart, size query when the output length is 0); `OneCoreSafe*` give the input parser a
  fixed US keyboard layout; everything else fails cleanly.
- `case/`: aliases for includes whose case differs from the file name (`Viewport.hpp` vs
  `viewport.hpp`), which Windows file systems tolerate.
- `../gen_features.py` produces `TilFeatureStaging.h` from `src/features.xml` (Dev branding),
  as `tools/Generate-FeatureStagingHeader.ps1` does for MSBuild.

`wchar_t` is 32 bits on Linux. The targets only ever store UTF-16 code units in it, so the
parsers see the same values as on Windows. One visible difference: the SSE2 scan in
`Utils::FindActionableControlCharacter` reads 16-bit lanes, so it stops after one or two
characters and print runs reach the dispatch in smaller pieces; the printed text is the same.

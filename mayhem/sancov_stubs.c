// No-op SanitizerCoverage callbacks for the -standalone reproducers. Upstream's objects are
// compiled with -fsanitize=fuzzer-no-link so libFuzzer sees their coverage; the standalone
// driver has no libFuzzer, and without ASan/UBSan (an empty SANITIZER_FLAGS build) nothing
// else defines these. Weak, so a sanitizer runtime's own definitions win when present.
#include <stddef.h>
#include <stdint.h>
#define W __attribute__((weak))
W __thread uintptr_t __sancov_lowest_stack;
W void __sanitizer_cov_trace_pc_indir(uintptr_t c) { (void)c; }
W void __sanitizer_cov_trace_cmp1(uint8_t a, uint8_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_cmp2(uint16_t a, uint16_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_cmp4(uint32_t a, uint32_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_cmp8(uint64_t a, uint64_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_const_cmp1(uint8_t a, uint8_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_const_cmp2(uint16_t a, uint16_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_const_cmp4(uint32_t a, uint32_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_const_cmp8(uint64_t a, uint64_t b) { (void)a; (void)b; }
W void __sanitizer_cov_trace_switch(uint64_t v, uint64_t* c) { (void)v; (void)c; }
W void __sanitizer_cov_trace_div4(uint32_t v) { (void)v; }
W void __sanitizer_cov_trace_div8(uint64_t v) { (void)v; }
W void __sanitizer_cov_trace_gep(uintptr_t i) { (void)i; }
W void __sanitizer_cov_8bit_counters_init(uint8_t* b, uint8_t* e) { (void)b; (void)e; }
W void __sanitizer_cov_bool_flag_init(uint8_t* b, uint8_t* e) { (void)b; (void)e; }
W void __sanitizer_cov_pcs_init(const uintptr_t* b, const uintptr_t* e) { (void)b; (void)e; }
W void __sanitizer_cov_trace_pc_guard_init(uint32_t* b, uint32_t* e) { (void)b; (void)e; }
W void __sanitizer_cov_trace_pc_guard(uint32_t* g) { (void)g; }

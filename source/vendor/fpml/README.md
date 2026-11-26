# fpml — Fixed Point Math Library

Upstream: Peter Schregle, copyright 2007–2009.
Source repository: https://github.com/PoL0/fpmath
Pinned commit: `27d638c8c616de87c520d20b88035bb7984f0192`
Header: https://raw.githubusercontent.com/PoL0/fpmath/27d638c8c616de87c520d20b88035bb7984f0192/include/fpml/fixed_point.h
Upstream SHA-256: `4789f8a26466428327697410e291d2df45dbecd195bd8632833fc045ba7113d0`
Vendored SHA-256: `746eadb8ec4294e9ef251ce81826896ba26189a7d342f28e0193bb9870fc0173`
License: BSD-style three-clause notice in the header, reproduced in LICENSE.

## Authenticity and deviation

The single header retains upstream bytes, including CRLF, except the documented
friend declaration and its adjacent vendor note. Replacing that block with
`friend class fpml::fixed_point;` restores the upstream file byte-for-byte.
The replacement is a template friend declaration granting every specialization
access. GCC 9 rejects the old declaration in C++17; no permissive compiler flag
or warning suppression is used.

The binary proves template identity (`long long, 48, 15` and `int, 10, 21`),
qword/dword storage, nontrivial value passing and conversion returns. It does
**not** pin this upstream revision: no fpml source paths or usable C++ DWARF
were recovered. The binary additionally contains `One`/`Zero` static members
absent upstream, so its source was a fork. Those unused app-side symbols are
not reconstructed here. The selected upstream implementation supplies the real
user-provided copy constructor, not an invented destructor to force sret.

## Include closure

Include as `<fpml/fixed_point.h>` with `-isystem source/vendor`. Dependencies
(`boost/assert.hpp`, `boost/static_assert.hpp`, `boost/operators.hpp`,
`boost/concept_check.hpp`) use the existing Boost 1.67.0 vendor. Extract with:

```
g++ -std=c++17 -D_GTHREAD_USE_COND_INIT_FUNC=1 -isystem source/vendor -M seed.cpp
clang++ -std=c++17 -D_GTHREAD_USE_COND_INIT_FUNC=1 -isystem source/vendor -M seed.cpp
```

Here seed.cpp contains only `#include <fpml/fixed_point.h>`. Union both closures;
retain compiler-specific Boost configuration headers. Do not vendor all Boost.

## Corpus representation bridge

`CFixedPoint.hpp` includes this header and aliases the exact specialization.
Upstream's integer constructor converts whole units by shifting left 15; its
default constructor leaves storage uninitialized. `FPMLFromRaw` and `RawFPML`
are separate, checked, byte-copy bridges for the binary's raw-qword expressions,
not modifications of upstream or aliasing access to its private member.
Size/alignment/standard-layout assertions guard that representation. Writing
through an unsigned-char pointer avoids both type punning and copying into a
nontrivial class with memcpy's class pointer warning.

Addition, subtraction and comparison consumers use upstream operators directly.
Scaled products and signed quotients retain their exact old integer evaluation
order through the bridge. Upstream `promote_type` has no `long long`
specialization, so 64-bit multiply/divide operators cannot be substituted.
No new promotion or arithmetic is invented in the vendor. ABI and arithmetic
controls: `python3 tests/test_fpml/run_contract.py --help`.

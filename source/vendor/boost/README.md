boost 1.67.0 — vendored headers
===============================
Source: https://archives.boost.io/release/1.67.0/source/boost_1_67_0.tar.gz
Pinned by the binary: assert paths embed /mnt/external_libs2/boost_1_67_0/
Subset: dependency-closed closure of boost/dynamic_bitset.hpp under
gcc-9 -std=c++17 -M (51 headers; the only boost component modeled so far).
Grow: add the next boost header to the seed TU, re-run the -M closure, copy
the delta. License: BSL-1.0 (LICENSE_1_0.txt).

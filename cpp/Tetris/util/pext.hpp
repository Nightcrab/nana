#include <concepts>
#include <cstdint>
#include <climits>
#include <type_traits>

// if either clang/gcc
#ifdef __GNUC__
#ifndef __clang__
#ifdef __BMI2__
// gcc
#include <immintrin.h>
#endif
#elif
// clang
#include <immintrin.h>
#endif
#endif

// this is not specific to any architecture or integer size
// but its fine cause we should only be using this on u32s or u64s

template <std::integral T>
constexpr T pext_impl(const T SRC, const T MASK) {
    T DEST = 0;
    int m = 0, k = 0;  // iterators
    do {
        if ((MASK >> m) & 1) {
            DEST |= (SRC & (1 << m)) >> (m - k);
            k = k + 1;
        }
        m = m + 1;
    } while (m < sizeof(T) * CHAR_BIT);

    return DEST;
}

template <std::integral T>
constexpr T pext(const T src, const T mask) {
    if (std::is_constant_evaluated()) {
        return pext_impl(src, mask);
    }

#if defined(__GNUC__) && defined(__BMI2__)

    if constexpr (std::same_as<T, std::uint64_t>) {
        return _pext_u64(src, mask);
    }
    else if constexpr (std::same_as<T, std::uint32_t>) {
        return _pext_u32(src, mask);
    }
    else {
        return pext_impl(src, mask);
    }

#else

    return pext_impl(src, mask);

#endif
}
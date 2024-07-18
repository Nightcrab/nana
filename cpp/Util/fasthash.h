
#ifndef _FASTHASH_H
#define _FASTHASH_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * fasthash32 - 32-bit implementation of fasthash
 * @buf:  data buffer
 * @len:  data size
 * @seed: the seed
 */
uint32_t fasthash32(const void* buf, size_t len, uint32_t seed);

/**
 * fasthash64 - 64-bit implementation of fasthash
 * @buf:  data buffer
 * @len:  data size
 * @seed: the seed
 */
uint64_t fasthash64(const void* buf, size_t len, uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif

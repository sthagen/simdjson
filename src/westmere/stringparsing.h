#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

#include "simdjson.h"

#ifdef IS_X86_64

#include "jsoncharutils.h"
#include "westmere/simd.h"
#include "westmere/intrinsics.h"
#include "westmere/bitmanipulation.h"

TARGET_WESTMERE
namespace simdjson::westmere {

using namespace simd;

// Holds backslashes and quotes locations.
struct parse_string_helper {
  uint32_t bs_bits;
  uint32_t quote_bits;
  static const uint32_t BYTES_PROCESSED = 32;
};

really_inline parse_string_helper find_bs_bits_and_quote_bits(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (parse_string_helper::BYTES_PROCESSED - 1));
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + 16);
  v0.store(dst);
  v1.store(dst + 16);
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    static_cast<uint32_t>(bs_and_quote),      // bs_bits
    static_cast<uint32_t>(bs_and_quote >> 32) // quote_bits
  };
}

#include "generic/stringparsing.h"

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64

#endif // SIMDJSON_WESTMERE_STRINGPARSING_H

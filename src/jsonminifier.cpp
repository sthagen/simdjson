#include "simdjson.h"
#include <cstdint>

#ifndef SIMDJSON_ISSUE384RESOLVED // to avoid tripping users

namespace simdjson {
static uint8_t jump_table[256 * 3] = {
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
};

size_t json_minify(const unsigned char *bytes, size_t how_many,
                   unsigned char *out) {
  size_t i = 0, pos = 0;
  uint8_t quote = 0;
  uint8_t nonescape = 1;

  while (i < how_many) {
    unsigned char c = bytes[i];
    uint8_t *meta = jump_table + 3 * c;

    quote = quote ^ (meta[0] & nonescape);
    out[pos] = c;
    pos += meta[2] | quote;

    i += 1;
    nonescape = (~nonescape) | (meta[1]);
  }
  return pos;
}
} // namespace simdjson
#else

//
// This fast code is disabled.
// See issue https://github.com/lemire/simdjson/issues/384
//
#include "simdprune_tables.h"
#include <cstring>
#include <x86intrin.h> // currently, there is no runtime dispatch for the minifier

namespace simdjson {

// a straightforward comparison of a mask against input.
static uint64_t cmp_mask_against_input_mini(__m256i input_lo, __m256i input_hi,
                                            __m256i mask) {
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

// Write up to 16 bytes, only the bytes corresponding to a 1-bit are written
// out. credit: Anime Tosho
static __m128i skinnycleanm128(__m128i x, int mask) {
  int mask1 = mask & 0xFF;
  int mask2 = (mask >> 8) & 0xFF;
  __m128i shufmask = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(_mm_loadl_epi64(
                       (const __m128i *)(thintable_epi8 + mask1))),
                   (const __m64 *)(thintable_epi8 + mask2)));
  shufmask =
      _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
  __m128i pruned = _mm_shuffle_epi8(x, shufmask);
  intptr_t popx2 = BitsSetTable256mul2[mask1];
  __m128i compactmask =
      _mm_loadu_si128((const __m128i *)(pshufb_combine_table + popx2 * 8));
  return _mm_shuffle_epi8(pruned, compactmask);
}

// take input from buf and remove useless whitespace, input and output can be
// the same, result is null terminated, return the string length (minus the null
// termination)
size_t json_minify(const uint8_t *buf, size_t len, uint8_t *out) {
  // Useful constant masks
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint8_t *initout(out);
  uint64_t prev_iter_ends_odd_backslash =
      0ULL;                               // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_inside_quote = 0ULL; // either all zeros or all ones
  size_t idx = 0;
  if (len >= 64) {
    size_t avx_len = len - 63;

    for (; idx < avx_len; idx += 64) {
      __m256i input_lo =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 0));
      __m256i input_hi =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 32));
      uint64_t bs_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                     _mm256_set1_epi8('\\'));
      uint64_t start_edges = bs_bits & ~(bs_bits << 1);
      uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
      uint64_t even_starts = start_edges & even_start_mask;
      uint64_t odd_starts = start_edges & ~even_start_mask;
      uint64_t even_carries = bs_bits + even_starts;
      uint64_t odd_carries;
      bool iter_ends_odd_backslash =
          add_overflow(bs_bits, odd_starts, &odd_carries);
      odd_carries |= prev_iter_ends_odd_backslash;
      prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
      uint64_t even_carry_ends = even_carries & ~bs_bits;
      uint64_t odd_carry_ends = odd_carries & ~bs_bits;
      uint64_t even_start_odd_end = even_carry_ends & odd_bits;
      uint64_t odd_start_even_end = odd_carry_ends & even_bits;
      uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
      uint64_t quote_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                        _mm256_set1_epi8('"'));
      quote_bits = quote_bits & ~odd_ends;
      uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
          _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
      quote_mask ^= prev_iter_inside_quote;
      prev_iter_inside_quote = static_cast<uint64_t>(
          static_cast<int64_t>(quote_mask) >>
          63); // might be undefined behavior, should be fully defined in C++20,
               // ok according to John Regher from Utah University
      const __m256i low_nibble_mask = _mm256_setr_epi8(
          //  0                           9  a   b  c  d
          16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 16, 0, 0, 0, 0, 0,
          0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
      const __m256i high_nibble_mask = _mm256_setr_epi8(
          //  0     2   3     5     7
          8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 8, 0, 18, 4, 0, 1, 0,
          1, 0, 0, 0, 3, 2, 1, 0, 0);
      __m256i whitespace_shufti_mask = _mm256_set1_epi8(0x18);
      __m256i v_lo = _mm256_and_si256(
          _mm256_shuffle_epi8(low_nibble_mask, input_lo),
          _mm256_shuffle_epi8(high_nibble_mask,
                              _mm256_and_si256(_mm256_srli_epi32(input_lo, 4),
                                               _mm256_set1_epi8(0x7f))));

      __m256i v_hi = _mm256_and_si256(
          _mm256_shuffle_epi8(low_nibble_mask, input_hi),
          _mm256_shuffle_epi8(high_nibble_mask,
                              _mm256_and_si256(_mm256_srli_epi32(input_hi, 4),
                                               _mm256_set1_epi8(0x7f))));
      __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
          _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
      __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
          _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

      uint64_t ws_res_0 =
          static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
      uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
      uint64_t whitespace = ~(ws_res_0 | (ws_res_1 << 32));
      whitespace &= ~quote_mask;

      uint64_t non_whitespace = ~whitespace;

      __m128i x1 = _mm256_extracti128_si256(input_lo, 0);
      __m128i x2 = _mm256_extracti128_si256(input_lo, 1);
      __m128i x3 = _mm256_extracti128_si256(input_hi, 0);
      __m128i x4 = _mm256_extracti128_si256(input_hi, 1);

      int mask1 = non_whitespace & 0xFFFF;
      int mask2 = (non_whitespace >> 16) & 0xFFFF;
      int mask3 = (non_whitespace >> 32) & 0xFFFF;
      int mask4 = (non_whitespace >> 48) & 0xFFFF;

      x1 = skinnycleanm128(x1, mask1);
      x2 = skinnycleanm128(x2, mask2);
      x3 = skinnycleanm128(x3, mask3);
      x4 = skinnycleanm128(x4, mask4);
      int pop1 = hamming(non_whitespace & 0xFFFF);
      int pop2 = hamming(non_whitespace & UINT64_C(0xFFFFFFFF));
      int pop3 = hamming(non_whitespace & UINT64_C(0xFFFFFFFFFFFF));
      int pop4 = hamming(non_whitespace);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out), x1);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop1), x2);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop2), x3);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop3), x4);
      out += pop4;
    }
  }
  // we finish off the job... copying and pasting the code is not ideal here,
  // but it gets the job done.
  if (idx < len) {
    uint8_t buffer[64];
    memset(buffer, 0, 64);
    memcpy(buffer, buf + idx, len - idx);
    __m256i input_lo =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer));
    __m256i input_hi =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer + 32));
    uint64_t bs_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('\\'));
    uint64_t start_edges = bs_bits & ~(bs_bits << 1);
    uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
    uint64_t even_starts = start_edges & even_start_mask;
    uint64_t odd_starts = start_edges & ~even_start_mask;
    uint64_t even_carries = bs_bits + even_starts;
    uint64_t odd_carries;
    // bool iter_ends_odd_backslash =
    add_overflow(bs_bits, odd_starts, &odd_carries);
    odd_carries |= prev_iter_ends_odd_backslash;
    // prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
    // // we never use it
    uint64_t even_carry_ends = even_carries & ~bs_bits;
    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
    uint64_t quote_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('"'));
    quote_bits = quote_bits & ~odd_ends;
    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
    quote_mask ^= prev_iter_inside_quote;
    // prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63);// we
    // don't need this anymore

    __m256i mask_20 = _mm256_set1_epi8(0x20); // c==32
    __m256i mask_70 =
        _mm256_set1_epi8(0x70); // adding 0x70 does not check low 4-bits
    // but moves any value >= 16 above 128

    __m256i lut_cntrl = _mm256_setr_epi8(
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00);

    __m256i tmp_ws_lo = _mm256_or_si256(
        _mm256_cmpeq_epi8(mask_20, input_lo),
        _mm256_shuffle_epi8(lut_cntrl, _mm256_adds_epu8(mask_70, input_lo)));
    __m256i tmp_ws_hi = _mm256_or_si256(
        _mm256_cmpeq_epi8(mask_20, input_hi),
        _mm256_shuffle_epi8(lut_cntrl, _mm256_adds_epu8(mask_70, input_hi)));
    uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
    uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
    uint64_t whitespace = (ws_res_0 | (ws_res_1 << 32));
    whitespace &= ~quote_mask;

    if (len - idx < 64) {
      whitespace |= UINT64_C(0xFFFFFFFFFFFFFFFF) << (len - idx);
    }
    uint64_t non_whitespace = ~whitespace;

    int mask1 = non_whitespace & 0xFFFF;
    int mask2 = (non_whitespace >> 16) & 0xFFFF;
    int mask3 = (non_whitespace >> 32) & 0xFFFF;
    int mask4 = (non_whitespace >> 48) & 0xFFFF;

    x1 = skinnycleanm128(x1, mask1);
    x2 = skinnycleanm128(x2, mask2);
    x3 = skinnycleanm128(x3, mask3);
    x4 = skinnycleanm128(x4, mask4);
    int pop1 = hamming(non_whitespace & 0xFFFF);
    int pop2 = hamming(non_whitespace & UINT64_C(0xFFFFFFFF));
    int pop3 = hamming(non_whitespace & UINT64_C(0xFFFFFFFFFFFF));
    int pop4 = hamming(non_whitespace);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out), x1);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop1), x2);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop2), x3);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop3), x4);
    out += pop4;
  }
  *out = '\0'; // NULL termination
  return out - initout;
}

size_t oldjson_minify(const uint8_t *buf, size_t len, uint8_t *out) {
  // Useful constant masks
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint8_t *initout(out);
  uint64_t prev_iter_ends_odd_backslash =
      0ULL;                               // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_inside_quote = 0ULL; // either all zeros or all ones
  size_t idx = 0;
  if (len >= 64) {
    size_t avx_len = len - 63;

    for (; idx < avx_len; idx += 64) {
      __m256i input_lo =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 0));
      __m256i input_hi =
          _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + idx + 32));
      uint64_t bs_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                     _mm256_set1_epi8('\\'));
      uint64_t start_edges = bs_bits & ~(bs_bits << 1);
      uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
      uint64_t even_starts = start_edges & even_start_mask;
      uint64_t odd_starts = start_edges & ~even_start_mask;
      uint64_t even_carries = bs_bits + even_starts;
      uint64_t odd_carries;
      bool iter_ends_odd_backslash =
          add_overflow(bs_bits, odd_starts, &odd_carries);
      odd_carries |= prev_iter_ends_odd_backslash;
      prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
      uint64_t even_carry_ends = even_carries & ~bs_bits;
      uint64_t odd_carry_ends = odd_carries & ~bs_bits;
      uint64_t even_start_odd_end = even_carry_ends & odd_bits;
      uint64_t odd_start_even_end = odd_carry_ends & even_bits;
      uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
      uint64_t quote_bits = cmp_mask_against_input_mini(input_lo, input_hi,
                                                        _mm256_set1_epi8('"'));
      quote_bits = quote_bits & ~odd_ends;
      uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
          _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
      quote_mask ^= prev_iter_inside_quote;
      prev_iter_inside_quote = static_cast<uint64_t>(
          static_cast<int64_t>(quote_mask) >>
          63); // might be undefined behavior, should be fully defined in C++20,
               // ok according to John Regher from Utah University
      const __m256i low_nibble_mask = _mm256_setr_epi8(
          //  0                           9  a   b  c  d
          16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0, 16, 0, 0, 0, 0, 0,
          0, 0, 0, 8, 12, 1, 2, 9, 0, 0);
      const __m256i high_nibble_mask = _mm256_setr_epi8(
          //  0     2   3     5     7
          8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0, 8, 0, 18, 4, 0, 1, 0,
          1, 0, 0, 0, 3, 2, 1, 0, 0);
      __m256i whitespace_shufti_mask = _mm256_set1_epi8(0x18);
      __m256i v_lo = _mm256_and_si256(
          _mm256_shuffle_epi8(low_nibble_mask, input_lo),
          _mm256_shuffle_epi8(high_nibble_mask,
                              _mm256_and_si256(_mm256_srli_epi32(input_lo, 4),
                                               _mm256_set1_epi8(0x7f))));

      __m256i v_hi = _mm256_and_si256(
          _mm256_shuffle_epi8(low_nibble_mask, input_hi),
          _mm256_shuffle_epi8(high_nibble_mask,
                              _mm256_and_si256(_mm256_srli_epi32(input_hi, 4),
                                               _mm256_set1_epi8(0x7f))));
      __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
          _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
      __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
          _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));

      uint64_t ws_res_0 =
          static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
      uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
      uint64_t whitespace = ~(ws_res_0 | (ws_res_1 << 32));
      whitespace &= ~quote_mask;
      int mask1 = whitespace & 0xFFFF;
      int mask2 = (whitespace >> 16) & 0xFFFF;
      int mask3 = (whitespace >> 32) & 0xFFFF;
      int mask4 = (whitespace >> 48) & 0xFFFF;
      int pop1 = hamming((~whitespace) & 0xFFFF);
      int pop2 = hamming((~whitespace) & UINT64_C(0xFFFFFFFF));
      int pop3 = hamming((~whitespace) & UINT64_C(0xFFFFFFFFFFFF));
      int pop4 = hamming((~whitespace));
      __m128i x1 = _mm256_extracti128_si256(input_lo, 0);
      __m128i x2 = _mm256_extracti128_si256(input_lo, 1);
      __m128i x3 = _mm256_extracti128_si256(input_hi, 0);
      __m128i x4 = _mm256_extracti128_si256(input_hi, 1);
      x1 = skinnycleanm128(x1, mask1);
      x2 = skinnycleanm128(x2, mask2);
      x3 = skinnycleanm128(x3, mask3);
      x4 = skinnycleanm128(x4, mask4);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out), x1);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop1), x2);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop2), x3);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(out + pop3), x4);
      out += pop4;
    }
  }
  // we finish off the job... copying and pasting the code is not ideal here,
  // but it gets the job done.
  if (idx < len) {
    uint8_t buffer[64];
    memset(buffer, 0, 64);
    memcpy(buffer, buf + idx, len - idx);
    __m256i input_lo =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer));
    __m256i input_hi =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buffer + 32));
    uint64_t bs_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('\\'));
    uint64_t start_edges = bs_bits & ~(bs_bits << 1);
    uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
    uint64_t even_starts = start_edges & even_start_mask;
    uint64_t odd_starts = start_edges & ~even_start_mask;
    uint64_t even_carries = bs_bits + even_starts;
    uint64_t odd_carries;
    // bool iter_ends_odd_backslash =
    add_overflow(bs_bits, odd_starts, &odd_carries);
    odd_carries |= prev_iter_ends_odd_backslash;
    // prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
    // // we never use it
    uint64_t even_carry_ends = even_carries & ~bs_bits;
    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
    uint64_t quote_bits =
        cmp_mask_against_input_mini(input_lo, input_hi, _mm256_set1_epi8('"'));
    quote_bits = quote_bits & ~odd_ends;
    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
    quote_mask ^= prev_iter_inside_quote;
    // prev_iter_inside_quote = (uint64_t)((int64_t)quote_mask >> 63);// we
    // don't need this anymore

    __m256i mask_20 = _mm256_set1_epi8(0x20); // c==32
    __m256i mask_70 =
        _mm256_set1_epi8(0x70); // adding 0x70 does not check low 4-bits
    // but moves any value >= 16 above 128

    __m256i lut_cntrl = _mm256_setr_epi8(
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00);

    __m256i tmp_ws_lo = _mm256_or_si256(
        _mm256_cmpeq_epi8(mask_20, input_lo),
        _mm256_shuffle_epi8(lut_cntrl, _mm256_adds_epu8(mask_70, input_lo)));
    __m256i tmp_ws_hi = _mm256_or_si256(
        _mm256_cmpeq_epi8(mask_20, input_hi),
        _mm256_shuffle_epi8(lut_cntrl, _mm256_adds_epu8(mask_70, input_hi)));
    uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo));
    uint64_t ws_res_1 = _mm256_movemask_epi8(tmp_ws_hi);
    uint64_t whitespace = (ws_res_0 | (ws_res_1 << 32));
    whitespace &= ~quote_mask;

    if (len - idx < 64) {
      whitespace |= UINT64_C(0xFFFFFFFFFFFFFFFF) << (len - idx);
    }
    int mask1 = whitespace & 0xFFFF;
    int mask2 = (whitespace >> 16) & 0xFFFF;
    int mask3 = (whitespace >> 32) & 0xFFFF;
    int mask4 = (whitespace >> 48) & 0xFFFF;
    int pop1 = hamming((~whitespace) & 0xFFFF);
    int pop2 = hamming((~whitespace) & UINT64_C(0xFFFFFFFF));
    int pop3 = hamming((~whitespace) & UINT64_C(0xFFFFFFFFFFFF));
    int pop4 = hamming((~whitespace));
    __m128i x1 = _mm256_extracti128_si256(input_lo, 0);
    __m128i x2 = _mm256_extracti128_si256(input_lo, 1);
    __m128i x3 = _mm256_extracti128_si256(input_hi, 0);
    __m128i x4 = _mm256_extracti128_si256(input_hi, 1);
    x1 = skinnycleanm128(x1, mask1);
    x2 = skinnycleanm128(x2, mask2);
    x3 = skinnycleanm128(x3, mask3);
    x4 = skinnycleanm128(x4, mask4);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer), x1);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer + pop1), x2);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer + pop2), x3);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer + pop3), x4);
    memcpy(out, buffer, pop4);
    out += pop4;
  }
  *out = '\0'; // NULL termination
  return out - initout;
}

} // namespace simdjson
#endif

namespace stage2 {
namespace numberparsing {

#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), false)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), writer.append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), writer.append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), writer.append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (false)
#define WRITE_INTEGER(VALUE, SRC, WRITER) writer.append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) writer.append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) writer.append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
really_inline double compute_float_64(int64_t power, uint64_t i, bool negative, bool *success) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    double d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    *success = true;
    return d;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    return 0.0;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  components c =
      power_of_ten_components[power - FASTFLOAT_SMALLEST_POWER];
      // safe because
      // power >= FASTFLOAT_SMALLEST_POWER
      // and power <= FASTFLOAT_LARGEST_POWER
  // we recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = c.mantissa;

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      *success = false;
      return 0;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
      // if mantissa & 1 == 1 we might need to round up.
      //
      // Scenarios:
      // 1. We are not in the middle. Then we should round up.
      //
      // 2. We are right in the middle. Whether we round up depends
      // on the last significant bit: if it is "one" then we round
      // up (round to even) otherwise, we do not.
      //
      // So if the last significant bit is 1, we can safely round up.
      // Hence we only need to bail out if (mantissa & 3) == 1.
      // Otherwise we may need more accuracy or analysis to determine whether
      // we are exactly between two floating-point numbers.
      // It can be triggered with 1e23.
      // Note: because the factor_mantissa and factor_mantissa_low are
      // almost always rounded down (except for small positive powers),
      // almost always should round up.
      *success = false;
      return 0;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = c.exp - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    *success = false;
    return 0;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  double d;
  memcpy(&d, &mantissa, sizeof(d));
  *success = true;
  return d;
}

static bool parse_float_strtod(const uint8_t *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod((const char *)ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 x
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == (const char *)ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
bool slow_float_parsing(UNUSED const uint8_t * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    WRITE_DOUBLE(d, src, writer);
    return true;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

really_inline bool parse_decimal(UNUSED const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return true;
}

really_inline bool parse_exponent(UNUSED const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  exponent += (neg_exp ? -exp_number : exp_number);

  // If there were no digits, it's an error.
  // If there were more than 18 digits, we may have overflowed the integer.
  if (unlikely(p == start_exp || p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -9,999,999,999,999,999,999 and bigger
    // than 9,999,999,999,999,999,999.
    if (p == start_exp || p > start_exp+18) { return INVALID_NUMBER(src); }
  }
  return true;
}

template<typename W>
really_inline bool write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, int digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  // digit count is off by 1 because of the decimal (assuming there was one).
  if (unlikely((digit_count-1 >= 19))) { // this is uncommon
    // It is possible that the integer had an overflow.
    // We have to handle the case where we have 0.0000somenumber.
    const uint8_t *start = start_digits;
    while ((*start == '0') || (*start == '.')) {
      start++;
    }
    // we over-decrement by one when there is a '.'
    digit_count -= int(start - start_digits);
    if (digit_count >= 19) {
      // Ok, chances are good that we had an overflow!
      // this is almost never going to get called!!!
      // we start anew, going slowly!!!
      // This will happen in the following examples:
      // 10000000000000000000000000000000000000000000e+308
      // 3.1415926535897932384626433832795028841971693993751
      //
      bool success = slow_float_parsing(src, writer);
      // The number was already written, but we made a copy of the writer
      // when we passed it to the parse_large_integer() function, so
      writer.skip_double();
      return success;
    }
  }
  // NOTE: it's weird that the unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (unlikely(exponent < FASTFLOAT_SMALLEST_POWER) || (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    bool success = slow_float_parsing(src, writer);
    // The number was already written, but we made a copy of the writer when we passed it to the
    // slow_float_parsing() function, so we have to skip those tape spots now that we've returned
    writer.skip_double();
    return success;
  }
  bool success = true;
  double d = compute_float_64(exponent, i, negative, &success);
  if (!success) {
    // we are almost never going to get here.
    if (!parse_float_strtod(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return true;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
really_inline bool parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return true;                 // always succeeds
}

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
really_inline bool parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    if (!parse_decimal(src, p, i, exponent)) { return false; }
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    if (!parse_exponent(src, p, exponent)) { return false; }
  }
  if (is_float) {
    return write_float(src, negative, i, start_digits, digit_count, exponent, writer);
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if(negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) {
        return INVALID_NUMBER(src); 
      }
      WRITE_INTEGER(~i+1, src, writer);
      return is_structural_or_whitespace(*p);
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  return is_structural_or_whitespace(*p);
}

#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // namespace stage2

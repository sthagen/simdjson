// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

namespace stage1 {

class json_minifier {
public:
  template<size_t STEP_SIZE>
  static error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept;

private:
  really_inline json_minifier(uint8_t *_dst)
  : dst{_dst}
  {}
  template<size_t STEP_SIZE>
  really_inline void step(const uint8_t *block_buf, buf_block_reader<STEP_SIZE> &reader) noexcept;
  really_inline void next(simd::simd8x64<uint8_t> in, json_block block);
  really_inline error_code finish(uint8_t *dst_start, size_t &dst_len);
  json_scanner scanner{};
  uint8_t *dst;
};

really_inline void json_minifier::next(simd::simd8x64<uint8_t> in, json_block block) {
  uint64_t mask = block.whitespace();
  in.compress(mask, dst);
  dst += 64 - count_ones(mask);
}

really_inline error_code json_minifier::finish(uint8_t *dst_start, size_t &dst_len) {
  *dst = '\0';
  error_code error = scanner.finish(false);
  if (error) { dst_len = 0; return error; }
  dst_len = dst - dst_start;
  return SUCCESS;
}

template<>
really_inline void json_minifier::step<128>(const uint8_t *block_buf, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  simd::simd8x64<uint8_t> in_2(block_buf+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1);
  this->next(in_2, block_2);
  reader.advance();
}

template<>
really_inline void json_minifier::step<64>(const uint8_t *block_buf, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block_buf);
  json_block block_1 = scanner.next(in_1);
  this->next(block_buf, block_1);
  reader.advance();
}

template<size_t STEP_SIZE>
error_code json_minifier::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_minifier minifier(dst);
  while (reader.has_full_block()) {
    minifier.step<STEP_SIZE>(reader.full_block(), reader);
  }

  if (likely(reader.has_remainder())) {
    uint8_t block[STEP_SIZE];
    reader.get_remainder(block);
    minifier.step<STEP_SIZE>(block, reader);
  }

  return minifier.finish(dst, dst_len);
}

} // namespace stage1

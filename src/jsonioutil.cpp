#include "simdjson.h"
#include <cstdlib>
#include <cstring>
#include <climits>

namespace simdjson {

padded_string get_corpus(const std::string &filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp != nullptr) {
    if(std::fseek(fp, 0, SEEK_END) < 0) {
      std::fclose(fp);
      throw std::runtime_error("cannot seek in the file");
    }
    long llen = std::ftell(fp);
    if((llen < 0) || (llen == LONG_MAX)) {
      std::fclose(fp);
      throw std::runtime_error("cannot tell where we are in the file");
    }
    size_t len = (size_t) llen;
    padded_string s(len);
    if (s.data() == nullptr) {
      std::fclose(fp);
      throw std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(s.data(), 1, len, fp);
    std::fclose(fp);
    if (readb != len) {
      throw std::runtime_error("could not read the data");
    }
    return s;
  }
  throw std::runtime_error("could not load corpus");
}
} // namespace simdjson

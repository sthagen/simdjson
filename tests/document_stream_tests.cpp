#include <string>
#include <vector>
#include <unistd.h>

#include "simdjson.h"
#include "test_macros.h"

namespace document_stream_tests {
  static simdjson::dom::document_stream parse_many_stream_return(simdjson::dom::parser &parser, simdjson::padded_string &str) {
    simdjson::dom::document_stream stream;
    UNUSED auto error = parser.parse_many(str).get(stream);
    return stream;
  }
  // this is a compilation test
  UNUSED static void parse_many_stream_assign() {
      simdjson::dom::parser parser;
      simdjson::padded_string str("{}",2);
      simdjson::dom::document_stream s1 = parse_many_stream_return(parser, str);
  }
  bool test_current_index() {
    std::cout << "Running " << __func__ << std::endl;
    std::string base("1         ");// one JSON!
    std::string json;
    for(size_t k =  0; k < 1000; k++) {
      json += base;
    }
    simdjson::dom::parser parser;
    const size_t window = 32; // deliberately small
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json,window).get(stream) );
    auto i = stream.begin();
    size_t count = 0;
    for(; i != stream.end(); ++i) {
      auto doc = *i;
      ASSERT_SUCCESS(doc);
      if( i.current_index() != count) {
        std::cout << "index:" << i.current_index() << std::endl;
        std::cout << "expected index:" << count << std::endl;
        return false;
      }
      count += base.size();
    }
    return true;
  }
  bool small_window() {
    std::cout << "Running " << __func__ << std::endl;
    auto json = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;
    simdjson::dom::parser parser;
    size_t count = 0;
    size_t window_size = 10; // deliberately too small
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json, window_size).get(stream) );
    for (auto doc : stream) {
      if (!doc.error()) {
          std::cerr << "Expected a capacity error " << doc.error() << std::endl;
          return false;
      }
      count++;
    }
    if(count == 2) {
      std::cerr << "Expected a capacity error " << std::endl;
      return false;
    }
    return true;
  }

  bool large_window() {
    std::cout << "Running " << __func__ << std::endl;
#if SIZE_MAX > 17179869184
    auto json = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;
    simdjson::dom::parser parser;
    size_t count = 0;
    uint64_t window_size{17179869184}; // deliberately too big
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json, size_t(window_size)).get(stream) );
    for (auto doc : stream) {
      if (!doc.error()) {
          std::cerr << "I expected a failure (too big) but got  " << doc.error() << std::endl;
          return false;
      }
      count++;
    }
#endif
    return true;
  }
  static bool parse_json_message_issue467(simdjson::padded_string &json, size_t expectedcount) {
    simdjson::dom::parser parser;
    size_t count = 0;
    simdjson::dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json).get(stream) );
    for (auto doc : stream) {
      if (doc.error()) {
          std::cerr << "Failed with simdjson error= " << doc.error() << std::endl;
          return false;
      }
      count++;
    }
    if(count != expectedcount) {
        std::cerr << "bad count" << std::endl;
        return false;
    }
    return true;
  }

  bool json_issue467() {
    std::cout << "Running " << __func__ << std::endl;
    auto single_message = R"({"error":[],"result":{"token":"xxx"}})"_padded;
    auto two_messages = R"({"error":[],"result":{"token":"xxx"}}{"error":[],"result":{"token":"xxx"}})"_padded;

    if(!parse_json_message_issue467(single_message, 1)) {
      return false;
    }
    if(!parse_json_message_issue467(two_messages, 2)) {
      return false;
    }
    return true;
  }

  // returns true if successful
  bool document_stream_test() {
    std::cout << "Running " << __func__ << std::endl;
    fflush(NULL);
    const size_t n_records = 10000;
    std::string data;
    char buf[1024];
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf,
                          sizeof(buf),
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"ete\": {\"id\": %zu, \"name\": \"eventail%zu\"}}",
                      i, i, (i % 2) ? "homme" : "femme", i % 10, i % 10);
      if (n >= sizeof(buf)) { abort(); }
      data += std::string(buf, n);
    }
    for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
      printf(".");
      fflush(NULL);
      simdjson::padded_string str(data);
      simdjson::dom::parser parser;
      size_t count = 0;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS( parser.parse_many(str, batch_size).get(stream) );
      for (auto doc : stream) {
        int64_t keyid;
        ASSERT_SUCCESS( doc["id"].get(keyid) );
        ASSERT_EQUAL( keyid, int64_t(count) );

        count++;
      }
      if(count != n_records) {
        printf("Found wrong number of documents %zd, expected %zd at batch size %zu\n", count, n_records, batch_size);
        return false;
      }
    }
    printf("ok\n");
    return true;
  }

  // returns true if successful
  bool document_stream_utf8_test() {
    std::cout << "Running " << __func__ << std::endl;
    fflush(NULL);
    const size_t n_records = 10000;
    std::string data;
    char buf[1024];
    for (size_t i = 0; i < n_records; ++i) {
      size_t n = snprintf(buf,
                        sizeof(buf),
                      "{\"id\": %zu, \"name\": \"name%zu\", \"gender\": \"%s\", "
                      "\"\xC3\xA9t\xC3\xA9\": {\"id\": %zu, \"name\": \"\xC3\xA9ventail%zu\"}}",
                      i, i, (i % 2) ? "\xE2\xBA\x83" : "\xE2\xBA\x95", i % 10, i % 10);
      if (n >= sizeof(buf)) { abort(); }
      data += std::string(buf, n);
    }
    for(size_t batch_size = 1000; batch_size < 2000; batch_size += (batch_size>1050?10:1)) {
      printf(".");
      fflush(NULL);
      simdjson::padded_string str(data);
      simdjson::dom::parser parser;
      size_t count = 0;
      simdjson::dom::document_stream stream;
      ASSERT_SUCCESS( parser.parse_many(str, batch_size).get(stream) );
      for (auto doc : stream) {
        int64_t keyid;
        ASSERT_SUCCESS( doc["id"].get(keyid) );
        ASSERT_EQUAL( keyid, int64_t(count) );

        count++;
      }
      ASSERT_EQUAL( count, n_records )
    }
    printf("ok\n");
    return true;
  }

  bool run() {
    return test_current_index() &&
           small_window() &&
           large_window() &&
           json_issue467() &&
           document_stream_test() &&
           document_stream_utf8_test();
  }
}



int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::available_implementations[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::active_implementation = impl;
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::active_implementation->name() == "unsupported") {
    printf("unsupported CPU\n");
  }
  // We want to know what we are testing.
  std::cout << "Running tests against this implementation: " << simdjson::active_implementation->name();
  std::cout << "(" << simdjson::active_implementation->description() << ")" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  std::cout << "Running document_stream tests." << std::endl;
  if (document_stream_tests::run()) {
    std::cout << "document_stream tests are ok." << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}

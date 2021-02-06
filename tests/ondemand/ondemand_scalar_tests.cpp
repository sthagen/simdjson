#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace scalar_tests {
  using namespace std;

  template<typename T>
  bool test_scalar_value(const padded_string &json, const T &expected, bool test_twice=true) {
    std::cout << "- JSON: " << json << endl;
    SUBTEST( "simdjson_result<document>", test_ondemand_doc(json, [&](auto doc_result) {
      T actual;
      ASSERT_SUCCESS( doc_result.get(actual) );
      ASSERT_EQUAL( expected, actual );
      // Test it twice (scalars can be retrieved more than once)
      if (test_twice) {
        ASSERT_SUCCESS( doc_result.get(actual) );
        ASSERT_EQUAL( expected, actual );
      }
      return true;
    }));
    SUBTEST( "document", test_ondemand_doc(json, [&](auto doc_result) {
      T actual;
      ASSERT_SUCCESS( doc_result.get(actual) );
      ASSERT_EQUAL( expected, actual );
      // Test it twice (scalars can be retrieved more than once)
      if (test_twice) {
        ASSERT_SUCCESS( doc_result.get(actual) );
        ASSERT_EQUAL( expected, actual );
      }
      return true;
    }));

    {
      padded_string whitespace_json = std::string(json) + " ";
      std::cout << "- JSON: " << whitespace_json << endl;
      SUBTEST( "simdjson_result<document>", test_ondemand_doc(whitespace_json, [&](auto doc_result) {
        T actual;
        ASSERT_SUCCESS( doc_result.get(actual) );
        ASSERT_EQUAL( expected, actual );
        // Test it twice (scalars can be retrieved more than once)
        if (test_twice) {
          ASSERT_SUCCESS( doc_result.get(actual) );
          ASSERT_EQUAL( expected, actual );
        }
        return true;
      }));
      SUBTEST( "document", test_ondemand_doc(whitespace_json, [&](auto doc_result) {
        T actual;
        ASSERT_SUCCESS( doc_result.get(actual) );
        ASSERT_EQUAL( expected, actual );
        // Test it twice (scalars can be retrieved more than once)
        if (test_twice) {
          ASSERT_SUCCESS( doc_result.get(actual) );
          ASSERT_EQUAL( expected, actual );
        }
        return true;
      }));
    }

    {
      padded_string array_json = std::string("[") + std::string(json) + "]";
      std::cout << "- JSON: " << array_json << endl;
      SUBTEST( "simdjson_result<value>", test_ondemand_doc(array_json, [&](auto doc_result) {
        int count = 0;
        for (simdjson_result<ondemand::value> val_result : doc_result) {
          T actual;
          ASSERT_SUCCESS( val_result.get(actual) );
          ASSERT_EQUAL(expected, actual);
          // Test it twice (scalars can be retrieved more than once)
          if (test_twice) {
            ASSERT_SUCCESS( val_result.get(actual) );
            ASSERT_EQUAL(expected, actual);
          }
          count++;
        }
        ASSERT_EQUAL(count, 1);
        return true;
      }));
      SUBTEST( "value", test_ondemand_doc(array_json, [&](auto doc_result) {
        int count = 0;
        for (simdjson_result<ondemand::value> val_result : doc_result) {
          ondemand::value val;
          ASSERT_SUCCESS( val_result.get(val) );
          T actual;
          ASSERT_SUCCESS( val.get(actual) );
          ASSERT_EQUAL(expected, actual);
          // Test it twice (scalars can be retrieved more than once)
          if (test_twice) {
            ASSERT_SUCCESS( val.get(actual) );
            ASSERT_EQUAL(expected, actual);
          }
          count++;
        }
        ASSERT_EQUAL(count, 1);
        return true;
      }));
    }

    {
      padded_string whitespace_array_json = std::string("[") + std::string(json) + " ]";
      std::cout << "- JSON: " << whitespace_array_json << endl;
      SUBTEST( "simdjson_result<value>", test_ondemand_doc(whitespace_array_json, [&](auto doc_result) {
        int count = 0;
        for (simdjson_result<ondemand::value> val_result : doc_result) {
          T actual;
          ASSERT_SUCCESS( val_result.get(actual) );
          ASSERT_EQUAL(expected, actual);
          // Test it twice (scalars can be retrieved more than once)
          if (test_twice) {
            ASSERT_SUCCESS( val_result.get(actual) );
            ASSERT_EQUAL(expected, actual);
          }
          count++;
        }
        ASSERT_EQUAL(count, 1);
        return true;
      }));
      SUBTEST( "value", test_ondemand_doc(whitespace_array_json, [&](auto doc_result) {
        int count = 0;
        for (simdjson_result<ondemand::value> val_result : doc_result) {
          ondemand::value val;
          ASSERT_SUCCESS( val_result.get(val) );
          T actual;
          ASSERT_SUCCESS( val.get(actual) );
          ASSERT_EQUAL(expected, actual);
          // Test it twice (scalars can be retrieved more than once)
          if (test_twice) {
            ASSERT_SUCCESS( val.get(actual) );
            ASSERT_EQUAL(expected, actual);
          }
          count++;
        }
        ASSERT_EQUAL(count, 1);
        return true;
      }));
    }

    TEST_SUCCEED();
  }

  bool string_value() {
    TEST_START();
    // We can't retrieve a small string twice because it will blow out the string buffer
    if (!test_scalar_value(R"("hi")"_padded, std::string_view("hi"), false)) { return false; }
    // ... unless the document is big enough to have a big string buffer :)
    if (!test_scalar_value(R"("hi"        )"_padded, std::string_view("hi"))) { return false; }
    TEST_SUCCEED();
  }

  bool numeric_values() {
    TEST_START();
    if (!test_scalar_value<int64_t> ("0"_padded,   0)) { return false; }
    if (!test_scalar_value<uint64_t>("0"_padded,   0)) { return false; }
    if (!test_scalar_value<double>  ("0"_padded,   0)) { return false; }
    if (!test_scalar_value<int64_t> ("1"_padded,   1)) { return false; }
    if (!test_scalar_value<uint64_t>("1"_padded,   1)) { return false; }
    if (!test_scalar_value<double>  ("1"_padded,   1)) { return false; }
    if (!test_scalar_value<int64_t> ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value<double>  ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value<double>  ("1.1"_padded, 1.1)) { return false; }
    TEST_SUCCEED();
  }

  bool boolean_values() {
    TEST_START();
    if (!test_scalar_value<bool> ("true"_padded,  true)) { return false; }
    if (!test_scalar_value<bool> ("false"_padded, false)) { return false; }
    TEST_SUCCEED();
  }

  bool null_value() {
    TEST_START();
    auto json = "null"_padded;
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc.is_null(), true );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.is_null(), true );
      return true;
    }));
    json = "[null]"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      int count = 0;
      for (auto value_result : doc_result) {
        ondemand::value value;
        ASSERT_SUCCESS( value_result.get(value) );
        ASSERT_EQUAL( value.is_null(), true );
        count++;
      }
      ASSERT_EQUAL( count, 1 );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      int count = 0;
      for (auto value_result : doc_result) {
        ASSERT_EQUAL( value_result.is_null(), true );
        count++;
      }
      ASSERT_EQUAL( count, 1 );
      return true;
    }));
    return true;
  }

#if SIMDJSON_EXCEPTIONS

  template<typename T>
  bool test_scalar_value_exception(const padded_string &json, const T &expected) {
    std::cout << "- JSON: " << json << endl;
    SUBTEST( "document", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( expected, T(doc_result) );
      return true;
    }));
    padded_string array_json = std::string("[") + std::string(json) + "]";
    std::cout << "- JSON: " << array_json << endl;
    SUBTEST( "value", test_ondemand_doc(array_json, [&](auto doc_result) {
      int count = 0;
      for (T actual : doc_result) {
        ASSERT_EQUAL( expected, actual );
        count++;
      }
      ASSERT_EQUAL(count, 1);
      return true;
    }));
    TEST_SUCCEED();
  }
  bool string_value_exception() {
    TEST_START();
    return test_scalar_value_exception(R"("hi")"_padded, std::string_view("hi"));
  }

  bool numeric_values_exception() {
    TEST_START();
    if (!test_scalar_value_exception<int64_t> ("0"_padded,   0)) { return false; }
    if (!test_scalar_value_exception<uint64_t>("0"_padded,   0)) { return false; }
    if (!test_scalar_value_exception<double>  ("0"_padded,   0)) { return false; }
    if (!test_scalar_value_exception<int64_t> ("1"_padded,   1)) { return false; }
    if (!test_scalar_value_exception<uint64_t>("1"_padded,   1)) { return false; }
    if (!test_scalar_value_exception<double>  ("1"_padded,   1)) { return false; }
    if (!test_scalar_value_exception<int64_t> ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value_exception<double>  ("-1"_padded,  -1)) { return false; }
    if (!test_scalar_value_exception<double>  ("1.1"_padded, 1.1)) { return false; }
    TEST_SUCCEED();
  }

  bool boolean_values_exception() {
    TEST_START();
    if (!test_scalar_value_exception<bool> ("true"_padded,  true)) { return false; }
    if (!test_scalar_value_exception<bool> ("false"_padded, false)) { return false; }
    TEST_SUCCEED();
  }

#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
           string_value() &&
           numeric_values() &&
           boolean_values() &&
           null_value() &&
#if SIMDJSON_EXCEPTIONS
           string_value_exception() &&
           numeric_values_exception() &&
           boolean_values_exception() &&
#endif // SIMDJSON_EXCEPTIONS
           true;
  }

} // namespace scalar_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, scalar_tests::run);
}

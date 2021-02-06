#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace object_tests {
  using namespace std;

  bool iterate_object() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c": 3 })"_padded;
    const char* expected_key[] = { "a", "b", "c" };
    const uint64_t expected_value[] = { 1, 2, 3 };
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto [ field, error ] : object) {
        ASSERT_SUCCESS(error);
        ASSERT_EQUAL( field.key(), expected_key[i]);
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object_result = doc_result.get_object();
      size_t i = 0;
      for (auto [ field, error ] : object_result) {
        ASSERT_SUCCESS(error);
        ASSERT_EQUAL( field.key(), expected_key[i] );
        ASSERT_EQUAL( field.value().get_uint64().value_unsafe(), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_object_partial_children() {
    TEST_START();
    auto json = R"(
      {
        "scalar_ignore":       0,
        "empty_array_ignore":  [],
        "empty_object_ignore": {},
        "object_break":        { "x": 3, "y": 33 },
        "object_break_unused": { "x": 4, "y": 44 },
        "object_index":        { "x": 5, "y": 55 },
        "object_index_unused": { "x": 6, "y": 66 },
        "array_break":         [ 7, 77, 777 ],
        "array_break_unused":  [ 8, 88, 888 ],
        "quadruple_nested_break": { "a": [ { "b": [ 9, 99 ], "c": 999 }, 9999 ], "d": 99999 },
        "actual_value": 10
      }
    )"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      size_t i = 0;
      for (auto field : object) {
        ondemand::raw_json_string key;
        ASSERT_SUCCESS( field.key().get(key) );

        switch (i) {
          case 0: {
            ASSERT_EQUAL(key, "scalar_ignore");
            std::cout << "  - After ignoring empty scalar ..." << std::endl;
            break;
          }
          case 1: {
            ASSERT_EQUAL(key, "empty_array_ignore");
            std::cout << "  - After ignoring empty array ..." << std::endl;
            break;
          }
          case 2: {
            ASSERT_EQUAL(key, "empty_object_ignore");
            std::cout << "  - After ignoring empty object ..." << std::endl;
            break;
          }
          // Break after using first value in child object
          case 3: {
            ASSERT_EQUAL(key, "object_break");

            for (auto [ child_field, error ] : field.value().get_object()) {
              ASSERT_SUCCESS(error);
              ASSERT_EQUAL(child_field.key(), "x");
              uint64_t x;
              ASSERT_SUCCESS( child_field.value().get(x) );
              ASSERT_EQUAL(x, 3);
              break; // Break after the first value
            }
            std::cout << "  - After using first value in child object ..." << std::endl;
            break;
          }

          // Break without using first value in child object
          case 4: {
            ASSERT_EQUAL(key, "object_break_unused");

            for (auto [ child_field, error ] : field.value().get_object()) {
              ASSERT_SUCCESS(error);
              ASSERT_EQUAL(child_field.key(), "x");
              break;
            }
            std::cout << "  - After reaching (but not using) first value in child object ..." << std::endl;
            break;
          }

          // Only look up one field in child object
          case 5: {
            ASSERT_EQUAL(key, "object_index");

            uint64_t x;
            ASSERT_SUCCESS( field.value()["x"].get(x) );
            ASSERT_EQUAL( x, 5 );
            std::cout << "  - After looking up one field in child object ..." << std::endl;
            break;
          }

          // Only look up one field in child object, but don't use it
          case 6: {
            ASSERT_EQUAL(key, "object_index_unused");

            ASSERT_SUCCESS( field.value()["x"] );
            std::cout << "  - After looking up (but not using) one field in child object ..." << std::endl;
            break;
          }

          // Break after first value in child array
          case 7: {
            ASSERT_EQUAL(key, "array_break");
            for (auto child_value : field.value()) {
              uint64_t x;
              ASSERT_SUCCESS( child_value.get(x) );
              ASSERT_EQUAL( x, 7 );
              break;
            }
            std::cout << "  - After using first value in child array ..." << std::endl;
            break;
          }

          // Break without using first value in child array
          case 8: {
            ASSERT_EQUAL(key, "array_break_unused");
            for (auto child_value : field.value()) {
              ASSERT_SUCCESS(child_value);
              break;
            }
            std::cout << "  - After reaching (but not using) first value in child array ..." << std::endl;
            break;
          }

          // Break out of multiple child loops
          case 9: {
            ASSERT_EQUAL(key, "quadruple_nested_break");
            for (auto child1 : field.value().get_object()) {
              for (auto child2 : child1.value().get_array()) {
                for (auto child3 : child2.get_object()) {
                  for (auto child4 : child3.value().get_array()) {
                    uint64_t x;
                    ASSERT_SUCCESS( child4.get(x) );
                    ASSERT_EQUAL( x, 9 );
                    break;
                  }
                  break;
                }
                break;
              }
              break;
            }
            std::cout << "  - After breaking out of quadruply-nested arrays and objects ..." << std::endl;
            break;
          }

          // Test the actual value
          case 10: {
            ASSERT_EQUAL(key, "actual_value");
            uint64_t actual_value;
            ASSERT_SUCCESS( field.value().get(actual_value) );
            ASSERT_EQUAL( actual_value, 10 );
            break;
          }
        }

        i++;
      }
      ASSERT_EQUAL( i, 11 ); // Make sure we found all the keys we expected
      return true;
    }));
    return true;
  }

  bool object_index_partial_children() {
    TEST_START();
    auto json = R"(
      {
        "scalar_ignore":       0,
        "empty_array_ignore":  [],
        "empty_object_ignore": {},
        "object_break":        { "x": 3, "y": 33 },
        "object_break_unused": { "x": 4, "y": 44 },
        "object_index":        { "x": 5, "y": 55 },
        "object_index_unused": { "x": 6, "y": 66 },
        "array_break":         [ 7, 77, 777 ],
        "array_break_unused":  [ 8, 88, 888 ],
        "quadruple_nested_break": { "a": [ { "b": [ 9, 99 ], "c": 999 }, 9999 ], "d": 99999 },
        "actual_value": 10
      }
    )"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_SUCCESS( object["scalar_ignore"] );
      std::cout << "  - After ignoring empty scalar ..." << std::endl;

      ASSERT_SUCCESS( object["empty_array_ignore"] );
      std::cout << "  - After ignoring empty array ..." << std::endl;

      ASSERT_SUCCESS( object["empty_object_ignore"] );
      std::cout << "  - After ignoring empty object ..." << std::endl;

      // Break after using first value in child object
      {
        auto value = object["object_break"];
        for (auto [ child_field, error ] : value.get_object()) {
          ASSERT_SUCCESS(error);
          ASSERT_EQUAL(child_field.key(), "x");
          uint64_t x;
          ASSERT_SUCCESS( child_field.value().get(x) );
          ASSERT_EQUAL(x, 3);
          break; // Break after the first value
        }
        std::cout << "  - After using first value in child object ..." << std::endl;
      }

      // Break without using first value in child object
      {
        auto value = object["object_break_unused"];
        for (auto [ child_field, error ] : value.get_object()) {
          ASSERT_SUCCESS(error);
          ASSERT_EQUAL(child_field.key(), "x");
          break;
        }
        std::cout << "  - After reaching (but not using) first value in child object ..." << std::endl;
      }

      // Only look up one field in child object
      {
        auto value = object["object_index"];

        uint64_t x;
        ASSERT_SUCCESS( value["x"].get(x) );
        ASSERT_EQUAL( x, 5 );
        std::cout << "  - After looking up one field in child object ..." << std::endl;
      }

      // Only look up one field in child object, but don't use it
      {
        auto value = object["object_index_unused"];

        ASSERT_SUCCESS( value["x"] );
        std::cout << "  - After looking up (but not using) one field in child object ..." << std::endl;
      }

      // Break after first value in child array
      {
        auto value = object["array_break"];

        for (auto child_value : value) {
          uint64_t x;
          ASSERT_SUCCESS( child_value.get(x) );
          ASSERT_EQUAL( x, 7 );
          break;
        }
        std::cout << "  - After using first value in child array ..." << std::endl;
      }

      // Break without using first value in child array
      {
        auto value = object["array_break_unused"];

        for (auto child_value : value) {
          ASSERT_SUCCESS(child_value);
          break;
        }
        std::cout << "  - After reaching (but not using) first value in child array ..." << std::endl;
      }

      // Break out of multiple child loops
      {
        auto value = object["quadruple_nested_break"];
        for (auto child1 : value.get_object()) {
          for (auto child2 : child1.value().get_array()) {
            for (auto child3 : child2.get_object()) {
              for (auto child4 : child3.value().get_array()) {
                uint64_t x;
                ASSERT_SUCCESS( child4.get(x) );
                ASSERT_EQUAL( x, 9 );
                break;
              }
              break;
            }
            break;
          }
          break;
        }
        std::cout << "  - After breaking out of quadruply-nested arrays and objects ..." << std::endl;
      }

      // Test the actual value
      {
        auto value = object["actual_value"];
        uint64_t actual_value;
        ASSERT_SUCCESS( value.get(actual_value) );
        ASSERT_EQUAL( actual_value, 10 );
      }

      return true;
    }));

    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_SUCCESS( doc_result["scalar_ignore"] );
      std::cout << "  - After ignoring empty scalar ..." << std::endl;

      ASSERT_SUCCESS( doc_result["empty_array_ignore"] );
      std::cout << "  - After ignoring empty array ..." << std::endl;

      ASSERT_SUCCESS( doc_result["empty_object_ignore"] );
      std::cout << "  - After ignoring empty doc_result ..." << std::endl;

      // Break after using first value in child object
      {
        auto value = doc_result["object_break"];
        for (auto [ child_field, error ] : value.get_object()) {
          ASSERT_SUCCESS(error);
          ASSERT_EQUAL(child_field.key(), "x");
          uint64_t x;
          ASSERT_SUCCESS( child_field.value().get(x) );
          ASSERT_EQUAL(x, 3);
          break; // Break after the first value
        }
        std::cout << "  - After using first value in child object ..." << std::endl;
      }

      // Break without using first value in child object
      {
        auto value = doc_result["object_break_unused"];
        for (auto [ child_field, error ] : value.get_object()) {
          ASSERT_SUCCESS(error);
          ASSERT_EQUAL(child_field.key(), "x");
          break;
        }
        std::cout << "  - After reaching (but not using) first value in child object ..." << std::endl;
      }

      // Only look up one field in child object
      {
        auto value = doc_result["object_index"];

        uint64_t x;
        ASSERT_SUCCESS( value["x"].get(x) );
        ASSERT_EQUAL( x, 5 );
        std::cout << "  - After looking up one field in child object ..." << std::endl;
      }

      // Only look up one field in child object, but don't use it
      {
        auto value = doc_result["object_index_unused"];

        ASSERT_SUCCESS( value["x"] );
        std::cout << "  - After looking up (but not using) one field in child object ..." << std::endl;
      }

      // Break after first value in child array
      {
        auto value = doc_result["array_break"];

        for (auto child_value : value) {
          uint64_t x;
          ASSERT_SUCCESS( child_value.get(x) );
          ASSERT_EQUAL( x, 7 );
          break;
        }
        std::cout << "  - After using first value in child array ..." << std::endl;
      }

      // Break without using first value in child array
      {
        auto value = doc_result["array_break_unused"];

        for (auto child_value : value) {
          ASSERT_SUCCESS(child_value);
          break;
        }
        std::cout << "  - After reaching (but not using) first value in child array ..." << std::endl;
      }

      // Break out of multiple child loops
      {
        auto value = doc_result["quadruple_nested_break"];
        for (auto child1 : value.get_object()) {
          for (auto child2 : child1.value().get_array()) {
            for (auto child3 : child2.get_object()) {
              for (auto child4 : child3.value().get_array()) {
                uint64_t x;
                ASSERT_SUCCESS( child4.get(x) );
                ASSERT_EQUAL( x, 9 );
                break;
              }
              break;
            }
            break;
          }
          break;
        }
        std::cout << "  - After breaking out of quadruply-nested arrays and objects ..." << std::endl;
      }

      // Test the actual value
      {
        auto value = doc_result["actual_value"];
        uint64_t actual_value;
        ASSERT_SUCCESS( value.get(actual_value) );
        ASSERT_EQUAL( actual_value, 10 );
      }

      return true;
    }));

    return true;
  }

  bool iterate_empty_object() {
    TEST_START();
    auto json = R"({})"_padded;

    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      for (simdjson_unused auto field : object) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object_result = doc_result.get_object();
      for (simdjson_unused auto field : object_result) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));
    TEST_SUCCEED();
  }

  bool object_index() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object["b"].get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object["c/d"].get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object["d"], NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object;
      object = doc_result.get_object();

      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object["b"].get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object["c/d"].get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object["d"], NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc["b"].get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc["c/d"].get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( doc["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( doc["d"], NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc_result["b"].get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc_result["c/d"].get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( doc_result["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( doc_result["d"], NO_SUCH_FIELD );
      return true;
    }));

    json = R"({ "outer": { "a": 1, "b": 2, "c/d": 3 } })"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::value object;
      ASSERT_SUCCESS( doc_result["outer"].get(object) );
      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object["b"].get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object["c/d"].get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object["d"], NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::value> object = doc_result["outer"];
      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object["b"].get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object["c/d"].get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object["a"].get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object["d"], NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool object_find_field_unordered() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object;
      object = doc_result.get_object();

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( doc.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( doc.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc_result.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc_result.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( doc_result.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( doc_result.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));

    json = R"({ "outer": { "a": 1, "b": 2, "c/d": 3 } })"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::value object;
      ASSERT_SUCCESS( doc_result.find_field_unordered("outer").get(object) );
      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::value> object = doc_result.find_field_unordered("outer");
      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool object_find_field() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object;
      object = doc_result.get_object();

      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( doc.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( doc.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc_result.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc_result.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( doc_result.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( doc_result.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));

    json = R"({ "outer": { "a": 1, "b": 2, "c/d": 3 } })"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::value object;
      ASSERT_SUCCESS( doc_result.find_field("outer").get(object) );
      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::value> object = doc_result.find_field("outer");
      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool nested_object_index() {
    TEST_START();
    auto json = R"({ "x": { "y": { "z": 2 } } }})"_padded;
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result["x"]["y"]["z"].get_uint64().value_unsafe(), 2 );
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc["x"]["y"]["z"].get_uint64().value_unsafe(), 2 );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object = doc_result.get_object();
      ASSERT_EQUAL( object["x"]["y"]["z"].get_uint64().value_unsafe(), 2 );
      return true;
    }));
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      ASSERT_EQUAL( object["x"]["y"]["z"].get_uint64().value_unsafe(), 2 );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::value> x = doc_result["x"];
      ASSERT_EQUAL( x["y"]["z"].get_uint64().value_unsafe(), 2 );
      return true;
    }));
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::value x;
      ASSERT_SUCCESS( doc_result["x"].get(x) );
      ASSERT_EQUAL( x["y"]["z"].get_uint64().value_unsafe(), 2 );
      return true;
    }));
    TEST_SUCCEED();
  }

#if SIMDJSON_EXCEPTIONS

  bool iterate_object_exception() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c": 3 })"_padded;
    const char* expected_key[] = { "a", "b", "c" };
    const uint64_t expected_value[] = { 1, 2, 3 };
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      size_t i = 0;
      for (ondemand::field field : doc_result.get_object()) {
        ASSERT_EQUAL( field.key(), expected_key[i] );
        ASSERT_EQUAL( uint64_t(field.value()), expected_value[i] );
        i++;
      }
      ASSERT_EQUAL( i*sizeof(uint64_t), sizeof(expected_value) );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_empty_object_exception() {
    TEST_START();
    auto json = R"({})"_padded;

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused ondemand::field field : doc_result.get_object()) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));

    TEST_SUCCEED();
  }

  bool object_index_exception() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object = doc_result;

      ASSERT_EQUAL( uint64_t(object["a"]), 1 );
      ASSERT_EQUAL( uint64_t(object["b"]), 2 );
      ASSERT_EQUAL( uint64_t(object["c/d"]), 3 );

      return true;
    }));
    TEST_SUCCEED();
  }
  bool nested_object_index_exception() {
    TEST_START();
    auto json = R"({ "x": { "y": { "z": 2 } } }})"_padded;
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( uint64_t(doc_result["x"]["y"]["z"]), 2 );
      return true;
    }));
    TEST_SUCCEED();
  }

#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
           iterate_object() &&
           iterate_empty_object() &&
           object_index() &&
           object_find_field_unordered() &&
           object_find_field() &&
           nested_object_index() &&
           iterate_object_partial_children() &&
           object_index_partial_children() &&
#if SIMDJSON_EXCEPTIONS
           iterate_object_exception() &&
           object_index_exception() &&
           nested_object_index_exception() &&
#endif // SIMDJSON_EXCEPTIONS
           true;
  }

} // namespace object_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, object_tests::run);
}

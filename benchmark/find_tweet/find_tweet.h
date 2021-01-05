
#pragma once

#include "json_benchmark/file_runner.h"

namespace find_tweet {

template<typename I>
struct runner : public json_benchmark::file_runner<I> {
  std::string_view text;

  bool setup(benchmark::State &state) {
    return this->load_json(state, json_benchmark::TWITTER_JSON);
  }

  bool before_run(benchmark::State &state) {
    text = "";
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, 505874901689851900ULL, text);
  }

  template<typename R>
  bool diff(benchmark::State &state, runner<R> &reference) {
    return diff_results(state, text, reference.text);
  }
};

struct simdjson_dom;

template<typename I> simdjson_really_inline static void find_tweet(benchmark::State &state) {
  json_benchmark::run_json_benchmark<runner<I>, runner<simdjson_dom>>(state);
}

} // namespace find_tweet

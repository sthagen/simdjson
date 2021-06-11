namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= iter->depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the array is first found and the iterator is just past the `{`.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the `,` before the next value (or `]`). In this state,
//   depth == iter->depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > iter->depth, at_start == false, and error == SUCCESS.
//
// ## Error States
//
// In error states, we will yield exactly one more value before stopping. iter->depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the array iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet element may be missing or not be an
//   array--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter->depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between elements,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter->depth == depth, and at_start == false.
//
// ## Terminal State
//
// The terminal state has iter->depth < depth. at_start is always false.
//
// - Finished: When we have reached a `]` or have reported an error, we are finished. We signal this
//   by decrementing depth. In this state, iter->depth < depth, at_start == false, and
//   error == SUCCESS.
//

simdjson_really_inline array::array(const value_iterator &_iter) noexcept
  : iter{_iter}
{
}

simdjson_really_inline simdjson_result<array> array::start(value_iterator &iter) noexcept {
  // We don't need to know if the array is empty to start iteration, but we do want to know if there
  // is an error--thus `simdjson_unused`.
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_array().get(has_value) );
  return array(iter);
}
simdjson_really_inline simdjson_result<array> array::start_root(value_iterator &iter) noexcept {
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_root_array().get(has_value) );
  return array(iter);
}
simdjson_really_inline array array::started(value_iterator &iter) noexcept {
  simdjson_unused bool has_value = iter.started_array();
  return array(iter);
}

simdjson_really_inline simdjson_result<array_iterator> array::begin() noexcept {
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  if (!iter.is_at_iterator_start()) { return OUT_OF_ORDER_ITERATION; }
#endif
  return array_iterator(iter);
}
simdjson_really_inline simdjson_result<array_iterator> array::end() noexcept {
  return array_iterator(iter);
}

simdjson_really_inline simdjson_result<size_t> array::count_elements() & noexcept {
  // The count_elements method should always be called before you have begun
  // iterating through the array.
  // To open a new array you need to be at a `[`.
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  // array::begin() makes the same check.
  if(!iter.is_at_container_start()) { return OUT_OF_ORDER_ITERATION; }
#endif
  // Otherwise, we need to iterate through the array.
  iter.enter_at_container_start(); // sets the depth to indicate that we are inside the container and accesses the first element
  size_t count{0};
  // Important: we do not consume any of the values.
  for(simdjson_unused auto v : *this) { count++; }
  // The above loop will always succeed, but we want to report errors.
  if(iter.error()) { return iter.error(); }
  // We need to move back at the start because we expect users to iterator through
  // the array after counting the number of elements.
  iter.enter_at_container_start(); // sets the depth to indicate that we are inside the container and accesses the first element
  return count;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_IMPLEMENTATION::ondemand::array &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(
      std::forward<SIMDJSON_IMPLEMENTATION::ondemand::array>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::simdjson_result(
  error_code error
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::array>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::begin() noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::end() noexcept {
  if (error()) { return error(); }
  return first.end();
}
simdjson_really_inline  simdjson_result<size_t> simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array>::count_elements() & noexcept {
  if (error()) { return error(); }
  return first.count_elements();
}
} // namespace simdjson

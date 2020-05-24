#include "simdjson.h"

#include "fallback/implementation.h"
#include "fallback/stringparsing.h"
#include "fallback/numberparsing.h"

namespace simdjson {
namespace fallback {

#include "generic/stage2/logger.h"
#include "generic/stage2/atomparsing.h"
#include "generic/stage2/structural_iterator.h"
#include "generic/stage2/structural_parser.h"
#include "generic/stage2/streaming_structural_parser.h"

} // namespace fallback
} // namespace simdjson

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "query_term_simple.h"
#include <string>

namespace search {

/**
 * Class used to decode a single term.
 */
struct QueryTermDecoder {
    using QueryPacketT = std::string_view;

    static QueryTermSimple::UP decodeTerm(QueryPacketT term);
};

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "match_params.h"
#include <algorithm>
#include <cmath>

namespace proton::matching {

namespace {

uint32_t
computeArraySize(uint32_t hitsPlussOffset, uint32_t heapSize, uint32_t arraySize)
{
    return std::max(hitsPlussOffset, std::max(heapSize, arraySize));
}

}

MatchParams::MatchParams(uint32_t          numDocs_in,
                         uint32_t          heapSize_in,
                         uint32_t          arraySize_in,
                         std::optional<search::feature_t> first_phase_rank_score_drop_limit_in,
                         std::optional<search::feature_t> second_phase_rank_score_drop_limit_in,
                         uint32_t          offset_in,
                         uint32_t          hits_in,
                         bool              hasFinalRank,
                         bool              needRanking)
    : numDocs(numDocs_in),
      heapSize((hasFinalRank && needRanking) ? std::min(numDocs_in, heapSize_in) : 0),
      arraySize((needRanking && ((heapSize_in + arraySize_in) > 0))
                ? std::min(numDocs_in, computeArraySize(hits_in + offset_in, heapSize, arraySize_in))
                : 0),
      offset(std::min(numDocs_in, offset_in)),
      hits(std::min(numDocs_in - offset, hits_in)),
      diversity_want_hits(heapSize_in),
      first_phase_rank_score_drop_limit(first_phase_rank_score_drop_limit_in),
      second_phase_rank_score_drop_limit(second_phase_rank_score_drop_limit_in)
{ }

}

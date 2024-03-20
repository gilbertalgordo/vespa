// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "multi_term_hash_filter.h"
#include <vespa/searchlib/common/bitvector.h>
#include <vespa/searchlib/fef/termfieldmatchdata.h>

namespace search::attribute {

template <typename WrapperType>
MultiTermHashFilter<WrapperType>::MultiTermHashFilter(fef::TermFieldMatchData& tfmd,
                                                      WrapperType attr,
                                                      TokenMap&& map)
    : _tfmd(tfmd),
      _attr(attr),
      _map(std::move(map)),
      _weight(0)
{
}

template <typename WrapperType>
void
MultiTermHashFilter<WrapperType>::and_hits_into(BitVector& result, uint32_t begin_id)
{
    auto end = _map.end();
    result.foreach_truebit([&, end](uint32_t key) { if ( _map.find(_attr.getToken(key)) == end) { result.clearBit(key); }}, begin_id);
}

template <typename WrapperType>
void
MultiTermHashFilter<WrapperType>::doSeek(uint32_t docId)
{
    auto pos = _map.find(_attr.getToken(docId));
    if (pos != _map.end()) {
        _weight = pos->second;
        setDocId(docId);
    }
}

template <typename WrapperType>
void
MultiTermHashFilter<WrapperType>::doUnpack(uint32_t docId)
{
    if constexpr (unpack_weights) {
        _tfmd.reset(docId);
        fef::TermFieldMatchDataPosition pos;
        pos.setElementWeight(_weight);
        _tfmd.appendPosition(pos);
    } else {
        _tfmd.resetOnlyDocId(docId);
    }
}
    
}

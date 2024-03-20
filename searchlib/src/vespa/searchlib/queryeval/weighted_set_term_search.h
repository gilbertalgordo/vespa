// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "multisearch.h"
#include <vespa/vespalib/util/priority_queue.h>
#include <vespa/searchlib/fef/matchdata.h>
#include <vespa/searchlib/fef/termfieldmatchdataarray.h>
#include <vespa/searchlib/attribute/posting_iterator_pack.h>
#include <memory>
#include <variant>
#include <vector>

namespace search::attribute { class IAttributeVector; }

namespace search::fef { class TermFieldMatchData; }

namespace search::queryeval {

class Blueprint;

/**
 * Search iterator for a WeightedSetTerm, based on a set of child search iterators.
 */
class WeightedSetTermSearch : public SearchIterator
{
protected:
    WeightedSetTermSearch() = default;

public:
    // Whether this iterator is considered a filter, independent of attribute vector settings (ref rank: filter).
    static constexpr bool filter_search = false;
    // Whether this iterator requires btree iterators for all tokens/terms used by the operator.
    static constexpr bool require_btree_iterators = false;
    // Whether this supports creating a hash filter iterator;
    static constexpr bool supports_hash_filter = true;

    // TODO: pass ownership with unique_ptr
    static SearchIterator::UP create(const std::vector<SearchIterator *> &children,
                                     search::fef::TermFieldMatchData &tmd,
                                     bool is_filter_search,
                                     const std::vector<int32_t> &weights,
                                     fef::MatchData::UP match_data);

    static SearchIterator::UP create(search::fef::TermFieldMatchData& tmd,
                                     bool is_filter_search,
                                     std::variant<std::reference_wrapper<const std::vector<int32_t>>, std::vector<int32_t>> weights,
                                     std::vector<DocidIterator>&& iterators);

    static SearchIterator::UP create(search::fef::TermFieldMatchData &tmd,
                                     bool is_filter_search,
                                     std::variant<std::reference_wrapper<const std::vector<int32_t>>, std::vector<int32_t>> weights,
                                     std::vector<DocidWithWeightIterator> &&iterators);

    static SearchIterator::UP create_hash_filter(search::fef::TermFieldMatchData& tmd,
                                                 bool is_filter_search,
                                                 const std::vector<int32_t>& weights,
                                                 const std::vector<IDirectPostingStore::LookupResult>& terms,
                                                 const attribute::IAttributeVector& attr,
                                                 const IDirectPostingStore& posting_store,
                                                 vespalib::datastore::EntryRef dictionary_snapshot);

    // used during docsum fetching to identify matching elements
    // initRange must be called before use.
    // doSeek/doUnpack must not be called.
    virtual void find_matching_elements(uint32_t docid, const std::vector<std::unique_ptr<Blueprint>> &child_blueprints, std::vector<uint32_t> &dst) = 0;
};

}

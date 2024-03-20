// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchlib/aggregation/grouping.h>
#include <vespa/vespalib/objects/nbostream.h>
#include <vespa/vespalib/util/time.h>
#include <vector>
#include <atomic>

namespace search::grouping {

/**
 * A Grouping Context contains all grouping expressions that should be evaluated
 * for a particular pass, together with the ability to serialize and deserialize
 * the data from/to a byte buffer.
 **/
class GroupingContext
{
public:
    using UP = std::unique_ptr<GroupingContext>;
    using Grouping = search::aggregation::Grouping;
    using GroupingList = std::vector<std::shared_ptr<Grouping>>;
    using steady_time = vespalib::steady_time;

    /**
     * Deserialize a grouping spec into this context.
     *
     * @param groupSpec The grouping specification to use for initialization.
     * @param groupSpecLen The length of the grouping specification, in bytes.
     **/
    void deserialize(const char *groupSpec, uint32_t groupSpecLen);

    /**
     * Create a new grouping context from a byte buffer.
     * @param groupSpec The grouping specification to use for initialization.
     * @param groupSpecLen The length of the grouping specification, in bytes.
     **/
    GroupingContext(const BitVector & validLids, const std::atomic<steady_time> & now_ref, steady_time timeOfDoom,
                    const char *groupSpec, uint32_t groupSpecLen);

    /**
     * Create a new grouping context from a byte buffer.
     * @param groupSpec The grouping specification to use for initialization.
     * @param groupSpecLen The length of the grouping specification, in bytes.
     **/
    GroupingContext(const BitVector & validLids, const std::atomic<steady_time> & now_ref, steady_time timeOfDoom);

    /**
     * Shallow copy of references
     **/
    GroupingContext(const GroupingContext & rhs);

    GroupingContext &operator=(const GroupingContext &) = delete;

    /**
     * Add another grouping to this context.
     * @param g Pointer to the grouping object to become part of this context.
     **/
    void addGrouping(std::shared_ptr<Grouping> g);

    /**
     * Reset the context to an empty state.
     **/
    void reset() { _groupingList.clear(); }

    /**
     * Return the internal list of grouping expressions in this context.
     * @return a list of groupings.
     **/
    GroupingList &getGroupingList() noexcept { return _groupingList; }

    /**
     * Serialize the grouping expressions in this context.
     **/
    void serialize();

    /**
     * Check whether this context contains any groupings.
     **/
    bool empty() const noexcept { return _groupingList.empty(); }

    /**
     * Obtain the grouping result.
     *
     * @return grouping result
     **/
    vespalib::nbostream & getResult() { return _os; }

    /**
     * Count number of fs4hits
     *
     * @return number of fs4 hits.
     */
    size_t countFS4Hits();
    /**
     * Will inject the distribution key in the FS4Hits aggregated so far.
     *
     * @param the distribution key.
     */
    void setDistributionKey(uint32_t distributionKey);
    /**
     * Obtain the time of doom.
     */
    steady_time getTimeOfDoom() const noexcept { return _timeOfDoom; }
    bool hasExpired() const noexcept { return _now_ref.load(std::memory_order_relaxed) > _timeOfDoom; }
    /**
     * Figure out if ranking is necessary for any of the grouping requests here.
     * @return true if ranking is required.
     */
    bool needRanking() const noexcept;

    void groupUnordered(const RankedHit *searchResults, uint32_t binSize, const search::BitVector * overflow);
    void groupInRelevanceOrder(const RankedHit *searchResults, uint32_t binSize);
private:
    void aggregate(Grouping & grouping, const RankedHit * rankedHit, unsigned int len, const BitVector * bv) const;
    void aggregate(Grouping & grouping, const RankedHit * rankedHit, unsigned int len) const;
    void aggregate(Grouping & grouping, uint32_t docId, HitRank rank) const;
    unsigned int aggregateRanked(Grouping & grouping, const RankedHit * rankedHit, unsigned int len) const;
    void aggregate(Grouping & grouping, const BitVector * bv, unsigned int lidLimit) const;
    void aggregate(Grouping & grouping, const BitVector * bv, unsigned int , unsigned int topN) const;
    const BitVector                & _validLids;
    const std::atomic<steady_time> & _now_ref;
    steady_time                      _timeOfDoom;
    vespalib::nbostream              _os;
    GroupingList                     _groupingList;
};

}

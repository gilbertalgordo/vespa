// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "wand_parts.h"
#include <vespa/searchlib/queryeval/searchiterator.h>

namespace search::queryeval {

struct WeakAndSearch : SearchIterator {
    using Terms = wand::Terms;
    using MatchParams = wand::MatchParams;
    virtual size_t get_num_terms() const = 0;
    virtual int32_t get_term_weight(size_t idx) const = 0;
    virtual wand::score_t get_max_score(size_t idx) const = 0;
    virtual const Terms &getTerms() const = 0;
    virtual uint32_t getN() const = 0;
    WeakAndSearch *as_weak_and() noexcept override { return this; }
    void visitMembers(vespalib::ObjectVisitor &visitor) const override;
    template<typename Scorer>
    static SearchIterator::UP createArrayWand(const Terms &terms, const MatchParams & matchParams,
                                              const Scorer & scorer, uint32_t n, bool strict, bool readonly_scores_heap);
    template<typename Scorer>
    static SearchIterator::UP createHeapWand(const Terms &terms, const MatchParams & matchParams,
                                             const Scorer & scorer, uint32_t n, bool strict, bool readonly_scores_heap);
    template<typename Scorer>
    static SearchIterator::UP create(const Terms &terms, const MatchParams & matchParams,
                                     const Scorer & scorer, uint32_t n, bool strict, bool readonly_scores_heap);
    static SearchIterator::UP create(const Terms &terms, const MatchParams & matchParams,
                                     uint32_t n, bool strict, bool readonly_scores_heap);
};

}

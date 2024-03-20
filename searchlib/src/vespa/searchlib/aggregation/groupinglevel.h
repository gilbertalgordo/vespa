// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "group.h"
#include <vespa/searchlib/expression/currentindex.h>

namespace search::expression { class CurrentIndexSetup; }

namespace search::aggregation {

class Grouping;

/**
 * This struct contains information about how grouping should be
 * performed on a given level in the grouping tree. The Grouping class
 * holds an array of these, one for each level in the tree below the
 * root.
 **/
class GroupingLevel : public vespalib::Identifiable
{
private:
    using ResultNode = expression::ResultNode;
    using ExpressionNode = expression::ExpressionNode;
    using ExpressionTree = expression::ExpressionTree;
    using CurrentIndex = expression::CurrentIndex;
    using CurrentIndexSetup = expression::CurrentIndexSetup;
    class Grouper {
    public:
        virtual ~Grouper() = default;
        virtual void group(Group & group, const ResultNode & result, DocId doc, HitRank rank) const = 0;
        virtual void group(Group & group, const ResultNode & result, const document::Document & doc, HitRank rank) const = 0;
        virtual Grouper * clone() const = 0;
    protected:
        Grouper(const Grouping * grouping, uint32_t level) noexcept;
        bool  hasNext() const noexcept { return _hasNext; }
        bool   doNext() const noexcept { return _doNext; }
        bool  hasNext(size_t level) const noexcept;
        const Grouping * _grouping;
        uint32_t   _level;
        bool       _frozen;
        bool       _hasNext;
        bool       _doNext;
    };
    class SingleValueGrouper : public Grouper {
    public:
        SingleValueGrouper(const Grouping * grouping, uint32_t level) noexcept : Grouper(grouping, level) { }
    protected:
        template<typename Doc>
        void groupDoc(Group & group, const ResultNode & result, const Doc & doc, HitRank rank) const;
        void group(Group & g, const ResultNode & result, DocId doc, HitRank rank) const override {
            groupDoc(g, result, doc, rank);
        }
        void group(Group & g, const ResultNode & result, const document::Document & doc, HitRank rank) const override {
            groupDoc(g, result, doc, rank);
        }
        SingleValueGrouper * clone() const override { return new SingleValueGrouper(*this); }
    };
    class MultiValueGrouper : public SingleValueGrouper {
    public:
        MultiValueGrouper(CurrentIndex * currentIndex, const Grouping * grouping, uint32_t level) noexcept
            : SingleValueGrouper(grouping, level),
              _currentIndex(currentIndex)
        { }
        MultiValueGrouper(const MultiValueGrouper &) = default; //TODO Try to remove
        MultiValueGrouper & operator=(const MultiValueGrouper &) = delete;
    private:
        template<typename Doc>
        void groupDoc(Group & group, const ResultNode & result, const Doc & doc, HitRank rank) const;
        void group(Group & g, const ResultNode & result, DocId doc, HitRank rank) const override {
            groupDoc(g, result, doc, rank);
        }
        void group(Group & g, const ResultNode & result, const document::Document & doc, HitRank rank) const override {
            groupDoc(g, result, doc, rank);
        }
        MultiValueGrouper * clone() const override { return new MultiValueGrouper(*this); }
        CurrentIndex *_currentIndex;
    };
    int64_t        _maxGroups;
    int64_t        _precision;
    bool           _isOrdered;
    bool           _frozen;
    CurrentIndex   _currentIndex;
    ExpressionTree _classify;
    Group          _collect;

    vespalib::CloneablePtr<Grouper>    _grouper;
public:
    GroupingLevel() noexcept;
    GroupingLevel(GroupingLevel&&) noexcept;
    GroupingLevel& operator=(GroupingLevel&&) noexcept;
    GroupingLevel(const GroupingLevel &);
    GroupingLevel & operator =(const GroupingLevel &);
    ~GroupingLevel();
    DECLARE_IDENTIFIABLE_NS2(search, aggregation, GroupingLevel);
    DECLARE_NBO_SERIALIZE;

    GroupingLevel unchain() const { return *this; }

    GroupingLevel &setMaxGroups(int64_t maxGroups) {
        _maxGroups = maxGroups;
        if ((maxGroups == -1) || (maxGroups > _precision)) {
            _precision = maxGroups;
        }
        return *this;
    }
    GroupingLevel & freeze() { _frozen = true; return *this; }
    GroupingLevel &setPresicion(int64_t precision) { _precision = precision; return *this; }
    GroupingLevel &setExpression(ExpressionNode::UP root) { _classify = std::move(root); return *this; }
    GroupingLevel &addResult(ExpressionNode::UP result) { _collect.addResult(std::move(result)); return *this; }
    GroupingLevel &addResult(const ExpressionNode & result) { return addResult(ExpressionNode::UP(result.clone())); }
    GroupingLevel &addAggregationResult(ExpressionNode::UP aggr) { _collect.addAggregationResult(std::move(aggr)); return *this; }
    GroupingLevel &addOrderBy(ExpressionNode::UP orderBy, bool ascending) { _collect.addOrderBy(std::move(orderBy), ascending); return *this; }
    bool needResort() const { return _collect.needResort(); }

    int64_t getMaxGroups() const noexcept { return _maxGroups; }
    int64_t getPrecision() const noexcept { return _precision; }
    bool        isFrozen() const noexcept { return _frozen; }
    bool    allowMoreGroups(size_t sz) const noexcept { return (!_frozen && (!_isOrdered || (sz < (uint64_t)_precision))); }
    const ExpressionTree & getExpression() const { return _classify; }
    ExpressionTree & getExpression() { return _classify; }
    const       Group &getGroupPrototype() const { return _collect; }
    void wire_current_index(CurrentIndexSetup &setup, const vespalib::ObjectPredicate &resolve_pred, vespalib::ObjectOperation &resolve_op);
    void prepare(const Grouping * grouping, uint32_t level, bool isOrdered_);

    Group &groupPrototype() { return _collect; }
    const Group & groupPrototype() const { return _collect; }

    template<typename Doc>
    void group(Group & g, const ResultNode & result, const Doc & doc, HitRank rank) const {
        _grouper->group(g, result, doc, rank);
    }

    void visitMembers(vespalib::ObjectVisitor &visitor) const override;
    void selectMembers(const vespalib::ObjectPredicate &predicate, vespalib::ObjectOperation &operation) override;
};

}

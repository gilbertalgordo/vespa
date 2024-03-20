// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "singlenumericpostattribute.h"
#include "enumcomparator.h"
#include "enumstore.h"
#include "numeric_direct_posting_store_adapter.hpp"
#include "singlenumericenumattribute.hpp"

namespace search {

template <typename B>
SingleValueNumericPostingAttribute<B>::~SingleValueNumericPostingAttribute()
{
    this->disableFreeLists();
    this->disable_entry_hold_list();
    clearAllPostings();
}

template <typename B>
SingleValueNumericPostingAttribute<B>::SingleValueNumericPostingAttribute(const vespalib::string & name,
                                                                          const AttributeVector::Config & c) :
    SingleValueNumericEnumAttribute<B>(name, c),
    PostingParent(*this, this->getEnumStore()),
    _posting_store_adapter(this->get_posting_store(), this->_enumStore, this->getIsFilter())
{
}

template <typename B>
void
SingleValueNumericPostingAttribute<B>::freezeEnumDictionary()
{
    this->getEnumStore().freeze_dictionary();
}

template <typename B>
void
SingleValueNumericPostingAttribute<B>::mergeMemoryStats(vespalib::MemoryUsage & total)
{
    auto& compaction_strategy = this->getConfig().getCompactionStrategy();
    total.merge(this->_posting_store.update_stat(compaction_strategy));
}

template <typename B>
void
SingleValueNumericPostingAttribute<B>::applyUpdateValueChange(const Change & c,
                                                              EnumStore & enumStore,
                                                              std::map<DocId, EnumIndex> & currEnumIndices)
{
    EnumIndex newIdx;
    if (c.has_entry_ref()) {
        newIdx = EnumIndex(vespalib::datastore::EntryRef(c.get_entry_ref()));
    } else {
        enumStore.find_index(c._data.raw(), newIdx);
    }
    currEnumIndices[c._doc] = newIdx;
}

template <typename B>
void
SingleValueNumericPostingAttribute<B>::
makePostingChange(const vespalib::datastore::EntryComparator &cmpa,
                  const std::map<DocId, EnumIndex> &currEnumIndices,
                  PostingMap &changePost)
{
    for (const auto& elem : currEnumIndices) {
        uint32_t docId = elem.first;
        EnumIndex oldIdx = this->_enumIndices[docId].load_relaxed();
        EnumIndex newIdx = elem.second;

        // add new posting
        changePost[EnumPostingPair(newIdx, &cmpa)].add(docId, 1);

        // remove old posting
        if ( oldIdx.valid()) {
            changePost[EnumPostingPair(oldIdx, &cmpa)].remove(docId);
        }
    }
}


template <typename B>
void
SingleValueNumericPostingAttribute<B>::applyValueChanges(EnumStoreBatchUpdater& updater)
{
    EnumStore & enumStore = this->getEnumStore();
    IEnumStoreDictionary& dictionary = enumStore.get_dictionary();
    PostingMap changePost;

    // used to make sure several arithmetic operations on the same document in a single commit works
    std::map<DocId, EnumIndex> currEnumIndices;

    for (const auto& change : this->_changes.getInsertOrder()) {
        auto enumIter = currEnumIndices.find(change._doc);
        EnumIndex oldIdx;
        if (enumIter != currEnumIndices.end()) {
            oldIdx = enumIter->second;
        } else {
            oldIdx = this->_enumIndices[change._doc].load_relaxed();
        }

        if (change._type == ChangeBase::UPDATE) {
            applyUpdateValueChange(change, enumStore, currEnumIndices);
        } else if (change._type >= ChangeBase::ADD && change._type <= ChangeBase::DIV) {
            if (oldIdx.valid()) {
                T oldValue = enumStore.get_value(oldIdx);
                T newValue = this->template applyArithmetic<T, typename Change::DataType>(oldValue, change._data.getArithOperand(), change._type);
                EnumIndex newIdx;
                (void) dictionary.find_index(enumStore.make_comparator(newValue), newIdx);
                currEnumIndices[change._doc] = newIdx;
            }
        } else if(change._type == ChangeBase::CLEARDOC) {
            currEnumIndices[change._doc] = enumStore.get_default_value_ref().load_relaxed();
        }
    }

    makePostingChange(enumStore.get_comparator(), currEnumIndices, changePost);

    this->updatePostings(changePost);
    SingleValueNumericEnumAttribute<B>::applyValueChanges(updater);
}

template <typename B>
void
SingleValueNumericPostingAttribute<B>::reclaim_memory(generation_t oldest_used_gen)
{
    SingleValueNumericEnumAttribute<B>::reclaim_memory(oldest_used_gen);
    _posting_store.reclaim_memory(oldest_used_gen);
}

template <typename B>
void
SingleValueNumericPostingAttribute<B>::before_inc_generation(generation_t current_gen)
{
    _posting_store.freeze();
    SingleValueNumericEnumAttribute<B>::before_inc_generation(current_gen);
    _posting_store.assign_generation(current_gen);
}

template <typename B>
std::unique_ptr<attribute::SearchContext>
SingleValueNumericPostingAttribute<B>::getSearch(QueryTermSimple::UP qTerm,
                                                 const attribute::SearchContextParams & params) const
{
    using BaseSC = attribute::SingleNumericEnumSearchContext<T>;
    using SC = attribute::NumericPostingSearchContext<BaseSC, SelfType, vespalib::btree::BTreeNoLeafData>;
    auto docid_limit = this->getCommittedDocIdLimit();
    BaseSC base_sc(std::move(qTerm), *this, this->_enumIndices.make_read_view(docid_limit), this->_enumStore);
    return std::make_unique<SC>(std::move(base_sc), params, *this);
}

template <typename B>
const IDocidPostingStore*
SingleValueNumericPostingAttribute<B>::as_docid_posting_store() const
{
    if (this->getConfig().basicType().is_integer_type()) {
        return &_posting_store_adapter;
    }
    return nullptr;
}

}


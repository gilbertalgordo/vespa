// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "matching_elements_filler.h"
#include "querytermdata.h"
#include <vespa/searchlib/common/matching_elements.h>
#include <vespa/searchlib/common/matching_elements_fields.h>
#include <vespa/searchlib/fef/iindexenvironment.h>
#include <vespa/searchlib/query/streaming/in_term.h>
#include <vespa/searchlib/query/streaming/same_element_query_node.h>
#include <vespa/searchlib/query/streaming/weighted_set_term.h>
#include <vespa/vsm/searcher/fieldsearcher.h>
#include <vespa/vdslib/container/searchresult.h>
#include "hitcollector.h"
#include <algorithm>
#include <optional>

using search::MatchingElements;
using search::MatchingElementsFields;
using search::streaming::AndNotQueryNode;
using search::streaming::HitList;
using search::streaming::InTerm;
using search::streaming::MultiTerm;
using search::streaming::Query;
using search::streaming::QueryConnector;
using search::streaming::QueryNode;
using search::streaming::QueryTerm;
using search::streaming::SameElementQueryNode;
using search::streaming::WeightedSetTerm;
using streaming::QueryTermData;
using vdslib::SearchResult;
using vsm::FieldIdT;
using vsm::FieldIdTSearcherMap;
using vsm::StorageDocument;

namespace streaming {

namespace {

struct SubFieldTerm
{
    std::string _field_name;
    FieldIdT _id;
    const QueryTerm* _term;
public:
    SubFieldTerm(std::string field_name, FieldIdT id, const QueryTerm* term) noexcept
        : _field_name(std::move(field_name)),
          _id(id),
          _term(term)
    {
    }
    const std::string& get_field_name() const noexcept { return _field_name; }
    const QueryTerm& get_term() const noexcept { return *_term; }
    FieldIdT get_id() const noexcept { return _id; }
};

class Matcher
{
    std::vector<const SameElementQueryNode*> _same_element_nodes;
    std::vector<SubFieldTerm> _sub_field_terms;
    vsm::FieldIdTSearcherMap& _field_searcher_map;
    const search::fef::IIndexEnvironment& _index_env;
    HitList _hit_list;
    std::vector<uint32_t> _elements;

    const std::string* matching_elements_field(const MatchingElementsFields& fields, FieldIdT field_id);
    void select_multiterm_children(const MatchingElementsFields& fields, const MultiTerm& multi_term);
    void select_query_term_node(const MatchingElementsFields& fields, const QueryTerm& term);
    void select_query_nodes(const MatchingElementsFields& fields, const QueryNode& query_node);
    void add_matching_elements(const std::string& field_name, std::optional<FieldIdT> field_id, uint32_t doc_lid, const HitList& hit_list, MatchingElements& matching_elements);
    void find_matching_elements(const SameElementQueryNode& same_element, uint32_t doc_lid, MatchingElements& matching_elements);
    void find_matching_elements(const SubFieldTerm& sub_field_term, uint32_t doc_lid, MatchingElements& matching_elements);
public:
    Matcher(vsm::FieldIdTSearcherMap& field_searcher_map, const search::fef::IIndexEnvironment& index_env,
            const MatchingElementsFields& fields, const Query& query);
    ~Matcher();
    bool empty() const { return _same_element_nodes.empty() && _sub_field_terms.empty(); }
    void find_matching_elements(const vsm::StorageDocument& doc, uint32_t doc_lid, MatchingElements& matching_elements);
};

template<typename T>
const T* as(const QueryNode& query_node) { return dynamic_cast<const T*>(&query_node); }

Matcher::Matcher(FieldIdTSearcherMap& field_searcher_map, const search::fef::IIndexEnvironment& index_env,
                 const MatchingElementsFields& fields, const Query& query)
    : _same_element_nodes(),
      _sub_field_terms(),
      _field_searcher_map(field_searcher_map),
      _index_env(index_env),
      _hit_list()
{
    select_query_nodes(fields, query.getRoot());
}

Matcher::~Matcher() = default;

const std::string*
Matcher::matching_elements_field(const MatchingElementsFields& fields, FieldIdT field_id)
{
    auto* field_info = _index_env.getField(field_id);
    if (field_info != nullptr) {
        auto &field_name = field_info->name();
        if (fields.has_struct_field(field_name)) {
            return &fields.get_enclosing_field(field_name);
        } else if (fields.has_field(field_name)) {
            return &field_name;
        }
    }
    return nullptr;
}

void
Matcher::select_multiterm_children(const MatchingElementsFields& fields, const MultiTerm& multi_term)
{
    auto& qtd = dynamic_cast<const QueryTermData &>(multi_term.getQueryItem());
    auto& td = qtd.getTermData();
    auto num_fields = td.numFields();
    for (size_t i = 0; i < num_fields; ++i) {
        auto field_id = td.field(i).getFieldId();
        auto* field = matching_elements_field(fields, field_id);
        if (field != nullptr) {
            auto& terms = multi_term.get_terms();
            for (auto &term : terms) {
                _sub_field_terms.emplace_back(*field, field_id, term.get());
            }
        }
    }
}

void
Matcher::select_query_term_node(const MatchingElementsFields& fields, const QueryTerm& query_term)
{
    auto& qtd = dynamic_cast<const QueryTermData &>(query_term.getQueryItem());
    auto& td = qtd.getTermData();
    auto num_fields = td.numFields();
    for (size_t i = 0; i < num_fields; ++i) {
        auto field_id = td.field(i).getFieldId();
        auto* field = matching_elements_field(fields, field_id);
        if (field != nullptr) {
            _sub_field_terms.emplace_back(*field, field_id, &query_term);
        }
    }
}

void
Matcher::select_query_nodes(const MatchingElementsFields& fields, const QueryNode& query_node)
{
    if (auto same_element = as<SameElementQueryNode>(query_node)) {
        if (fields.has_field(same_element->getIndex())) {
            _same_element_nodes.emplace_back(same_element);
        }
    } else if (auto weighted_set_term = as<WeightedSetTerm>(query_node)) {
        select_multiterm_children(fields, *weighted_set_term);
    } else if (auto in_term = as<InTerm>(query_node)) {
        select_multiterm_children(fields, *in_term);
    } else if (auto query_term = as<QueryTerm>(query_node)) {
        select_query_term_node(fields, *query_term);
    } else if (auto and_not = as<AndNotQueryNode>(query_node)) {
        select_query_nodes(fields, *(*and_not)[0]);
    } else if (auto intermediate = as<QueryConnector>(query_node)) {
        for (size_t i = 0; i < intermediate->size(); ++i) {
            select_query_nodes(fields, *(*intermediate)[i]);
        }
    }
}

void
Matcher::add_matching_elements(const std::string& field_name, std::optional<FieldIdT> field_id, uint32_t doc_lid, const HitList& hit_list, MatchingElements& matching_elements)
{
    _elements.clear();
    for (auto& hit : hit_list) {
        if (!field_id.has_value() || field_id.value() == hit.field_id()) {
            _elements.emplace_back(hit.element_id());
        }
    }
    if (_elements.size() > 1) {
        std::sort(_elements.begin(), _elements.end());
        auto last = std::unique(_elements.begin(), _elements.end());
        _elements.erase(last, _elements.end());
    }
    matching_elements.add_matching_elements(doc_lid, field_name, _elements);
}
                               
void
Matcher::find_matching_elements(const SameElementQueryNode& same_element, uint32_t doc_lid, MatchingElements& matching_elements)
{
    const HitList& hit_list = same_element.evaluateHits(_hit_list);
    if (!hit_list.empty()) {
        add_matching_elements(same_element.getIndex(), std::nullopt, doc_lid, hit_list, matching_elements);
    }
}

void
Matcher::find_matching_elements(const SubFieldTerm& sub_field_term, uint32_t doc_lid, MatchingElements& matching_elements)
{
    const HitList& hit_list = sub_field_term.get_term().evaluateHits(_hit_list);
    if (!hit_list.empty()) {
        add_matching_elements(sub_field_term.get_field_name(), sub_field_term.get_id(), doc_lid, hit_list, matching_elements);
    }
}

void
Matcher::find_matching_elements(const StorageDocument& doc, uint32_t doc_lid, MatchingElements& matching_elements)
{
    for (vsm::FieldSearcherContainer& fSearch : _field_searcher_map) {
        fSearch->search(doc);
    }
    for (const auto* same_element : _same_element_nodes) {
        find_matching_elements(*same_element, doc_lid, matching_elements);
    }
    for (const auto& term : _sub_field_terms) {
        find_matching_elements(term, doc_lid, matching_elements);
    }
}

}

MatchingElementsFiller::MatchingElementsFiller(FieldIdTSearcherMap& field_searcher_map,
                                               const search::fef::IIndexEnvironment& index_env, Query& query,
                                               HitCollector& hit_collector, SearchResult& search_result)
    : vsm::IMatchingElementsFiller(),
      _field_searcher_map(field_searcher_map),
      _index_env(index_env),
      _query(query),
      _hit_collector(hit_collector),
      _search_result(search_result)
{
}

MatchingElementsFiller::~MatchingElementsFiller() = default;

std::unique_ptr<MatchingElements>
MatchingElementsFiller::fill_matching_elements(const MatchingElementsFields& fields)
{
    auto result = std::make_unique<MatchingElements>();
    if (fields.empty()) {
        return result;
    }
    Matcher matcher(_field_searcher_map, _index_env, fields, _query);
    if (matcher.empty()) {
        return result;
    }
    // Scan documents that will be returned as hits
    size_t count = std::min(_search_result.getHitCount(), _search_result.getWantedHitCount());
    for (size_t i(0); i < count; i++ ) {
        const char* doc_id(nullptr);
        SearchResult::RankType rank(0);
        uint32_t lid = _search_result.getHit(i, doc_id, rank);
        const vsm::Document& vsm_doc = _hit_collector.getDocSum(lid);
        const auto& doc = dynamic_cast<const StorageDocument&>(vsm_doc);
        matcher.find_matching_elements(doc, lid, *result);
        _query.reset();
    }
    return result;
}

}

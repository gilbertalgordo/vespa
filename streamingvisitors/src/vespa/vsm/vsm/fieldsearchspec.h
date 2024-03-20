// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/searchcommon/attribute/distance_metric.h>
#include <vespa/vsm/searcher/fieldsearcher.h>
#include <vespa/vsm/config/vsm-cfif.h>

namespace vsm {

class FieldSearchSpec
{
public:
    using Searchmethod = VsmfieldsConfig::Fieldspec::Searchmethod;
    using Normalizing = search::Normalizing;
    FieldSearchSpec();
    FieldSearchSpec(const FieldIdT & id, const vespalib::string & name, Searchmethod searchMethod,
                    Normalizing normalize_mode, vespalib::stringref arg1, size_t maxLength);
    ~FieldSearchSpec();
    FieldSearchSpec(FieldSearchSpec&& rhs) noexcept;
    FieldSearchSpec& operator=(FieldSearchSpec&& rhs) noexcept;
    const FieldSearcher & searcher() const noexcept { return *_searcher; }
    const vespalib::string &  name() const noexcept { return _name; }
    FieldIdT                    id() const noexcept { return _id; }
    bool                     valid() const noexcept { return static_cast<bool>(_searcher); }
    size_t               maxLength() const noexcept { return _maxLength; }
    Normalizing     normalize_mode() const noexcept { return _normalize_mode; }
    const vespalib::string&   arg1() const noexcept { return _arg1; }
    bool uses_nearest_neighbor_search_method() const noexcept {
        return _searchMethod == Searchmethod::NEAREST_NEIGHBOR;
    }
    bool uses_string_search_method() const noexcept {
        return  (_searchMethod == Searchmethod::UTF8) ||
                (_searchMethod == Searchmethod::AUTOUTF8) ||
                (_searchMethod == Searchmethod::SSE2UTF8);
    }

    /**
     * Reconfigures the field searcher based on information in the given query term.
     **/
    void reconfig(const search::streaming::QueryTerm & term);

    friend vespalib::asciistream & operator <<(vespalib::asciistream & os, const FieldSearchSpec & f);

private:
    void propagate_settings_to_searcher();

    FieldIdT               _id;
    vespalib::string       _name;
    size_t                 _maxLength;
    FieldSearcherContainer _searcher;
    Searchmethod           _searchMethod;
    Normalizing            _normalize_mode;
    vespalib::string       _arg1;
    bool                   _reconfigured;
};

using FieldSearchSpecMapT = std::map<FieldIdT, FieldSearchSpec>;

class FieldSearchSpecMap
{
public:
    FieldSearchSpecMap();
    ~FieldSearchSpecMap();

    /**
     * Iterates over all fields in the vsmfields config and creates a mapping from field id to FieldSearchSpec objects
     * and a mapping from field name to field id. It then iterates over all document types and index names
     * and creates a mapping from index name to list of field ids for each document type.
     **/
    void buildFromConfig(const VsmfieldsHandle & conf);

    /**
     * Iterates over the given field name vector adding extra elements to the mapping from field name to field id.
     **/
    void buildFromConfig(const std::vector<vespalib::string> & otherFieldsNeeded);

    /**
     * Reconfigures some of the field searchers based on information in the given query.
     **/
    void reconfigFromQuery(const search::streaming::Query & query);

    /**
     * Adds a [field name, field id] entry to the given mapping for each field name used in the given query.
     * This is achieved by mapping from query term index name -> list of field ids -> [field name, field id] pairs.
     **/
    StringFieldIdTMap buildFieldsInQuery(const search::streaming::Query & query) const;
    void addFieldsFromIndex(vespalib::stringref index, StringFieldIdTMap & fieldIdMap) const;

    /**
     * Adds a FieldSearcher object to the given field searcher map for each field name in the other map.
     **/
    void buildSearcherMap(const StringFieldIdTMapT & fieldsInQuery, FieldIdTSearcherMap & fieldSearcherMap) const;

    const FieldSearchSpecMapT & specMap()                 const { return _specMap; }
    //const IndexFieldMapT & indexMap()                     const { return _documentTypeMap.begin()->second; }
    const DocumentTypeIndexFieldMapT & documentTypeMap()  const { return _documentTypeMap; }
    const StringFieldIdTMap & nameIdMap()                 const { return _nameIdMap; }
    friend vespalib::asciistream & operator <<(vespalib::asciistream & os, const FieldSearchSpecMap & f);

    static vespalib::string stripNonFields(vespalib::stringref rawIndex);
    search::attribute::DistanceMetric get_distance_metric(const vespalib::string& name) const;

private:
    FieldSearchSpecMapT         _specMap;         // mapping from field id to field search spec
    DocumentTypeIndexFieldMapT  _documentTypeMap; // mapping from index name to field id list for each document type
    StringFieldIdTMap           _nameIdMap;       // mapping from field name to field id
};

}


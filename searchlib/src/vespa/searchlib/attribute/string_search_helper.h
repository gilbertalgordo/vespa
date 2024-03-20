// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "dfa_string_comparator.h"
#include <vespa/vespalib/fuzzy/fuzzy_matching_algorithm.h>
#include <vespa/vespalib/regex/regex.h>

namespace vespalib { class FuzzyMatcher; }
namespace search { class QueryTermUCS4; }

namespace search::attribute {

class DfaFuzzyMatcher;

/**
 * Helper class for search context when scanning string fields
 * It handles different search settings like prefix, regex and cased/uncased.
 */
class StringSearchHelper {
public:
    using FuzzyMatcher = vespalib::FuzzyMatcher;
    StringSearchHelper(QueryTermUCS4 & qTerm, bool cased,
                       vespalib::FuzzyMatchingAlgorithm fuzzy_matching_algorithm = vespalib::FuzzyMatchingAlgorithm::BruteForce);
    StringSearchHelper(StringSearchHelper&&) noexcept;
    StringSearchHelper(const StringSearchHelper &) = delete;
    StringSearchHelper & operator =(const StringSearchHelper &) = delete;
    ~StringSearchHelper();
    bool isMatch(const char *src) const noexcept;
    bool isPrefix() const noexcept { return _isPrefix; }
    bool isRegex() const noexcept { return _isRegex; }
    bool isCased() const noexcept { return _isCased; }
    bool isFuzzy() const noexcept { return _isFuzzy; }
    const vespalib::Regex & getRegex() const noexcept { return _regex; }
    const FuzzyMatcher& getFuzzyMatcher() const noexcept { return *_fuzzyMatcher; }

    template <typename DictionaryConstIteratorType>
    bool is_fuzzy_match(const char* word, DictionaryConstIteratorType& itr, const DfaStringComparator::DataStoreType& data_store) const;

private:
    using ucs4_t = uint32_t;
    vespalib::Regex                _regex;
    std::unique_ptr<FuzzyMatcher>  _fuzzyMatcher;
    std::unique_ptr<DfaFuzzyMatcher> _dfa_fuzzy_matcher;
    std::unique_ptr<ucs4_t[]>      _ucs4;
    const char *                   _term;
    uint32_t                       _termLen; // measured in bytes
    bool                           _isPrefix;
    bool                           _isRegex;
    bool                           _isCased;
    bool                           _isFuzzy;
};

}

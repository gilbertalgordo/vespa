// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <string>

namespace search::docsummary {

/*
 * Explicit values used when JuniperDFWQueryItem doesn't have a query
 * stack dump iterator.
 */
struct JuniperDFWExplicitItemData
{
    std::string_view _index;
    int32_t _weight;

    JuniperDFWExplicitItemData()
        : _index(), _weight(0)
    {}
};

}

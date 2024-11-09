// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <string>

namespace search::parseitem { enum class ItemCreator; }

namespace juniper {

using ItemCreator = search::parseitem::ItemCreator;

/*
 * Interface class for juniper query items.
 */
class QueryItem {
public:
    virtual ~QueryItem() = default;
    virtual std::string_view get_index() const = 0;
    virtual int get_weight() const = 0;
    virtual ItemCreator get_creator() const = 0;
};

};

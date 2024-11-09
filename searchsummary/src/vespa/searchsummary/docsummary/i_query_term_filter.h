// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <string>

namespace search::docsummary {

/*
 * Interface class for checking if query term view indicates that
 * related query term is useful from the perspective of juniper.
 */
class IQueryTermFilter
{
public:
    virtual ~IQueryTermFilter() = default;

    virtual bool use_view(std::string_view view) const = 0;
};

}

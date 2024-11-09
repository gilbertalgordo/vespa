// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "viewresolver.h"
#include <vespa/searchcommon/common/schema.h>

namespace proton::matching {

ViewResolver &
ViewResolver::add(const std::string & view, std::string_view field)
{
    _map[view].emplace_back(field);
    return *this;
}

bool
ViewResolver::resolve(std::string_view view, std::vector<std::string> &fields) const
{
    auto pos = _map.find(view);
    if (pos == _map.end()) {
        fields.emplace_back(view);
        return false;
    }
    fields = pos->second;
    return true;
}

ViewResolver
ViewResolver::createFromSchema(const search::index::Schema &schema)
{
    ViewResolver resolver;
    for (uint32_t i = 0; i < schema.getNumFieldSets(); ++i) {
        const search::index::Schema::FieldSet &f = schema.getFieldSet(i);
        const std::string &view = f.getName();
        const std::vector<std::string> &fields = f.getFields();
        for (const auto & field : fields) {
            resolver.add(view, field);
        }
    }
    return resolver;
}

}

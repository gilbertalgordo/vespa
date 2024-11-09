// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/searchcommon/attribute/iattributecontext.h>
#include <vespa/vespalib/stllike/hash_set.h>

namespace streaming {

/*
 * This class wraps an IAttributeContext and records accesses to attribute
 * vectors.
 */
class AttributeAccessRecorder : public search::attribute::IAttributeContext
{
    std::unique_ptr<search::attribute::IAttributeContext> _ctx;
    mutable vespalib::hash_set<std::string> _accessed_attributes;

public:
    AttributeAccessRecorder(std::unique_ptr<IAttributeContext> ctx);
    ~AttributeAccessRecorder() override;
    void asyncForAttribute(std::string_view name, std::unique_ptr<search::attribute::IAttributeFunctor> func) const override;
    const search::attribute::IAttributeVector* getAttribute(std::string_view name) const override;
    const search::attribute::IAttributeVector * getAttributeStableEnum(std::string_view name) const override;
    void getAttributeList(std::vector<const search::attribute::IAttributeVector *>& list) const override;
    void releaseEnumGuards() override;
    std::vector<std::string> get_accessed_attributes() const;
};

}

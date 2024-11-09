// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "blueprint.h"
#include "parametervalidator.h"
#include <vespa/vespalib/util/stringfmt.h>
#include <cassert>
#include <string>

namespace search::fef {

std::optional<FeatureType>
Blueprint::defineInput(const std::string & inName, AcceptInput accept)
{
    assert(_dependency_handler != nullptr);
    return _dependency_handler->resolve_input(inName, accept);
}

void
Blueprint::describeOutput(const std::string & outName, std::string_view desc, FeatureType type)
{
    (void) desc;
    assert(_dependency_handler != nullptr);
    _dependency_handler->define_output(outName, std::move(type));
}

bool
Blueprint::fail(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    std::string msg = vespalib::make_string_va(format, ap);
    va_end(ap);
    assert(_dependency_handler != nullptr);
    _dependency_handler->fail(msg);
    return false;
}

Blueprint::Blueprint(std::string_view baseName)
    : _baseName(baseName),
      _name(),
      _dependency_handler(nullptr)
{
}

Blueprint::~Blueprint() = default;

ParameterDescriptions
Blueprint::getDescriptions() const
{
    // desc: 0-n parameters
    return ParameterDescriptions().desc().string().repeat();
}

bool
Blueprint::setup(const IIndexEnvironment &indexEnv,
                 const StringVector &params)
{
    ParameterDescriptions descs = getDescriptions();
    ParameterValidator validator(indexEnv, params, descs);
    ParameterValidator::Result result = validator.validate();
    if (result.valid()) {
        return setup(indexEnv, result.getParameters());
    } else {
        return fail("The parameter list used for setting up rank feature %s is not valid: %s",
                    getBaseName().c_str(), result.getError().c_str());
    }
}

bool
Blueprint::setup(const IIndexEnvironment &indexEnv, const ParameterList &params)
{
    (void) indexEnv; (void) params;
    return fail("The setup function using a typed parameter list does not have a default implementation. "
                "Make sure the setup function is implemented in the rank feature %s.", getBaseName().c_str());
}

void
Blueprint::prepareSharedState(const IQueryEnvironment & queryEnv, IObjectStore & objectStore) const {
    (void) queryEnv; (void) objectStore;
}

using IAttributeVectorWrapper = AnyWrapper<const attribute::IAttributeVector *>;

const attribute::IAttributeVector *
Blueprint::lookupAndStoreAttribute(const std::string & key, std::string_view attrName,
                                   const IQueryEnvironment & env, IObjectStore & store)
{
    const Anything * obj = store.get(key);
    if (obj == nullptr) {
        const IAttributeVector * attribute = env.getAttributeContext().getAttribute(attrName);
        store.add(key, std::make_unique<IAttributeVectorWrapper>(attribute));
        return attribute;
    }
    return IAttributeVectorWrapper::getValue(*obj);
}

const attribute::IAttributeVector *
Blueprint::lookupAttribute(const std::string & key, std::string_view attrName, const IQueryEnvironment & env)
{
    const Anything * attributeArg = env.getObjectStore().get(key);
    const IAttributeVector * attribute = (attributeArg != nullptr)
                                       ? IAttributeVectorWrapper::getValue(*attributeArg)
                                       : nullptr;
    if (attribute == nullptr) {
        attribute = env.getAttributeContext().getAttribute(attrName);
    }
    return attribute;;
}

std::string
Blueprint::createAttributeKey(std::string_view attrName) {
    return "fef.attribute.key." + std::string(attrName);
}

}

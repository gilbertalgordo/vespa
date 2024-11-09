// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <memory>
#include <string>

namespace proton {

class IDocumentDBReference;

/*
 * Interface class for a registry of named IDocumentDBReferences.
 */
class IDocumentDBReferenceRegistry
{
public:
    virtual ~IDocumentDBReferenceRegistry() = default;

    /*
     * Get named IDocumentDBReference.  Block while it doesn't exist.
     */
    virtual std::shared_ptr<IDocumentDBReference> get(std::string_view name) const = 0;

    /*
     * Get named IDocumentDBReference.  Returns empty shared pointer if
     * it doesn't exist.
     */
    virtual std::shared_ptr<IDocumentDBReference> tryGet(std::string_view name) const = 0;

    virtual void add(std::string_view name, std::shared_ptr<IDocumentDBReference> referee) = 0;

    virtual void remove(std::string_view name) = 0;
};

} // namespace proton

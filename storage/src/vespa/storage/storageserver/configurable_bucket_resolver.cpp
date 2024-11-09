// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "configurable_bucket_resolver.h"
#include <vespa/document/base/documentid.h>
#include <vespa/document/bucket/fixed_bucket_spaces.h>
#include <vespa/vespalib/util/exceptions.h>
#include <vespa/vespalib/stllike/hash_map.hpp>
#include <string>

using namespace document;

namespace storage {

ConfigurableBucketResolver::ConfigurableBucketResolver(BucketSpaceMapping type_to_space) noexcept
    : _type_to_space(std::move(type_to_space))
{}
ConfigurableBucketResolver::~ConfigurableBucketResolver() = default;

document::Bucket
ConfigurableBucketResolver::bucketFromId(const DocumentId& id) const {
    if (!id.hasDocType()) {
        // Legacy document ids without document type maps to default bucket space
        return {FixedBucketSpaces::default_space(), BucketId(0)};
    }
    auto iter = _type_to_space.find(id.getDocType());
    if (iter != _type_to_space.end()) {
        return {iter->second, BucketId(0)};
    }
    throw UnknownBucketSpaceException("Unknown bucket space mapping for document type '"
                                      + std::string(id.getDocType()) + "' in id: '" + id.toString() + "'", VESPA_STRLOC);
}

BucketSpace ConfigurableBucketResolver::bucketSpaceFromName(const std::string& name) const {
    return FixedBucketSpaces::from_string(name);
}

std::string ConfigurableBucketResolver::nameFromBucketSpace(const BucketSpace& space) const {
    return FixedBucketSpaces::to_string(space);
}

std::shared_ptr<ConfigurableBucketResolver> ConfigurableBucketResolver::from_config(
        const vespa::config::content::core::BucketspacesConfig& config) {
    ConfigurableBucketResolver::BucketSpaceMapping type_to_space;
    for (auto& mapping : config.documenttype) {
        type_to_space.insert(std::make_pair(mapping.name, FixedBucketSpaces::from_string(mapping.bucketspace)));
    }
    return std::make_shared<ConfigurableBucketResolver>(std::move(type_to_space));
}

}

VESPALIB_HASH_MAP_INSTANTIATE(std::string, document::BucketSpace);

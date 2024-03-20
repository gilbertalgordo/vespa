// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/searchlib/fef/blueprint.h>
#include <vespa/searchcommon/attribute/attributecontent.h>

namespace search::features {

/** Convenience typedef. */
using GeoLocationSpecPtrs = std::vector<const search::common::GeoLocationSpec *>;

/**
 * Implements the executor for the distance feature.
 */
class DistanceExecutor {
public:
    static const feature_t DEFAULT_DISTANCE;
};

/**
 * Implements the blueprint for the distance executor.
 */
class DistanceBlueprint : public fef::Blueprint {
private:
    vespalib::string _field_name;
    vespalib::string _label_name;
    vespalib::string _attr_name;
    uint32_t _attr_id;
    bool _use_geo_pos;
    bool _use_nns_tensor;
    bool _use_item_label;

    bool setup_geopos(const vespalib::string &attr);
    bool setup_nns(const vespalib::string &attr);

public:
    DistanceBlueprint();
    ~DistanceBlueprint();
    void visitDumpFeatures(const fef::IIndexEnvironment & env, fef::IDumpFeatureVisitor & visitor) const override;
    fef::Blueprint::UP createInstance() const override;
    fef::ParameterDescriptions getDescriptions() const override {
        return fef::ParameterDescriptions().desc().string().desc().string().string();
    }
    bool setup(const fef::IIndexEnvironment & env, const fef::ParameterList & params) override;
    void prepareSharedState(const fef::IQueryEnvironment& env, fef::IObjectStore& store) const override;
    fef::FeatureExecutor &createExecutor(const fef::IQueryEnvironment &env, vespalib::Stash &stash) const override;
};

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/metrics/handle.h>
#include <string>

namespace metrics {

struct MetricNameIdTag {};
struct DescriptionIdTag {};
struct TagKeyIdTag {};
struct TagValueIdTag {};

using MetricNameId = vespalib::metrics::Handle<MetricNameIdTag>;
using DescriptionId = vespalib::metrics::Handle<DescriptionIdTag>;
using TagKeyId = vespalib::metrics::Handle<TagKeyIdTag>;
using TagValueId = vespalib::metrics::Handle<TagValueIdTag>;

struct NameRepo {
    static MetricNameId metricId(std::string_view name);
    static DescriptionId descriptionId(std::string_view name);
    static TagKeyId tagKeyId(std::string_view name);
    static TagValueId tagValueId(std::string_view value);

    static const std::string& metricName(MetricNameId id);
    static const std::string& description(DescriptionId id);
    static const std::string& tagKey(TagKeyId id);
    static const std::string& tagValue(TagValueId id);
};

} // metrics


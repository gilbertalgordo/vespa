// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.athenz.api;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public record AthenzDomainMeta(
        @JsonProperty("account") String account,
        @JsonProperty("gcpProject") String gcpProject,
        @JsonProperty("name") String name
) {
}

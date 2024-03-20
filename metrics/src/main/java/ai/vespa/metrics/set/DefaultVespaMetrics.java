// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package ai.vespa.metrics.set;

import ai.vespa.metrics.ContainerMetrics;
import ai.vespa.metrics.SearchNodeMetrics;

/**
 * Encapsulates a minimal set of Vespa metrics to be used as default for all metrics consumers.
 *
 * Note: most predefined metric sets use this as a child, so changing this will require updating
 *       e.g. the list of Vespa metrics in the Datadog integration.
 *
 * @author leandroalves
 */
public class DefaultVespaMetrics {
    public static final MetricSet defaultVespaMetricSet = createDefaultVespaMetricSet();

    private static MetricSet createDefaultVespaMetricSet() {

        return new MetricSet.Builder("default-vespa")
                .metric(ContainerMetrics.FEED_OPERATIONS.rate())
                .metric(SearchNodeMetrics.CONTENT_PROTON_RESOURCE_USAGE_FEEDING_BLOCKED.last())
                .build();
    }

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package ai.vespa.metrics.set;

import ai.vespa.metrics.HostedNodeAdminMetrics;

import java.util.Set;

/**
 * @author gjoranv
 */

// TODO: Move to hosted repo.
public class NetworkMetrics {

    public static final MetricSet networkMetricSet = createNetworkMetricSet();

    private static MetricSet createNetworkMetricSet() {
        Set<Metric> dockerNetworkMetrics =
                Set.of(new Metric(HostedNodeAdminMetrics.NET_IN_BYTES.baseName()),
                                new Metric(HostedNodeAdminMetrics.NET_IN_ERROR.baseName()),
                                new Metric(HostedNodeAdminMetrics.NET_IN_DROPPED.baseName()),
                                new Metric(HostedNodeAdminMetrics.NET_OUT_BYTES.baseName()),
                                new Metric(HostedNodeAdminMetrics.NET_OUT_ERROR.baseName()),
                                new Metric(HostedNodeAdminMetrics.NET_OUT_DROPPED.baseName()),
                                new Metric(HostedNodeAdminMetrics.BANDWIDTH_LIMIT.baseName())
                );

        return new MetricSet("network", dockerNetworkMetrics);
    }
}

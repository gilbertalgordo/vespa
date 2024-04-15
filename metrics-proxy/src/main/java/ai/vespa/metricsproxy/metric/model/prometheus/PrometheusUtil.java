// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package ai.vespa.metricsproxy.metric.model.prometheus;

import ai.vespa.metricsproxy.metric.model.MetricsPacket;
import ai.vespa.metricsproxy.metric.model.ServiceId;
import io.prometheus.client.Collector;
import io.prometheus.client.Collector.MetricFamilySamples;
import io.prometheus.client.Collector.MetricFamilySamples.Sample;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * @author yj-jtakagi
 * @author gjoranv
 */
public class PrometheusUtil {

    private static String sanitize(String name, Map<String, String> sanitizedCache) {
        return sanitizedCache.computeIfAbsent(name, key -> {
            String sanitized = Collector.sanitizeMetricName(name);
            return (name.equals(sanitized)) ? name : sanitized;
        });
    }

    public static PrometheusModel toPrometheusModel(List<MetricsPacket> metricsPackets) {
        Map<String, String> sanitizedMetrics = new HashMap<>();
        Map<ServiceId, List<MetricsPacket>> packetsByService = metricsPackets.stream()
                .collect(Collectors.groupingBy(packet -> packet.service));

        List<MetricFamilySamples> metricFamilySamples = new ArrayList<>(packetsByService.size());

        Map<String, List<Sample>> samples = new HashMap<>();
        packetsByService.forEach(((serviceId, packets) -> {

            var serviceName = sanitize(serviceId.id, sanitizedMetrics);
            for (var packet : packets) {
                Long timeStamp = packet.timestamp * 1000;
                var dimensions = packet.dimensions();
                List<String> labels = new ArrayList<>(dimensions.size());
                List<String> labelValues = new ArrayList<>(dimensions.size());
                for (var entry : dimensions.entrySet()) {
                    var labelName = sanitize(entry.getKey().id, sanitizedMetrics);
                    labels.add(labelName);
                    labelValues.add(entry.getValue());
                }
                labels.add("vespa_service");
                labelValues.add(serviceName);

                for (var metric : packet.metrics().entrySet()) {
                    var metricName = sanitize(metric.getKey().id, sanitizedMetrics);
                    List<Sample> sampleList;
                    if (samples.containsKey(metricName)) {
                        sampleList = samples.get(metricName);
                    } else {
                        sampleList = new ArrayList<>();
                        samples.put(metricName, sampleList);
                        metricFamilySamples.add(new MetricFamilySamples(metricName, Collector.Type.UNKNOWN, "", sampleList));
                    }
                    sampleList.add(new Sample(metricName, labels, labelValues, metric.getValue().doubleValue(), timeStamp));
                }
            }
            if (!packets.isEmpty()) {
                var firstPacket = packets.get(0);
                var statusMetricName = serviceName + "_status";
                // MetricsPacket status 0 means OK, but it's the opposite in Prometheus.
                var statusMetricValue = (firstPacket.statusCode == 0) ? 1 : 0;
                var sampleList = List.of(new Sample(statusMetricName, List.of(), List.of(),
                        statusMetricValue, firstPacket.timestamp * 1000));
                metricFamilySamples.add(new MetricFamilySamples(statusMetricName, Collector.Type.UNKNOWN, "status of service", sampleList));
            }
        }));

        return new PrometheusModel(metricFamilySamples);
    }

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.clustercontroller.core;

import com.yahoo.vdslib.distribution.ConfiguredNode;
import com.yahoo.vdslib.distribution.Distribution;
import com.yahoo.vespa.config.content.StorDistributionConfig;

import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class DistributionBuilder {
    // TODO support nested groups
    public static class GroupBuilder {
        final int groupCount;
        List<Integer> groupsWithNodeCount;

        public GroupBuilder(int groupCount) {
            this.groupCount = groupCount;
        }

        public GroupBuilder(int... nodeCounts) {
            this.groupCount = nodeCounts.length;
            this.groupsWithNodeCount = IntStream.of(nodeCounts).boxed()
                    .toList();
        }

        public GroupBuilder eachWithNodeCount(int nodeCount) {
            groupsWithNodeCount = IntStream.range(0, groupCount)
                    .map(i -> nodeCount).boxed()
                    .toList();
            return this;
        }

        public int totalNodeCount() {
            return groupsWithNodeCount.stream().reduce(0, Integer::sum);
        }

        public String groupDistributionSpec() {
            return IntStream.range(0, groupCount).mapToObj(i -> "1")
                    .collect(Collectors.joining("|")) + "|*";
        }
    }

    public static GroupBuilder withGroups(int groups) {
        return new GroupBuilder(groups);
    }

    public static GroupBuilder withGroupNodes(int... nodeCounts) {
        return new GroupBuilder(nodeCounts);
    }

    static List<ConfiguredNode> buildConfiguredNodes(int nodeCount) {
        return IntStream.range(0, nodeCount)
                .mapToObj(i -> new ConfiguredNode(i, false))
                .toList();
    }

    static List<ConfiguredNode> buildConfiguredNodes(Collection<Integer> nodes) {
        return nodes.stream().map(i -> new ConfiguredNode(i, false)).toList();
    }

    private static StorDistributionConfig.Group.Nodes.Builder configuredNode(ConfiguredNode node) {
        StorDistributionConfig.Group.Nodes.Builder builder = new StorDistributionConfig.Group.Nodes.Builder();
        builder.index(node.index());
        return builder;
    }

    private static StorDistributionConfig.Group.Builder configuredGroup(
            String name, String index, Collection<ConfiguredNode> nodes) {
        StorDistributionConfig.Group.Builder builder = new StorDistributionConfig.Group.Builder();
        builder.name(name);
        builder.index(index);
        nodes.forEach(n -> builder.nodes(configuredNode(n)));
        return builder;
    }

    private static StorDistributionConfig.Group.Builder configuredGroup(
            String name, int index, Collection<ConfiguredNode> nodes) {
        return configuredGroup(name, Integer.toString(index), nodes);
    }

    public static StorDistributionConfig configForFlatCluster(int redundancy, int searchableCopies, Collection<Integer> nodes) {
        StorDistributionConfig.Builder configBuilder = new StorDistributionConfig.Builder();
        configBuilder.redundancy(redundancy);
        configBuilder.ready_copies(searchableCopies);
        configBuilder.group(configuredGroup("invalid", "invalid", buildConfiguredNodes(nodes)));

        return new StorDistributionConfig(configBuilder);
    }

    public static StorDistributionConfig configForFlatCluster(int redundancy, int searchableCopies, int nodeCount) {
        return configForFlatCluster(redundancy, searchableCopies, IntStream.range(0, nodeCount).boxed().toList());
    }

    public static StorDistributionConfig configForFlatCluster(int nodeCount) {
        return configForFlatCluster(2, 0, nodeCount);
    }

    public static Distribution forFlatCluster(int nodeCount) {
        return new Distribution(configForFlatCluster(nodeCount));
    }

    public static StorDistributionConfig configForHierarchicCluster(GroupBuilder root) {
        List<ConfiguredNode> nodes = buildConfiguredNodes(root.totalNodeCount());

        StorDistributionConfig.Builder configBuilder = new StorDistributionConfig.Builder();
        configBuilder.redundancy(2);

        StorDistributionConfig.Group.Builder rootBuilder = new StorDistributionConfig.Group.Builder();
        rootBuilder.name("invalid");
        rootBuilder.index("invalid");
        rootBuilder.partitions(root.groupDistributionSpec());
        configBuilder.group(rootBuilder);

        int offset = 0;
        for (int group = 0; group < root.groupsWithNodeCount.size(); ++group) {
            int nodeCount = root.groupsWithNodeCount.get(group);
            StorDistributionConfig.Group.Builder groupBuilder
                    = configuredGroup("group_" + (group + 1), group + 1, nodes.subList(offset, offset + nodeCount));
            configBuilder.group(groupBuilder);
            offset += nodeCount;
        }

        return new StorDistributionConfig(configBuilder);
    }

    public static Distribution forHierarchicCluster(GroupBuilder root) {
        return new Distribution(configForHierarchicCluster(root));
    }
}

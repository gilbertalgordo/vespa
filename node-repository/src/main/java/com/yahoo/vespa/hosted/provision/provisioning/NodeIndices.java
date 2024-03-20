// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.provision.provisioning;

import com.yahoo.config.provision.ClusterSpec;
import com.yahoo.vespa.hosted.provision.NodeList;

import java.util.List;

/**
 * Tracks indices of a node cluster, and proposes the index of the next allocation.
 *
 * @author jonmv
 */
class NodeIndices {

    private final List<Integer> used;

    private int last;
    private int probe;

    /** Pass the list of current indices in the cluster. */
    NodeIndices(ClusterSpec.Id cluster, NodeList allNodes) {
        this(allNodes.cluster(cluster).mapToList(node -> node.allocation().get().membership().index()));
    }

    NodeIndices(List<Integer> used) {
        this.used = used;
        this.last = -1;
        this.probe = last;
    }

    /** Returns the next available index and commits to using it. Throws if a probe is ongoing. */
    int next() {
        if (probe != last)
            throw new IllegalStateException("Must commit ongoing probe before calling 'next'");

        probeNext();
        commitProbe();
        return last;
    }

    /** Returns the next available index, without committing to using it. Yields increasing indices when called multiple times. */
    int probeNext() {
        while (used.contains(++probe));
        return probe;
    }

    /** Commits to using all indices returned by an ongoing probe. */
    void commitProbe() {
        last = probe;
    }

    /** Resets any probed state to what's currently committed. */
    void resetProbe() {
        probe = last;
    }

}

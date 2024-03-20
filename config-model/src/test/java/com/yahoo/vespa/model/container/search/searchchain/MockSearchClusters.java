// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.container.search.searchchain;

import com.yahoo.config.model.deploy.DeployState;
import com.yahoo.search.config.SchemaInfoConfig;
import com.yahoo.vespa.config.search.AttributesConfig;
import com.yahoo.config.model.producer.AbstractConfigProducerRoot;
import com.yahoo.prelude.fastsearch.DocumentdbInfoConfig;
import com.yahoo.search.config.IndexInfoConfig;
import com.yahoo.vespa.configdefinition.IlscriptsConfig;
import com.yahoo.vespa.model.search.DocumentDatabase;
import com.yahoo.vespa.model.search.SearchCluster;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MockSearchClusters {

    private static class MockSearchCluster extends SearchCluster {

        public MockSearchCluster(AbstractConfigProducerRoot root, String clusterName,  boolean isStreaming) {
            super(root, clusterName);
            streaming = isStreaming;
        }

        private final boolean streaming;

        @Override
        public void deriveFromSchemas(DeployState deployState) { }
        @Override public List<DocumentDatabase> getDocumentDbs() { return List.of(); }
        @Override public void getConfig(AttributesConfig.Builder builder) {}
        @Override public void getConfig(DocumentdbInfoConfig.Builder builder) {}
        @Override public void getConfig(IndexInfoConfig.Builder builder) {}
        @Override public void getConfig(IlscriptsConfig.Builder builder) {}
        @Override public void getConfig(SchemaInfoConfig.Builder builder) {}

    }

    public static SearchCluster mockSearchCluster(AbstractConfigProducerRoot root, String clusterName, boolean isStreaming) {

        return new MockSearchCluster(root, clusterName, isStreaming);
    }

    public static Map<String, SearchCluster> twoMockClusterSpecsByName(AbstractConfigProducerRoot root) {
        Map<String, SearchCluster> result = new HashMap<>();
        result.put("cluster1", mockSearchCluster(root, "cluster1", false));
        result.put("cluster2", mockSearchCluster(root, "cluster2", true));
        return result;
    }
}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.schema;

import com.yahoo.container.QrSearchersConfig;
import com.yahoo.search.config.SchemaInfoConfig;

import java.util.ArrayList;
import java.util.List;

/**
 * Translation between schema info configuration and schema objects.
 *
 * @author bratseth
 */
class SchemaInfoConfigurer {

    static List<Schema> toSchemas(SchemaInfoConfig schemaInfoConfig) {
        return schemaInfoConfig.schema().stream().map(SchemaInfoConfigurer::toSchema).toList();
    }

    static Schema toSchema(SchemaInfoConfig.Schema schemaInfoConfig) {
        Schema.Builder builder = new Schema.Builder(schemaInfoConfig.name());

        for (var profileConfig : schemaInfoConfig.rankprofile()) {
            RankProfile.Builder profileBuilder = new RankProfile.Builder(profileConfig.name());
            profileBuilder.setHasSummaryFeatures(profileConfig.hasSummaryFeatures());
            profileBuilder.setHasRankFeatures(profileConfig.hasRankFeatures());
            for (var inputConfig : profileConfig.input())
                profileBuilder.addInput(inputConfig.name(), RankProfile.InputType.fromSpec(inputConfig.type()));
            builder.add(profileBuilder.build());
        }

        for (var summaryConfig : schemaInfoConfig.summaryclass()) {
            DocumentSummary.Builder summaryBuilder = new DocumentSummary.Builder(summaryConfig.name());
            for (var field : summaryConfig.fields()) {
                if (field.dynamic())
                    summaryBuilder.setDynamic(true);
                summaryBuilder.add(new DocumentSummary.Field(field.name(), field.type()));
            }
            builder.add(summaryBuilder.build());
        }

        return builder.build();
    }

    static List<Cluster> toClusters(QrSearchersConfig config) {
        List<Cluster> clusters = new ArrayList<>();
        for (var searchCluster : config.searchcluster()) {
            String clusterName = searchCluster.name();
            var clusterInfo = new Cluster.Builder(clusterName);
            clusterInfo.setStreaming(searchCluster.indexingmode() == QrSearchersConfig.Searchcluster.Indexingmode.Enum.STREAMING);
            for (var schemaDef : searchCluster.searchdef())
                clusterInfo.addSchema(schemaDef);
            clusters.add(clusterInfo.build());
        }
        return clusters;
    }

}

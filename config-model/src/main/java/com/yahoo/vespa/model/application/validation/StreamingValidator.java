// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation;

import com.yahoo.config.application.api.DeployLogger;
import com.yahoo.document.DataType;
import com.yahoo.document.NumericDataType;
import com.yahoo.document.TensorDataType;
import com.yahoo.documentmodel.NewDocumentReferenceDataType;
import com.yahoo.schema.Schema;
import com.yahoo.schema.derived.DerivedConfiguration;
import com.yahoo.schema.derived.SchemaInfo;
import com.yahoo.schema.document.Attribute;
import com.yahoo.schema.document.ImmutableSDField;
import com.yahoo.schema.document.MatchType;
import com.yahoo.vespa.model.application.validation.Validation.Context;
import com.yahoo.vespa.model.search.SearchCluster;

import java.util.List;
import java.util.logging.Level;

/**
 * Validates streaming mode
 */
public class StreamingValidator implements Validator {

    @Override
    public void validate(Context context) {
        List<SearchCluster> searchClusters = context.model().getSearchClusters();
        for (SearchCluster cluster : searchClusters) {
            for (SchemaInfo schemaInfo : cluster.schemas().values()) {
                if (schemaInfo.getIndexMode() == SchemaInfo.IndexMode.STREAMING) {
                    var deployLogger = context.deployState().getDeployLogger();
                    warnStreamingAttributes(cluster.getClusterName(), schemaInfo.fullSchema(), deployLogger);
                    warnStreamingIndexFields(cluster.getClusterName(), schemaInfo.fullSchema(), deployLogger);
                    warnStreamingGramMatching(cluster.getClusterName(), schemaInfo.fullSchema(), deployLogger);
                    failStreamingDocumentReferences(cluster.getClusterName(), cluster.getDocumentDB(schemaInfo.name()).getDerivedConfiguration(), context);
                }
            }
        }
    }

    private static void logWarning(String cluster, Schema schema, ImmutableSDField sd, DeployLogger logger, String message) {
        logger.logApplicationPackage(Level.WARNING, "For search cluster '" + cluster +
                "', streaming schema '" + schema.getName() + "', SD field '" + sd.getName() + "': " + message);
    }

    private static void warnStreamingGramMatching(String cluster, Schema schema, DeployLogger logger) {
        for (ImmutableSDField sd : schema.allConcreteFields()) {
            if (sd.getMatching().getType() == MatchType.GRAM) {
                logWarning(cluster, schema, sd, logger, "n-gram matching is not supported for streaming search.");
            }
        }
    }

    /**
     * Warn if one or more attributes are defined in a streaming search cluster SD.
     */
    private static void warnStreamingAttributes(String cluster, Schema schema, DeployLogger logger) {
        for (ImmutableSDField sd : schema.allConcreteFields()) {
            if (sd.doesAttributing()) {
                warnStreamingAttribute(cluster, schema, sd, logger);
            }
        }
    }

    private static void warnStreamingAttribute(String cluster, Schema schema, ImmutableSDField sd, DeployLogger logger) {
        // If the field is numeric, we can't print this, because we may have converted the field to
        // attribute indexing ourselves (IntegerIndex2Attribute)
        if (sd.getDataType() instanceof NumericDataType) return;
        // Tensor fields are only searchable via nearest neighbor search, and match semantics are irrelevant.
        if (sd.getDataType() instanceof TensorDataType) {
            for (var fieldAttribute : sd.getAttributes().values()) {
                if (fieldAttribute.hnswIndexParams().isPresent()) {
                    logWarning(cluster, schema, sd, logger,
                            "hnsw index is not relevant and not supported, ignoring setting");
                }
            }
            return;
        } else if (sd.getDataType() == DataType.PREDICATE) {
            logWarning(cluster, schema, sd, logger, "field type predicate is not supported for streaming search");
        }
    }

    private static void failStreamingDocumentReferences(String cluster, DerivedConfiguration derived, Context context) {
        for (Attribute attribute : derived.getAttributeFields().attributes()) {
            DataType dataType = attribute.getDataType();
            if (dataType instanceof NewDocumentReferenceDataType) {
                String errorMessage = String.format("For search cluster '%s', streaming schema '%s': Attribute '%s' has type '%s'. " +
                                                    "Document references and imported fields are not allowed in streaming search.",
                                                    cluster, derived.getSchema().getName(), attribute.getName(), dataType.getName());
                context.illegal(errorMessage);
            }
        }
    }

    private static void warnStreamingIndexFields(String cluster, Schema schema, DeployLogger logger) {
        for (ImmutableSDField sd : schema.allConcreteFields()) {
            if (sd.doesIndexing()) {
                warnStreamingIndexField(cluster, schema, sd, logger);
            }
        }
    }

    private static void warnStreamingIndexField(String cluster, Schema schema, ImmutableSDField sd, DeployLogger logger) {
        if (sd.getDataType() == DataType.URI) {
            logWarning(cluster, schema, sd, logger,
                    "field type uri is not supported for streaming search, it will be handled as a string field");

        }
    }
}

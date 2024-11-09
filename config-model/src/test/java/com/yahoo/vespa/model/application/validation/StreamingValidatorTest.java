// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation;

import com.yahoo.config.application.api.DeployLogger;
import com.yahoo.config.model.deploy.DeployState;
import com.yahoo.schema.derived.TestableDeployLogger;
import com.yahoo.vespa.model.VespaModel;
import com.yahoo.vespa.model.content.utils.ApplicationPackageBuilder;
import com.yahoo.vespa.model.content.utils.ContentClusterBuilder;
import com.yahoo.vespa.model.content.utils.DocType;
import com.yahoo.vespa.model.content.utils.SchemaBuilder;
import com.yahoo.vespa.model.test.utils.VespaModelCreatorWithFilePkg;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * @author bjorncs
 */
public class StreamingValidatorTest {

    @Test
    void document_references_are_forbidden_in_streaming_search() {
        Throwable exception = assertThrows(IllegalArgumentException.class, () -> {
            new VespaModelCreatorWithFilePkg("src/test/cfg/application/validation/document_references_validation/")
                    .create();
        });
        assertTrue(exception.getMessage().contains("For search cluster 'content', streaming schema 'ad': Attribute 'campaign_ref' has type 'Reference<campaign>'. " +
                "Document references and imported fields are not allowed in streaming search."));
    }

    private static List<String> filter(List<String> warnings) {
        return warnings.stream().filter(x -> x.indexOf("Cannot run program") == -1).toList();
    }

    @Test
    void tensor_field_without_index_gives_no_warning() {
        var logger = new TestableDeployLogger();
        var model = createModel(logger, "field nn type tensor(x[2]) { indexing: attribute | summary\n" +
                    "attribute { distance-metric: euclidean } }");
        assertTrue(filter(logger.warnings).isEmpty());
    }

    @Test
    void tensor_field_with_index_triggers_warning_in_streaming_search() {
        var logger = new TestableDeployLogger();
        var model = createModel(logger, "field nn type tensor(x[2]) { indexing: attribute | index | summary\n" +
                    "attribute { distance-metric: euclidean } }");
        var warnings = filter(logger.warnings);
        assertEquals(1, warnings.size());
        assertEquals("For search cluster 'content', streaming schema 'test', SD field 'nn': hnsw index is not relevant and not supported, ignoring setting",
                     warnings.get(0));
    }

    @Test
    void uriFieldWithIndexTriggersWarningInStreamingSearch() {
        var logger = new TestableDeployLogger();
        var model = createModel(logger, "field URI type uri { indexing: index | summary }");
        var warnings = filter(logger.warnings);
        assertEquals(1, warnings.size());
        assertEquals("For search cluster 'content', streaming schema 'test', SD field 'URI': " +
                        "field type uri is not supported for streaming search, it will be handled as a string field",
                warnings.get(0));
    }

    @Test
    void predicateFieldWithAttributeTriggersWarningInStreamingSearch() {
        var logger = new TestableDeployLogger();
        var model = createModel(logger, "field pred type predicate { indexing: attribute | summary }");
        var warnings = filter(logger.warnings);
        assertEquals(1, warnings.size());
        assertEquals("For search cluster 'content', streaming schema 'test', SD field 'pred': " +
                        "field type predicate is not supported for streaming search",
                warnings.get(0));

    }

    private static VespaModel createModel(DeployLogger logger, String sdContent) {
        var builder = new DeployState.Builder();
        builder.deployLogger(logger);
        return new ApplicationPackageBuilder()
                .addCluster(new ContentClusterBuilder().name("content").docTypes(List.of(DocType.streaming("test"))))
                .addSchemas(new SchemaBuilder().name("test").content(sdContent).build())
                .buildCreator().create(builder);
    }
}

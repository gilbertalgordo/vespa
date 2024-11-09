// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation.change.search;

import com.yahoo.documentmodel.NewDocumentType;
import com.yahoo.vespa.model.application.validation.change.VespaConfigChangeAction;
import com.yahoo.vespa.model.content.cluster.ContentCluster;
import com.yahoo.vespa.model.content.utils.ContentClusterBuilder;
import com.yahoo.vespa.model.content.utils.ContentClusterUtils;
import com.yahoo.vespa.model.content.utils.SchemaBuilder;
import com.yahoo.vespa.model.search.DocumentDatabase;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Test fixture to setup current and next content clusters used for change validation.
 *
 * @author geirst
 */
public abstract class ContentClusterFixture {

    protected ContentCluster currentCluster;
    protected ContentCluster nextCluster;

    public ContentClusterFixture(String currentSd, String nextSd) throws Exception {
        currentCluster = createCluster(currentSd);
        nextCluster = createCluster(nextSd);
    }

    protected ContentClusterFixture(ContentCluster currentCluster, ContentCluster nextCluster) {
        this.currentCluster = currentCluster;
        this.nextCluster = nextCluster;
    }

    public ContentClusterFixture(String entireSd) throws Exception {
        currentCluster = createClusterFromEntireSd(entireSd);
        nextCluster = createClusterFromEntireSd(entireSd);
    }

    protected static ContentCluster createClusterFromEntireSd(String sdContent) throws Exception {
        return new ContentClusterBuilder().build(ContentClusterUtils.createMockRoot(List.of(sdContent)));
    }

    private static ContentCluster createCluster(String sdContent) throws Exception {
        return new ContentClusterBuilder().build(
                ContentClusterUtils.createMockRoot(
                        List.of(new SchemaBuilder().content(sdContent).build())));
    }

    protected DocumentDatabase currentDb() {
        return currentCluster.getSearch().getSearchCluster().getDocumentDbs().get(0);
    }

    protected NewDocumentType currentDocType() {
        return currentCluster.getDocumentDefinitions().get("test");
    }

    protected DocumentDatabase nextDb() {
        return nextCluster.getSearch().getSearchCluster().getDocumentDbs().get(0);
    }

    protected NewDocumentType nextDocType() {
        return nextCluster.getDocumentDefinitions().get("test");
    }

    public void assertValidation() {
        List<VespaConfigChangeAction> act = validate();
        assertTrue(act.isEmpty());
    }

    public void assertValidation(VespaConfigChangeAction exp) {
        assertValidation(List.of(exp));
    }

    public void assertValidation(List<VespaConfigChangeAction> exp) {
        List<VespaConfigChangeAction> act = validate();
        assertEquals(exp, act);
    }

    public abstract List<VespaConfigChangeAction> validate();

}


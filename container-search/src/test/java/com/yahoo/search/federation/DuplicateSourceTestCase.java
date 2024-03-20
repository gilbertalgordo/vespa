// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.federation;

import com.yahoo.component.ComponentId;
import com.yahoo.component.chain.Chain;
import com.yahoo.search.Query;
import com.yahoo.search.Result;
import com.yahoo.search.Searcher;
import com.yahoo.search.federation.sourceref.SearchChainResolver;
import com.yahoo.search.searchchain.Execution;
import com.yahoo.search.searchchain.SearchChainRegistry;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;

/**
 * Tests that sources representing document types which resolve to the same actual source
 * are only included once.
 * 
 * @author bratseth
 */
public class DuplicateSourceTestCase {

    @Test
    void testDuplicateSource() {
        String chain1 = "chain1";
        String schema1 = "doc1", schema2 = "doc2";
        // Set up a single cluster and chain (chain1), containing a MockBackendSearcher and having 2 doc types (doc1, doc2)
        MockBackendSearcher mockBackendSearcher = new MockBackendSearcher();
        SearchChainRegistry searchChains = new SearchChainRegistry();
        searchChains.register(new Chain<>(chain1, mockBackendSearcher));
        SearchChainResolver resolver = new SearchChainResolver.Builder()
                .addSearchChain(new ComponentId(chain1), List.of(schema1, schema2))
                .build();
        FederationSearcher searcher = new FederationSearcher(new ComponentId("test"), resolver,
                                                             Map.of(schema1, List.of(chain1), schema2, List.of(chain1)));

        Result result = searcher.search(new Query("?query=test&sources=doc1%2cdoc2"),
                new Execution(Execution.Context.createContextStub(searchChains)));

        assertNull(result.hits().getError());
        assertEquals(1, mockBackendSearcher.getInvocationCount());
    }
    
    private static final class MockBackendSearcher extends Searcher {

        private final AtomicInteger invocationCount = new AtomicInteger(0);
        
        @Override
        public Result search(Query query, Execution execution) {
            invocationCount.incrementAndGet();
            return new Result(query);
        }
        
        public int getInvocationCount() { return invocationCount.get(); }

    }

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.cluster;

import com.yahoo.component.annotation.Inject;
import com.yahoo.component.ComponentId;
import com.yahoo.component.chain.dependencies.After;
import com.yahoo.component.provider.ComponentRegistry;
import com.yahoo.container.QrSearchersConfig;
import com.yahoo.container.core.documentapi.VespaDocumentAccess;
import com.yahoo.container.handler.VipStatus;
import com.yahoo.prelude.fastsearch.ClusterParams;
import com.yahoo.prelude.fastsearch.DocumentdbInfoConfig;
import com.yahoo.prelude.fastsearch.FastSearcher;
import com.yahoo.prelude.fastsearch.SummaryParameters;
import com.yahoo.prelude.fastsearch.VespaBackEndSearcher;
import com.yahoo.search.Query;
import com.yahoo.search.Result;
import com.yahoo.search.Searcher;
import com.yahoo.search.config.ClusterConfig;
import com.yahoo.search.dispatch.Dispatcher;
import com.yahoo.search.query.ParameterParser;
import com.yahoo.search.ranking.GlobalPhaseRanker;
import com.yahoo.search.result.ErrorMessage;
import com.yahoo.search.schema.Cluster;
import com.yahoo.search.schema.SchemaInfo;
import com.yahoo.search.searchchain.Execution;
import com.yahoo.vespa.streamingvisitors.StreamingSearcher;
import com.yahoo.yolean.Exceptions;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.FutureTask;
import java.util.concurrent.RejectedExecutionException;
import java.util.stream.Collectors;

import static com.yahoo.container.QrSearchersConfig.Searchcluster.Indexingmode.STREAMING;

/**
 * A searcher which forwards to a cluster of monitored native Vespa backends.
 *
 * @author bratseth
 * @author Steinar Knutsen
 * @author geirst
 */
@After("*")
public class ClusterSearcher extends Searcher {

    private final static long DEFAULT_MAX_QUERY_TIMEOUT = 600000L;
    private final static long DEFAULT_MAX_QUERY_CACHE_TIMEOUT = 10000L;

    private final String searchClusterName;

    // The set of document types contained in this search cluster
    private final Set<String> schemas;
    private final SchemaInfo schemaInfo;

    private final long maxQueryTimeout; // in milliseconds
    private final long maxQueryCacheTimeout; // in milliseconds

    private final VespaBackEndSearcher server;
    private final Executor executor;
    private final GlobalPhaseRanker globalPhaseRanker;

    @Inject
    public ClusterSearcher(ComponentId id,
                           Executor executor,
                           QrSearchersConfig qrsConfig,
                           ClusterConfig clusterConfig,
                           DocumentdbInfoConfig documentDbConfig,
                           SchemaInfo schemaInfo,
                           ComponentRegistry<Dispatcher> dispatchers,
                           GlobalPhaseRanker globalPhaseRanker,
                           VipStatus vipStatus,
                           VespaDocumentAccess access) {
        super(id);
        this.executor = executor;
        this.schemaInfo = schemaInfo;
        int searchClusterIndex = clusterConfig.clusterId();
        searchClusterName = clusterConfig.clusterName();
        QrSearchersConfig.Searchcluster searchClusterConfig = getSearchClusterConfigFromClusterName(qrsConfig, searchClusterName);
        this.globalPhaseRanker = globalPhaseRanker;
        schemas = new LinkedHashSet<>();

        maxQueryTimeout = ParameterParser.asMilliSeconds(clusterConfig.maxQueryTimeout(), DEFAULT_MAX_QUERY_TIMEOUT);
        maxQueryCacheTimeout = ParameterParser.asMilliSeconds(clusterConfig.maxQueryCacheTimeout(), DEFAULT_MAX_QUERY_CACHE_TIMEOUT);

        SummaryParameters docSumParams = new SummaryParameters(qrsConfig
                .com().yahoo().prelude().fastsearch().FastSearcher().docsum()
                .defaultclass());

        for (DocumentdbInfoConfig.Documentdb docDb : documentDbConfig.documentdb())
            schemas.add(docDb.name());

        String uniqueServerId = UUID.randomUUID().toString();
        if (searchClusterConfig.indexingmode() == STREAMING) {
            server = streamingCluster(uniqueServerId, searchClusterIndex,
                                      searchClusterConfig, docSumParams, documentDbConfig, schemaInfo, access);
            vipStatus.addToRotation(server.getName());
        } else {
            server = searchDispatch(searchClusterIndex, searchClusterName, uniqueServerId,
                                    docSumParams, documentDbConfig, schemaInfo, dispatchers);
        }
    }

    private static QrSearchersConfig.Searchcluster getSearchClusterConfigFromClusterName(QrSearchersConfig config, String name) {
        for (QrSearchersConfig.Searchcluster searchCluster : config.searchcluster()) {
            if (searchCluster.name().equals(name)) {
                return searchCluster;
            }
        }
        return null;
    }

    private static ClusterParams makeClusterParams(int searchclusterIndex) {
        return new ClusterParams("sc" + searchclusterIndex + ".num" + 0);
    }

    private static FastSearcher searchDispatch(int searchclusterIndex,
                                               String searchClusterName,
                                               String serverId,
                                               SummaryParameters docSumParams,
                                               DocumentdbInfoConfig documentdbInfoConfig,
                                               SchemaInfo schemaInfo,
                                               ComponentRegistry<Dispatcher> dispatchers) {
        ClusterParams clusterParams = makeClusterParams(searchclusterIndex);
        ComponentId dispatcherComponentId = new ComponentId("dispatcher." + searchClusterName);
        Dispatcher dispatcher = dispatchers.getComponent(dispatcherComponentId);
        if (dispatcher == null)
            throw new IllegalArgumentException("Configuration error: No dispatcher " + dispatcherComponentId +
                                               " is configured");
        return new FastSearcher(serverId, dispatcher, docSumParams, clusterParams, documentdbInfoConfig, schemaInfo);
    }

    private static StreamingSearcher streamingCluster(String serverId,
                                                      int searchclusterIndex,
                                                      QrSearchersConfig.Searchcluster searchClusterConfig,
                                                      SummaryParameters docSumParams,
                                                      DocumentdbInfoConfig documentdbInfoConfig,
                                                      SchemaInfo schemaInfo,
                                                      VespaDocumentAccess access) {
        if (searchClusterConfig.searchdef().size() != 1)
            throw new IllegalArgumentException("Streaming search clusters can only contain a single schema but got " +
                                               searchClusterConfig.searchdef());
        ClusterParams clusterParams = makeClusterParams(searchclusterIndex);
        StreamingSearcher searcher = new StreamingSearcher(access);
        searcher.setSearchClusterName(searchClusterConfig.rankprofiles_configid());
        searcher.setStorageClusterRouteSpec(searchClusterConfig.storagecluster().routespec());
        searcher.init(serverId, docSumParams, clusterParams, documentdbInfoConfig, schemaInfo);
        return searcher;
    }

    /** Do not use, for internal testing purposes only. **/
    ClusterSearcher(SchemaInfo schemaInfo, Set<String> schemas, VespaBackEndSearcher searcher, Executor executor) {
        this.schemaInfo = schemaInfo;
        searchClusterName = "testScenario";
        maxQueryTimeout = DEFAULT_MAX_QUERY_TIMEOUT;
        maxQueryCacheTimeout = DEFAULT_MAX_QUERY_CACHE_TIMEOUT;
        server = searcher;
        this.executor = executor;
        this.globalPhaseRanker = null;
        this.schemas = schemas;
    }

    /** Do not use, for internal testing purposes only. **/
    ClusterSearcher(SchemaInfo schemaInfo, Set<String> schemas) {
        this(schemaInfo, schemas, null, null);
    }

    @Override
    public Result search(Query query, Execution execution) {
        validateQueryTimeout(query);
        validateQueryCache(query);
        var searcher = server;
        if (searcher == null) {
            return new Result(query, ErrorMessage.createNoBackendsInService("Could not search"));
        }
        if (query.getTimeLeft() <= 0) {
            return new Result(query, ErrorMessage.createTimeout("No time left for searching"));
        }

        return doSearch(searcher, query);
    }

    @Override
    public void fill(com.yahoo.search.Result result, String summaryClass, Execution execution) {
        Query query = result.getQuery();

        VespaBackEndSearcher searcher = server;
        if (searcher != null) {
            if (query.getTimeLeft() > 0) {
                searcher.fill(result, summaryClass);
            } else {
                if (result.hits().getErrorHit() == null) {
                    result.hits().addError(ErrorMessage.createTimeout("No time left to get summaries, query timeout was " +
                                                                      query.getTimeout() + " ms"));
                }
            }
        } else {
            if (result.hits().getErrorHit() == null) {
                result.hits().addError(ErrorMessage.createNoBackendsInService("Could not fill result"));
            }
        }
    }

    private void validateQueryTimeout(Query query) {
        if (query.getTimeout() <= maxQueryTimeout) return;

        if (query.getTrace().isTraceable(2)) {
            query.trace("Query timeout (" + query.getTimeout() + " ms) > max query timeout (" +
                        maxQueryTimeout + " ms). Setting timeout to " + maxQueryTimeout + " ms.", 2);
        }
        query.setTimeout(maxQueryTimeout);
    }

    private void validateQueryCache(Query query) {
        if ( ! query.getRanking().getQueryCache() ) return;
        if (query.getTimeout() <= maxQueryCacheTimeout) return;

        if (query.getTrace().isTraceable(2)) {
            query.trace("Query timeout (" + query.getTimeout() + " ms) > max query cache timeout (" +
                        maxQueryCacheTimeout + " ms). Disabling query cache.", 2);
        }
        query.getRanking().setQueryCache(false);
    }

    private Result doSearch(VespaBackEndSearcher searcher, Query query) {
        if (schemas.size() > 1) {
            return searchMultipleDocumentTypes(searcher, query);
        } else {
            String schema = schemas.iterator().next();
            query.getModel().setRestrict(schema);
            return perSchemaSearch(searcher, schema, query);
        }
    }

    private Result perSchemaSearch(VespaBackEndSearcher searcher, String schema, Query query) {
        Set<String> restrict = query.getModel().getRestrict();
        if (restrict.size() != 1) {
            throw new IllegalStateException("perSchemaSearch must always be called with 1 schema, got: " + restrict.size());
        }
        int rerankCount = globalPhaseRanker != null ? globalPhaseRanker.getRerankCount(query, schema) : 0;
        boolean useGlobalPhase = rerankCount > 0;
        final int wantOffset = query.getOffset();
        final int wantHits = query.getHits();
        if (useGlobalPhase) {
            var error = globalPhaseRanker.validateNoSorting(query, schema).orElse(null);
            if (error != null) return new Result(query, error);
            int useHits = Math.max(wantOffset + wantHits, rerankCount);
            query.setOffset(0);
            query.setHits(useHits);
        }
        Result result = searcher.search(schema, query);
        if (useGlobalPhase) {
            globalPhaseRanker.rerankHits(query, result, schema);
            result.hits().trim(wantOffset, wantHits);
            query.setOffset(wantOffset);
            query.setHits(wantHits);
        }
        return result;
    }

    private static void processResult(Query query, FutureTask<Result> task, Result mergedResult) {
        try {
            Result result = task.get();
            mergedResult.mergeWith(result);
            mergedResult.hits().addAll(result.hits().asUnorderedHits());
        } catch (ExecutionException e) {
            mergedResult.hits().addError(ErrorMessage.createInternalServerError("Failed querying '" +
                                                                                query.getModel().getRestrict() + "': " +
                                                                                Exceptions.toMessageString(e),
                                                                                e));
        } catch (InterruptedException e) {
            mergedResult.hits().addError(ErrorMessage.createInternalServerError("Failed querying '" +
                                                                                query.getModel().getRestrict() + "': " +
                                                                                Exceptions.toMessageString(e)));
        }
    }

    private Result searchMultipleDocumentTypes(VespaBackEndSearcher searcher, Query query) {
        Set<String> schemas = resolveSchemas(query);
        Map<String, Query> schemaQueries = createQueries(query, schemas);
        if (schemaQueries.size() == 1) {
            var entry = schemaQueries.entrySet().iterator().next();
            return perSchemaSearch(searcher, entry.getKey(), entry.getValue());
        } else {
            Result mergedResult = new Result(query);
            List<FutureTask<Result>> pending = new ArrayList<>(schemaQueries.size());
            for (var entry : schemaQueries.entrySet()) {
                FutureTask<Result> task = new FutureTask<>(() -> perSchemaSearch(searcher, entry.getKey(), entry.getValue()));
                try {
                    executor.execute(task);
                    pending.add(task);
                } catch (RejectedExecutionException rej) {
                    task.run();
                    processResult(query, task, mergedResult);
                }
            }
            for (FutureTask<Result> task : pending) {
                processResult(query, task, mergedResult);
            }
            // Should we trim the merged result?
            if (query.getOffset() > 0 || query.getHits() < mergedResult.hits().size()) {
                if (mergedResult.getHitOrderer() != null) {
                    // Make sure we have the necessary data for sorting
                    searcher.fill(mergedResult, VespaBackEndSearcher.SORTABLE_ATTRIBUTES_SUMMARY_CLASS);
                }
                mergedResult.hits().trim(query.getOffset(), query.getHits());
                query.setOffset(0); // Needed when doing a trim
            }
            return mergedResult;
        }
    }

    private Set<String> resolveSourceSubset(Set<String> sources) {
        Set<String> candidates = new HashSet<>();
        for (String source : sources) {
            Cluster cluster = schemaInfo.clusters().get(source);
            if (cluster != null)
                candidates.addAll(cluster.schemas());
        }
        return (candidates.isEmpty() ? sources : candidates).stream()
                .filter(schemas::contains).collect(Collectors.toUnmodifiableSet());
    }

    Set<String> resolveSchemas(Query query) {
        Set<String> restrict = query.getModel().getRestrict();
        if (restrict == null || restrict.isEmpty()) {
            Set<String> sources = query.getModel().getSources();
            return (sources == null || sources.isEmpty())
                    ? schemas
                    : resolveSourceSubset(sources);
        } else {
            return filterValidDocumentTypes(restrict);
        }
    }

    private Set<String> filterValidDocumentTypes(Collection<String> restrict) {
        Set<String> retval = new LinkedHashSet<>();
        for (String docType : restrict) {
            if (docType != null && schemas.contains(docType)) {
                retval.add(docType);
            }
        }
        return retval;
    }

    private Map<String, Query> createQueries(Query query, Set<String> schemas) {
        query.getModel().getQueryTree(); // performance: parse query before cloning such that it is only done once
        if (schemas.size() == 1) {
            String schema = schemas.iterator().next();
            query.getModel().setRestrict(schema);
            return Map.of(schema, query);
        } else if ( ! schemas.isEmpty() ) {
            var schemaQueries = new HashMap<String, Query>();
            for (String schema : schemas) {
                Query q = query.clone();
                q.setOffset(0);
                q.setHits(query.getOffset() + query.getHits());
                q.getModel().setRestrict(schema);
                schemaQueries.put(schema, q);
            }
            return schemaQueries;
        }
        return Map.of();
    }

    @Override
    public void deconstruct() {
        if (server != null) {
            server.shutDown();
        }
    }

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.content.storagecluster;

import ai.vespa.metrics.StorageMetrics;
import com.yahoo.config.model.deploy.DeployState;
import com.yahoo.vespa.config.content.core.StorVisitorConfig;
import com.yahoo.vespa.config.content.StorFilestorConfig;
import com.yahoo.vespa.config.content.core.StorServerConfig;
import com.yahoo.vespa.config.content.PersistenceConfig;
import com.yahoo.metrics.MetricsmanagerConfig;
import com.yahoo.config.model.producer.AnyConfigProducer;
import com.yahoo.config.model.producer.TreeConfigProducer;
import com.yahoo.vespa.model.builder.xml.dom.VespaDomBuilder;
import com.yahoo.vespa.model.content.cluster.ContentCluster;
import com.yahoo.vespa.model.builder.xml.dom.ModelElement;
import com.yahoo.vespa.model.content.StorageNode;
import org.w3c.dom.Element;

/**
 * Represents configuration that is common to all storage nodes.
 */
public class StorageCluster extends TreeConfigProducer<StorageNode>
    implements StorServerConfig.Producer,
        StorFilestorConfig.Producer,
        StorVisitorConfig.Producer,
        PersistenceConfig.Producer,
        MetricsmanagerConfig.Producer
{
    public static class Builder extends VespaDomBuilder.DomConfigProducerBuilderBase<StorageCluster> {
        @Override
        protected StorageCluster doBuild(DeployState deployState, TreeConfigProducer<AnyConfigProducer> ancestor, Element producerSpec) {
            final ModelElement clusterElem = new ModelElement(producerSpec);
            final ContentCluster cluster = (ContentCluster)ancestor;

            return new StorageCluster(ancestor,
                                      ContentCluster.getClusterId(clusterElem),
                                      new FileStorProducer.Builder().build(deployState.getProperties(), cluster, clusterElem),
                                      new StorServerProducer.Builder().build(deployState.getProperties(), clusterElem),
                                      new StorVisitorProducer.Builder().build(clusterElem),
                                      new PersistenceProducer.Builder().build(clusterElem));
        }
    }

    private final String clusterName;
    private final FileStorProducer fileStorProducer;
    private final StorServerProducer storServerProducer;
    private final StorVisitorProducer storVisitorProducer;
    private final PersistenceProducer persistenceProducer;

    StorageCluster(TreeConfigProducer<?> parent,
                   String clusterName,
                   FileStorProducer fileStorProducer,
                   StorServerProducer storServerProducer,
                   StorVisitorProducer storVisitorProducer,
                   PersistenceProducer persistenceProducer) {
        super(parent, "storage");
        this.clusterName = clusterName;
        this.fileStorProducer = fileStorProducer;
        this.storServerProducer = storServerProducer;
        this.storVisitorProducer = storVisitorProducer;
        this.persistenceProducer = persistenceProducer;
    }

    @Override
    public void getConfig(MetricsmanagerConfig.Builder builder) {
        ContentCluster.getMetricBuilder("log", builder).
                addedmetrics("vds.filestor.allthreads.put").
                addedmetrics("vds.filestor.allthreads.get").
                addedmetrics("vds.filestor.allthreads.remove").
                addedmetrics("vds.filestor.allthreads.update").
                addedmetrics(StorageMetrics.VDS_DATASTORED_ALLDISKS_DOCS.baseName()).
                addedmetrics(StorageMetrics.VDS_DATASTORED_ALLDISKS_BYTES.baseName()).
                addedmetrics(StorageMetrics.VDS_FILESTOR_QUEUESIZE.baseName()).
                addedmetrics(StorageMetrics.VDS_FILESTOR_AVERAGEQUEUEWAIT.baseName()).
                addedmetrics(StorageMetrics.VDS_VISITOR_CV_QUEUEWAITTIME.baseName()).
                addedmetrics(StorageMetrics.VDS_VISITOR_ALLTHREADS_AVERAGEQUEUEWAIT.baseName()).
                addedmetrics(StorageMetrics.VDS_VISITOR_ALLTHREADS_AVERAGEVISITORLIFETIME.baseName()).
                addedmetrics(StorageMetrics.VDS_VISITOR_ALLTHREADS_CREATED.baseName());
    }

    public String getClusterName() {
        return clusterName;
    }

    @Override
    public void getConfig(StorServerConfig.Builder builder) {
        storServerProducer.getConfig(builder);
    }

    @Override
    public void getConfig(StorVisitorConfig.Builder builder) {
        storVisitorProducer.getConfig(builder);
    }

    @Override
    public void getConfig(PersistenceConfig.Builder builder) {
        persistenceProducer.getConfig(builder);
    }

    @Override
    public void getConfig(StorFilestorConfig.Builder builder) {
        fileStorProducer.getConfig(builder);
        storVisitorProducer.getConfig(builder);
    }

}

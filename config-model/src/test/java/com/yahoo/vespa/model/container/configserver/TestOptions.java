// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.container.configserver;

import com.yahoo.vespa.model.container.configserver.option.ConfigOptions;

import java.util.Optional;

/**
 * @author Ulf Lilleengen
 */
public class TestOptions implements ConfigOptions {

    private ConfigServer[] configServers = new ConfigServer[0];
    private int[] configServerZookeeperIds = new int[0];
    private Optional<Integer> rpcPort = Optional.empty();
    private Optional<String> environment = Optional.empty();
    private Optional<String> region = Optional.empty();
    private Optional<Boolean> useVespaVersionInRequest = Optional.empty();
    private Optional<Boolean> hostedVespa = Optional.empty();
    private static final String zooKeeperSnapshotMethod = "gz";

    @Override
    public Optional<Integer> rpcPort() {
        return rpcPort;
    }

    public TestOptions rpcPort(int port) {
        this.rpcPort = Optional.of(port);
        return this;
    }

    public TestOptions useVespaVersionInRequest(boolean useVespaVersionInRequest) {
        this.useVespaVersionInRequest = Optional.of(useVespaVersionInRequest);
        return this;
    }

    @Override
    public Optional<Boolean> multiTenant() { return Optional.empty();  }

    @Override
    public Optional<Boolean> hostedVespa() {
        return hostedVespa;
    }

    @Override
    public ConfigServer[] allConfigServers() {
        return configServers;
    }

    @Override
    public int[] configServerZookeeperIds() {
        return configServerZookeeperIds;
    }

    @Override
    public Optional<Long> zookeeperBarrierTimeout() {
        return Optional.empty();
    }

    @Override
    public Optional<String> environment() { return environment; }

    @Override
    public Optional<String> region() { return region; }

    @Override
    public Optional<String> system() { return Optional.empty(); }

    @Override
    public Optional<Boolean> useVespaVersionInRequest() { return useVespaVersionInRequest; }

    @Override
    public String zooKeeperSnapshotMethod() { return zooKeeperSnapshotMethod; }

    @Override
    public Integer zookeeperJuteMaxBuffer() { return 12345; }

    public TestOptions configServers(ConfigServer[] configServers) {
        this.configServers = configServers;
        return this;
    }

    public TestOptions configServerZookeeperIds(int[] configServerZookeeperIds) {
        this.configServerZookeeperIds = configServerZookeeperIds;
        return this;
    }

    public TestOptions environment(String environment) {
        this.environment = Optional.of(environment);
        return this;
    }

    public TestOptions region(String region) {
        this.region = Optional.of(region);
        return this;
    }

    public TestOptions hostedVespa(boolean hostedVespa) {
        this.hostedVespa = Optional.of(hostedVespa);
        return this;
    }

}

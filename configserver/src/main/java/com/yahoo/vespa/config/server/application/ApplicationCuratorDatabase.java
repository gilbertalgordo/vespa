// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.application;

import com.yahoo.config.provision.ApplicationId;
import com.yahoo.config.provision.TenantName;
import com.yahoo.path.Path;
import com.yahoo.slime.Cursor;
import com.yahoo.slime.Inspector;
import com.yahoo.slime.Slime;
import com.yahoo.slime.SlimeUtils;
import com.yahoo.text.Utf8;
import com.yahoo.transaction.Transaction;
import com.yahoo.vespa.config.server.application.ApplicationReindexing.Cluster;
import com.yahoo.vespa.config.server.application.ApplicationReindexing.Status;
import com.yahoo.vespa.config.server.tenant.TenantRepository;
import com.yahoo.vespa.curator.Curator;
import com.yahoo.vespa.curator.Lock;
import com.yahoo.vespa.curator.transaction.CuratorOperations;
import com.yahoo.vespa.curator.transaction.CuratorTransaction;
import com.yahoo.yolean.Exceptions;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.OptionalLong;
import java.util.concurrent.ExecutorService;
import java.util.function.UnaryOperator;

import static com.yahoo.vespa.curator.transaction.CuratorOperations.setData;
import static com.yahoo.yolean.Exceptions.uncheck;
import static java.util.stream.Collectors.toMap;
import static java.util.stream.Collectors.toUnmodifiableMap;

/**
 * Stores data and holds locks for the applications of a tenant, backed by a {@link Curator}.
 *
 * Each application is stored under /config/v2/tenants/&lt;tenant&gt;/applications/&lt;application&gt;,
 * the root contains the currently active session, if any. Children of this node may hold more data.
 * Locks for synchronising writes to these paths, and changes to the config of this application, are found
 * under /config/v2/tenants/&lt;tenant&gt;/locks/&lt;application&gt;.
 *
 * @author jonmv
 */
public class ApplicationCuratorDatabase {

    final TenantName tenant;
    final Path applicationsPath;
    final Path locksPath;

    private final Curator curator;

    public ApplicationCuratorDatabase(TenantName tenant, Curator curator) {
        this.tenant = tenant;
        this.applicationsPath = TenantRepository.getApplicationsPath(tenant);
        this.locksPath = TenantRepository.getLocksPath(tenant);
        this.curator = curator;
    }

    /** Returns the lock for changing the session status of the given application. */
    public Lock lock(ApplicationId id) {
        return curator.lock(lockPath(id), Duration.ofMinutes(1)); // These locks shouldn't be held for very long.
    }

    /** Reads, modifies and writes the application reindexing for this application, while holding its lock. */
    public void modifyReindexing(ApplicationId id, ApplicationReindexing emptyValue, UnaryOperator<ApplicationReindexing> modifications) {
        try (Lock lock = curator.lock(reindexingLockPath(id), Duration.ofMinutes(1))) {
            writeReindexingStatus(id, modifications.apply(readReindexingStatus(id).orElse(emptyValue)));
        }
    }

    public boolean exists(ApplicationId id) {
        return curator.exists(applicationPath(id));
    }

    /**
     * Creates a node for the given application, marking its existence.
     */
    public void createApplication(ApplicationId id) {
        if ( ! id.tenant().equals(tenant))
            throw new IllegalArgumentException("Cannot write application id '" + id + "' for tenant '" + tenant + "'");

        try (Lock lock = lock(id)) {
            if (curator.exists(applicationPath(id))) return;

            var applicationData = new ApplicationData(id, OptionalLong.empty(), OptionalLong.empty());
            curator.set(applicationPath(id), applicationData.toJson());
            modifyReindexing(id, ApplicationReindexing.empty(), UnaryOperator.identity());
        }
    }

    /**
     * Creates a node for the given application, marking its existence.
     */
    // TODO: Remove in Vespa 9
    public void createApplicationInOldFormat(ApplicationId id) {
        if (! id.tenant().equals(tenant))
            throw new IllegalArgumentException("Cannot write application id '" + id + "' for tenant '" + tenant + "'");

        try (Lock lock = lock(id)) {
            if (curator.exists(applicationPath(id))) return;

            curator.create(applicationPath(id));
            modifyReindexing(id, ApplicationReindexing.empty(), UnaryOperator.identity());
        }
    }

    /**
     * Returns a transaction which writes the given session id as the currently active for the given application.
     *
     * @param applicationId An {@link ApplicationId} that represents an active application.
     * @param sessionId     session id belonging to the application package for this application id.
     */
    public Transaction createWriteActiveTransaction(Transaction transaction, ApplicationId applicationId, long sessionId) {
        String path = applicationPath(applicationId).getAbsolute();
        return transaction.add(setData(path, new ApplicationData(applicationId, OptionalLong.of(sessionId), OptionalLong.of(sessionId)).toJson()));
    }

    /**
     * Returns a transaction which writes the given session id as the currently active for the given application.
     *
     * @param applicationId An {@link ApplicationId} that represents an active application.
     * @param sessionId session id belonging to the application package for this application id.
     */
    // TODO: Remove in Vespa 9
    public Transaction createWriteActiveTransactionInOldFormat(Transaction transaction, ApplicationId applicationId, long sessionId) {
        String path = applicationPath(applicationId).getAbsolute();
        return transaction.add(setData(path, Utf8.toAsciiBytes(sessionId)));
    }

    /**
     * Returns a transaction which writes the given session id as the currently active for the given application.
     *
     * @param applicationId An {@link ApplicationId} that represents an active application.
     * @param sessionId session id belonging to the application package for this application id.
     */
    public Transaction createWritePrepareTransaction(Transaction transaction,
                                                     ApplicationId applicationId,
                                                     long sessionId,
                                                     OptionalLong activeSessionId) {
        // Needs to read or be supplied current active session id, to avoid overwriting a newer session id.
        String path = applicationPath(applicationId).getAbsolute();
        return transaction.add(setData(path, new ApplicationData(applicationId, activeSessionId, OptionalLong.of(sessionId)).toJson()));
    }

    /**
     * Returns a transaction which deletes this application.
     */
    public CuratorTransaction createDeleteTransaction(ApplicationId applicationId) {
        return CuratorTransaction.from(CuratorOperations.deleteAll(applicationPath(applicationId).getAbsolute(), curator), curator);
    }

    /**
     * Returns the active session id for the given application.
     * Returns Optional.empty() if application not found or no active session exists.
     */
    public Optional<Long> activeSessionOf(ApplicationId id) {
        return applicationData(id).flatMap(ApplicationData::activeSession);
    }

    /**
     * Returns application data for the given application.
     * Returns Optional.empty() if application not found or no application data exists.
     */
    public Optional<ApplicationData> applicationData(ApplicationId id) {
        Optional<byte[]> data = curator.getData(applicationPath(id));
        if (data.isEmpty() || data.get().length == 0) return Optional.empty();

        try {
            return Optional.of(ApplicationData.fromBytes(data.get()));
        } catch (IllegalArgumentException e) {
            return applicationDataOldFormat(id);
        }
    }

    /**
     * Returns application data for the given application.
     * Returns Optional.empty() if application not found or no application data exists.
     */
    public Optional<ApplicationData> applicationDataOldFormat(ApplicationId id) {
        Optional<byte[]> data = curator.getData(applicationPath(id));
        if (data.isEmpty() || data.get().length == 0) return Optional.empty();

        return Optional.of(new ApplicationData(id,
                                               OptionalLong.of(data.map(bytes -> Long.parseLong(Utf8.toString(bytes))).get()),
                                               OptionalLong.empty()));
    }

    /**
     * List the active applications of a tenant in this config server.
     *
     * @return a list of {@link ApplicationId}s that are active.
     */
    public List<ApplicationId> activeApplications() {
        return curator.getChildren(applicationsPath).stream()
                      .sorted()
                      .map(ApplicationId::fromSerializedForm)
                      .filter(id -> activeSessionOf(id).isPresent())
                      .toList();
    }

    public PendingRestarts readPendingRestarts(ApplicationId id) {
        try (Lock lock = curator.lock(restartsLockPath(id), Duration.ofMinutes(1))) {
            return curator.getData(pendingRestartsPath(id))
                          .map(PendingRestartsSerializer::fromBytes)
                          .orElse(PendingRestarts.empty());
        }
    }

    public void modifyPendingRestarts(ApplicationId id, UnaryOperator<PendingRestarts> modification) {
        try (Lock lock = curator.lock(restartsLockPath(id), Duration.ofMinutes(1))) {
            PendingRestarts original = readPendingRestarts(id);
            PendingRestarts modified = modification.apply(original);
            if (original != modified) {
                if (modified.isEmpty())
                    curator.delete(pendingRestartsPath(id));
                else
                    curator.set(pendingRestartsPath(id), PendingRestartsSerializer.toBytes(modified));
            }
        }
    }

    public Optional<ApplicationReindexing> readReindexingStatus(ApplicationId id) {
        return curator.getData(reindexingDataPath(id))
                      .map(ReindexingStatusSerializer::fromBytes);
    }

    void writeReindexingStatus(ApplicationId id, ApplicationReindexing status) {
        curator.set(reindexingDataPath(id), ReindexingStatusSerializer.toBytes(status));
    }

    /** Sets up a listenable cache with the given listener, over the applications path of this tenant. */
    public Curator.DirectoryCache createApplicationsPathCache(ExecutorService zkCacheExecutor) {
        return curator.createDirectoryCache(applicationsPath.getAbsolute(), false, false, zkCacheExecutor);
    }

    private Path restartsLockPath(ApplicationId id) {
        return locksPath.append(id.serializedForm() + "::restarts");
    }

    private Path reindexingLockPath(ApplicationId id) {
        return locksPath.append(id.serializedForm()).append("reindexing");
    }

    private Path lockPath(ApplicationId id) {
        return locksPath.append(id.serializedForm());
    }

    private Path applicationPath(ApplicationId id) {
        return applicationsPath.append(id.serializedForm());
    }

    private Path reindexingDataPath(ApplicationId id) {
        return applicationPath(id).append("reindexing");
    }

    private Path pendingRestartsPath(ApplicationId id) {
        return applicationPath(id).append("restarts");
    }

    private static class PendingRestartsSerializer {

        private static final String GENERATIONS = "generations";
        private static final String GENERATION = "generation";
        private static final String HOSTNAMES = "hostnames";

        private static byte[] toBytes(PendingRestarts pendingRestarts) {
            Cursor root = new Slime().setObject();
            Cursor generationsArray = root.setArray(GENERATIONS);
            pendingRestarts.generationsForRestarts().forEach((generation, hostnames) -> {
                Cursor generationObject = generationsArray.addObject();
                generationObject.setLong(GENERATION, generation);
                hostnames.forEach(generationObject.setArray(HOSTNAMES)::addString);
            });
            return uncheck(() -> SlimeUtils.toJsonBytes(root));
        }

        private static PendingRestarts fromBytes(byte[] data) {
            Cursor root = SlimeUtils.jsonToSlimeOrThrow(data).get();
            return new PendingRestarts(SlimeUtils.entriesStream(root.field(GENERATIONS))
                                                 .collect(toMap(entry -> entry.field(GENERATION).asLong(),
                                                                entry -> SlimeUtils.entriesStream(entry.field(HOSTNAMES))
                                                                                   .map(Inspector::asString)
                                                                                   .toList())));
        }

    }

    private static class ReindexingStatusSerializer {

        private static final String ENABLED = "enabled";
        private static final String CLUSTERS = "clusters";
        private static final String PENDING = "pending";
        private static final String READY = "ready";
        private static final String TYPE = "type";
        private static final String NAME = "name";
        private static final String GENERATION = "generation";
        private static final String EPOCH_MILLIS = "epochMillis";
        private static final String SPEED = "speed";
        private static final String CAUSE = "cause";

        private static byte[] toBytes(ApplicationReindexing reindexing) {
            Cursor root = new Slime().setObject();
            root.setBool(ENABLED, reindexing.enabled());

            Cursor clustersArray = root.setArray(CLUSTERS);
            reindexing.clusters().forEach((name, cluster) -> {
                Cursor clusterObject = clustersArray.addObject();
                clusterObject.setString(NAME, name);

                Cursor pendingArray = clusterObject.setArray(PENDING);
                cluster.pending().forEach((type, generation) -> {
                    Cursor pendingObject =  pendingArray.addObject();
                    pendingObject.setString(TYPE, type);
                    pendingObject.setLong(GENERATION, generation);
                });

                Cursor readyArray = clusterObject.setArray(READY);
                cluster.ready().forEach((type, status) -> {
                    Cursor statusObject = readyArray.addObject();
                    statusObject.setString(TYPE, type);
                    setStatus(statusObject, status);
                });
            });
            return uncheck(() -> SlimeUtils.toJsonBytes(root));
        }

        private static void setStatus(Cursor statusObject, Status status) {
            statusObject.setLong(EPOCH_MILLIS, status.ready().toEpochMilli());
            statusObject.setDouble(SPEED, status.speed());
            statusObject.setString(CAUSE, status.cause());
        }

        private static ApplicationReindexing fromBytes(byte[] data) {
            Cursor root = SlimeUtils.jsonToSlimeOrThrow(data).get();
            return new ApplicationReindexing(root.field(ENABLED).valid() ? root.field(ENABLED).asBool() : true,
                                             SlimeUtils.entriesStream(root.field(CLUSTERS))
                                                       .collect(toUnmodifiableMap(object -> object.field(NAME).asString(),
                                                                                  object -> getCluster(object))));
        }

        private static Cluster getCluster(Inspector object) {
            return new Cluster(SlimeUtils.entriesStream(object.field(PENDING))
                                         .collect(toUnmodifiableMap(entry -> entry.field(TYPE).asString(),
                                                                    entry -> entry.field(GENERATION).asLong())),
                               SlimeUtils.entriesStream(object.field(READY))
                                         .collect(toUnmodifiableMap(entry -> entry.field(TYPE).asString(),
                                                                    entry -> getStatus(entry))));
        }

        private static Status getStatus(Inspector statusObject) {
            return new Status(Instant.ofEpochMilli(statusObject.field(EPOCH_MILLIS).asLong()),
                              statusObject.field(SPEED).valid() ? statusObject.field(SPEED).asDouble() : 0.2,
                              statusObject.field(CAUSE).asString());
        }

    }

}

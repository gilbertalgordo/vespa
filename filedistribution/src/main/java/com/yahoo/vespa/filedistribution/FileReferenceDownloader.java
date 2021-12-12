// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.filedistribution;

import com.yahoo.concurrent.DaemonThreadFactory;
import com.yahoo.config.FileReference;
import com.yahoo.jrt.Int32Value;
import com.yahoo.jrt.Request;
import com.yahoo.jrt.StringValue;
import com.yahoo.vespa.config.Connection;
import com.yahoo.vespa.config.ConnectionPool;

import java.io.File;
import java.time.Duration;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Downloads file reference using rpc requests to config server and keeps track of files being downloaded
 *
 * @author hmusum
 */
public class FileReferenceDownloader {

    private final static Logger log = Logger.getLogger(FileReferenceDownloader.class.getName());

    private final ExecutorService downloadExecutor =
            Executors.newFixedThreadPool(Math.max(8, Runtime.getRuntime().availableProcessors()),
                                         new DaemonThreadFactory("filereference downloader"));
    private final ConnectionPool connectionPool;
    private final Downloads downloads;
    private final Duration downloadTimeout;
    private final Duration sleepBetweenRetries;
    private final Duration rpcTimeout;
    private final File downloadDirectory;

    FileReferenceDownloader(ConnectionPool connectionPool,
                            Downloads downloads,
                            Duration timeout,
                            Duration sleepBetweenRetries,
                            File downloadDirectory) {
        this.connectionPool = connectionPool;
        this.downloads = downloads;
        this.downloadTimeout = timeout;
        this.sleepBetweenRetries = sleepBetweenRetries;
        this.downloadDirectory = downloadDirectory;
        String timeoutString = System.getenv("VESPA_CONFIGPROXY_FILEDOWNLOAD_RPC_TIMEOUT");
        this.rpcTimeout = Duration.ofSeconds(timeoutString == null ? 30 : Integer.parseInt(timeoutString));
    }

    private void waitUntilDownloadStarted(FileReferenceDownload fileReferenceDownload) {
        FileReference fileReference = fileReferenceDownload.fileReference();
        int retryCount = 0;
        Connection connection = connectionPool.getCurrent();
        do {
            backoff(retryCount);

            if (FileDownloader.fileReferenceExists(fileReference, downloadDirectory))
                return;
            if (startDownloadRpc(fileReferenceDownload, retryCount, connection))
                return;

            retryCount++;
            // There is no one connection that will always work for each file reference (each file reference might
            // exist on just one config server, and which one could be different for each file reference), so we
            // should get a new connection for every retry
            connection = connectionPool.switchConnection(connection);
        } while (retryCount < 5);

        fileReferenceDownload.future().completeExceptionally(new RuntimeException("Failed getting " + fileReference));
        downloads.remove(fileReference);
    }

    private void backoff(int retryCount) {
        if (retryCount > 0) {
            try {
                long sleepTime = Math.min(120_000, (long) (Math.pow(2, retryCount)) * sleepBetweenRetries.toMillis());
                Thread.sleep(sleepTime);
            } catch (InterruptedException e) {
                /* ignored */
            }
        }
    }

    Future<Optional<File>> startDownload(FileReferenceDownload fileReferenceDownload) {
        FileReference fileReference = fileReferenceDownload.fileReference();
        Optional<FileReferenceDownload> inProgress = downloads.get(fileReference);
        if (inProgress.isPresent()) return inProgress.get().future();

        log.log(Level.FINE, () -> "Will download " + fileReference + " with timeout " + downloadTimeout);
        downloads.add(fileReferenceDownload);
        downloadExecutor.submit(() -> waitUntilDownloadStarted(fileReferenceDownload));
        return fileReferenceDownload.future();
    }

    void failedDownloading(FileReference fileReference) {
        downloads.remove(fileReference);
    }

    private boolean startDownloadRpc(FileReferenceDownload fileReferenceDownload, int retryCount, Connection connection) {
        Request request = createRequest(fileReferenceDownload);
        Duration rpcTimeout = rpcTimeout(retryCount);
        connection.invokeSync(request, rpcTimeout.getSeconds());

        Level logLevel = (retryCount > 3 ? Level.INFO : Level.FINE);
        FileReference fileReference = fileReferenceDownload.fileReference();
        if (validateResponse(request)) {
            log.log(Level.FINE, () -> "Request callback, OK. Req: " + request + "\nSpec: " + connection);
            if (request.returnValues().get(0).asInt32() == 0) {
                log.log(Level.FINE, () -> "Found " + fileReference + " available at " + connection.getAddress());
                return true;
            } else {
                log.log(logLevel, fileReference + " not found at " + connection.getAddress());
                return false;
            }
        } else {
            log.log(logLevel, "Downloading " + fileReference + " from " + connection.getAddress() + " failed: " +
                    request + ", error: " + request.errorCode() + "(" + request.errorMessage() +
                    "). Will switch config server for next request" +
                    " (retry " + retryCount + ", rpc timeout " + rpcTimeout + ")");
            return false;
        }
    }

    private Request createRequest(FileReferenceDownload fileReferenceDownload) {
        Request request = new Request("filedistribution.serveFile");
        request.parameters().add(new StringValue(fileReferenceDownload.fileReference().value()));
        request.parameters().add(new Int32Value(fileReferenceDownload.downloadFromOtherSourceIfNotFound() ? 0 : 1));
        return request;
    }

    private Duration rpcTimeout(int retryCount) {
        return Duration.ofSeconds(rpcTimeout.getSeconds()).plus(Duration.ofSeconds(retryCount * 10L));
    }

    private boolean validateResponse(Request request) {
        if (request.isError()) {
            return false;
        } else if (request.returnValues().size() == 0) {
            return false;
        } else if (!request.checkReturnTypes("is")) { // TODO: Do not hard-code return type
            log.log(Level.WARNING, "Invalid return types for response: " + request.errorMessage());
            return false;
        }
        return true;
    }

    public void close() {
        downloadExecutor.shutdown();
        try {
            downloadExecutor.awaitTermination(1, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Thread.interrupted(); // Ignore and continue shutdown.
        }
    }

}

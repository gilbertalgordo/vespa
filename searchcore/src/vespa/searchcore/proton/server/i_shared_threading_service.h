// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <atomic>
#include <vespa/vespalib/util/time.h>

class FNET_Transport;

namespace storage::spi { struct BucketExecutor; }

namespace vespalib {
class ISequencedTaskExecutor;
class ThreadExecutor;
class InvokeService;
}

namespace proton {

/**
 * Interface containing the thread executors that are shared across all document dbs.
 */
class ISharedThreadingService {
public:
    virtual ~ISharedThreadingService() = default;

    /**
     * Returns the shared executor used for various assisting tasks in a document db.
     *
     * Example usages include:
     *   - Disk index fusion.
     *   - Updating nearest neighbor index (in DenseTensorAttribute).
     *   - Loading nearest neighbor index (in DenseTensorAttribute).
     *   - Writing of data in the document store.
     */
    virtual vespalib::ThreadExecutor& shared() = 0;

    /**
     * Returns the sequenced executor used to write index and attribute fields in a document db.
     */
    virtual vespalib::ISequencedTaskExecutor& field_writer() = 0;

    /**
     * Returns an InvokeService intended for regular wakeup calls.
     */
    virtual vespalib::InvokeService & invokeService() = 0;

    /**
     * Returns a shared transport object that can be utilized by multiple services.
     */
    virtual FNET_Transport & transport() = 0;


    /**
     * Returns the executor for running a BucketTask in the persistence layer above the SPI.
     */
    virtual storage::spi::BucketExecutor& bucket_executor() = 0;

    /**
     * Return a very cheap clock.
     */
    virtual const std::atomic<vespalib::steady_time> & nowRef() const = 0;
};

}


// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "transport_helper.h"
#include <vespa/persistence/dummyimpl/dummy_bucket_executor.h>
#include <vespa/searchcore/proton/server/i_shared_threading_service.h>
#include <vespa/vespalib/util/invokeserviceimpl.h>

namespace proton {

class MockSharedThreadingService : public ISharedThreadingService {
private:
    using ThreadExecutor = vespalib::ThreadExecutor;
    ThreadExecutor              & _shared;
    std::unique_ptr<vespalib::ISequencedTaskExecutor> _field_writer;
    vespalib::InvokeServiceImpl   _invokeService;
    Transport                     _transport;
    storage::spi::dummy::DummyBucketExecutor _bucket_executor;
public:
    MockSharedThreadingService(ThreadExecutor& shared_in, size_t num_bucket_executors = 2);
    ~MockSharedThreadingService() override;
    ThreadExecutor& shared() override { return _shared; }
    vespalib::ISequencedTaskExecutor& field_writer() override { return *_field_writer; }
    vespalib::InvokeService & invokeService() override { return _invokeService; }
    FNET_Transport & transport() override { return _transport.transport(); }
    storage::spi::BucketExecutor& bucket_executor() override { return _bucket_executor; }
    const std::atomic<vespalib::steady_time> & nowRef() const override { return _invokeService.nowRef(); }
};

}

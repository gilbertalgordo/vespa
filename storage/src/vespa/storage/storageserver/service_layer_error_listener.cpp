// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "service_layer_error_listener.h"
#include <vespa/storage/common/storagecomponent.h>
#include <vespa/storage/storageserver/mergethrottler.h>

#include <vespa/log/log.h>
LOG_SETUP(".node.errorlistener");

namespace storage {

void ServiceLayerErrorListener::on_fatal_error(std::string_view message) {
    bool expected = false;
    if (_shutdown_initiated.compare_exchange_strong(expected, true)) {
        LOG(info,
            "Received FATAL_ERROR from persistence provider, "
            "shutting down node: %s",
            std::string(message).c_str());
        _component.requestShutdown(message); // Thread safe
    } else {
        LOG(debug,
            "Received FATAL_ERROR from persistence provider: %s. "
            "Node has already been instructed to shut down so "
            "not doing anything now.",
            std::string(message).c_str());
    }
}

void ServiceLayerErrorListener::on_resource_exhaustion_error(std::string_view message) {
    LOG(debug, "SPI reports resource exhaustion ('%s'). "
                "Applying back-pressure to merge throttler",
        std::string(message).c_str());
    _merge_throttler.apply_timed_backpressure(); // Thread safe
}

}

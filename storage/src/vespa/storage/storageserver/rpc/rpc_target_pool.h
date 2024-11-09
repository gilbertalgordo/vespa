// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace storage::rpc {

class RpcTarget;

/**
 * A pool of RPC targets used for a single node endpoint.
 *
 * The bucket id associated with a message is used to select the RPC target.
 * This ensures the same RPC target is used for all messages to the same bucket to the same node,
 * and the RPC target itself handles sequencing of these messages.
 */
class RpcTargetPool {
public:
    using RpcTargetVector = std::vector<std::shared_ptr<RpcTarget>>;

private:
    RpcTargetVector _targets;
    const std::string _spec;
    uint32_t _slobrok_gen;

public:
    RpcTargetPool(RpcTargetVector&& targets, const std::string& spec, uint32_t slobrok_gen);
    const std::string& spec() const { return _spec; }
    uint32_t slobrok_gen() const { return _slobrok_gen; }
    void update_slobrok_gen(uint32_t curr_slobrok_gen) { _slobrok_gen = curr_slobrok_gen; }
    std::shared_ptr<RpcTarget> get_target(uint64_t bucket_id) const;
};

}

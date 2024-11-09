// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/storageapi/messageapi/storagecommand.h>
#include <vespa/storageapi/messageapi/storagereply.h>
#include <vespa/vdslib/state/nodestate.h>
#include <vespa/vdslib/state/cluster_state_bundle.h>

namespace storage::api {

class GetNodeStateCommand : public StorageCommand {
    lib::NodeState::UP _expectedState;

public:
    explicit GetNodeStateCommand(lib::NodeState::UP expectedState);

    const lib::NodeState* getExpectedState() const { return _expectedState.get(); }
    void print(std::ostream& out, bool verbose, const std::string& indent) const override;

    DECLARE_STORAGECOMMAND(GetNodeStateCommand, onGetNodeState)
};

class GetNodeStateReply : public StorageReply {
    lib::NodeState::UP _state;
    std::string _nodeInfo;

public:
    GetNodeStateReply(const GetNodeStateCommand&); // Only used on makeReply()
    GetNodeStateReply(const GetNodeStateCommand&, const lib::NodeState&);

    bool hasNodeState() const { return (_state.get() != 0); }
    const lib::NodeState& getNodeState() const { return *_state; }

    void print(std::ostream& out, bool verbose, const std::string& indent) const override;

    void setNodeInfo(const std::string& info) { _nodeInfo = info; }
    const std::string& getNodeInfo() const { return _nodeInfo; }

    DECLARE_STORAGEREPLY(GetNodeStateReply, onGetNodeStateReply)
};

/**
 * Command for telling a node about the cluster state - state of each node
 * in the cluster and state of the cluster itself (all ok, no merging, block
 * put/get/remove etx)
 */
class SetSystemStateCommand : public StorageCommand {
    std::shared_ptr<const lib::ClusterStateBundle> _state;

public:
    explicit SetSystemStateCommand(std::shared_ptr<const lib::ClusterStateBundle> state);
    explicit SetSystemStateCommand(const lib::ClusterStateBundle &state);
    explicit SetSystemStateCommand(const lib::ClusterState &state);
    ~SetSystemStateCommand() override;

    [[nodiscard]] const lib::ClusterState& getSystemState() const { return *_state->getBaselineClusterState(); }
    [[nodiscard]] const lib::ClusterStateBundle& getClusterStateBundle() const { return *_state; }
    [[nodiscard]] std::shared_ptr<const lib::ClusterStateBundle> cluster_state_bundle_ptr() const noexcept {
        return _state;
    }
    void print(std::ostream& out, bool verbose, const std::string& indent) const override;

    DECLARE_STORAGECOMMAND(SetSystemStateCommand, onSetSystemState)
};

class SetSystemStateReply : public StorageReply {
    std::shared_ptr<const lib::ClusterStateBundle> _state;

public:
    explicit SetSystemStateReply(const SetSystemStateCommand& cmd);

    // Not serialized. Available locally
    const lib::ClusterState& getSystemState() const { return *_state->getBaselineClusterState(); }
    const lib::ClusterStateBundle& getClusterStateBundle() const { return *_state; }
    void print(std::ostream& out, bool verbose, const std::string& indent) const override;

    DECLARE_STORAGEREPLY(SetSystemStateReply, onSetSystemStateReply)
};

class ActivateClusterStateVersionCommand : public StorageCommand {
    uint32_t _version;
public:
    explicit ActivateClusterStateVersionCommand(uint32_t version);
    uint32_t version() const noexcept { return _version; }
    void print(std::ostream& out, bool verbose, const std::string& indent) const override;

    DECLARE_STORAGECOMMAND(ActivateClusterStateVersionCommand, onActivateClusterStateVersion);
};

class ActivateClusterStateVersionReply : public StorageReply {
    uint32_t _activateVersion;
    uint32_t _actualVersion;
public:
    explicit ActivateClusterStateVersionReply(const ActivateClusterStateVersionCommand&);
    uint32_t activateVersion() const noexcept { return _activateVersion; }
    void setActualVersion(uint32_t version) noexcept { _actualVersion = version; }
    uint32_t actualVersion() const noexcept { return _actualVersion; }
    void print(std::ostream& out, bool verbose, const std::string& indent) const override;

    DECLARE_STORAGEREPLY(ActivateClusterStateVersionReply, onActivateClusterStateVersionReply);
};

}

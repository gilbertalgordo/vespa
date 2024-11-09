// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "bucketinfocommand.h"

namespace storage::api {

class MaintenanceCommand : public BucketInfoCommand
{
public:
    MaintenanceCommand(const MessageType& type, const document::Bucket &bucket)
        : BucketInfoCommand(type, bucket)
    {}
    MaintenanceCommand(const MaintenanceCommand &) = default;
    MaintenanceCommand(MaintenanceCommand &&) noexcept = default;
    MaintenanceCommand & operator = (const MaintenanceCommand &) = delete;
    MaintenanceCommand & operator = (MaintenanceCommand &&) noexcept = delete;
    ~MaintenanceCommand() override;

    const std::string& getReason() const { return _reason; };
    void setReason(std::string_view reason) { _reason = reason; };
protected:
    std::string _reason;
};

}

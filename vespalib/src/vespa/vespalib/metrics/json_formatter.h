// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "snapshots.h"
#include <vespa/vespalib/data/slime/slime.h>
#include <string>

namespace vespalib::metrics {

/**
 * utility for converting a snapshot to JSON format
 * (which can be inserted into /state/v1/metrics page).
 **/
class JsonFormatter
{
private:
    using Cursor = vespalib::slime::Cursor;
    vespalib::Slime _data;
    Cursor& _top;
    double _snapLen;

    void handle(const Snapshot &snapshot,        Cursor &target);
    void handle(const PointSnapshot &snapshot,   Cursor &target);
    void handle(const CounterSnapshot &snapshot, Cursor &target);
    void handle(const GaugeSnapshot &snapshot,   Cursor &target);
public:
    explicit JsonFormatter(const Snapshot &snapshot);

    [[nodiscard]] std::string asString() const {
        return _data.toString();
    }
};

} // namespace vespalib::metrics

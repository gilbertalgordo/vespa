// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "indexmaintainer.h"
#include <vespa/searchcorespi/flush/iflushtarget.h>

namespace searchcorespi::index {

/**
 * Flush target for flushing a memory index in an IndexMaintainer.
 **/
class IndexFlushTarget : public IFlushTarget {
private:
    IndexMaintainer &_indexMaintainer;
    IndexMaintainer::FlushStats _flushStats;
    uint32_t _numFrozenMemoryIndexes;
    uint32_t _maxFrozenMemoryIndexes;
    FlushStats _lastStats;

public:
    IndexFlushTarget(IndexMaintainer &indexMaintainer);
    ~IndexFlushTarget();

    // Implements IFlushTarget
    MemoryGain getApproxMemoryGain() const override;
    DiskGain   getApproxDiskGain() const override;
    SerialNum getFlushedSerialNum() const override;
    Time    getLastFlushTime() const override;

    bool needUrgentFlush() const override;

    Task::UP initFlush(SerialNum currentSerial, std::shared_ptr<search::IFlushToken> flush_token) override;
    FlushStats getLastFlushStats() const override { return _lastStats; }
    uint64_t getApproxBytesToWriteToDisk() const override;
};

}

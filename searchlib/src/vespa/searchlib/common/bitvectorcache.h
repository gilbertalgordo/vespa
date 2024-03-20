// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "condensedbitvectors.h"
#include <vespa/vespalib/stllike/hash_set.h>
#include <vespa/vespalib/stllike/hash_map.h>
#include <shared_mutex>

namespace search {

class PopulateInterface
{
public:
    class Iterator {
    public:
        using UP = std::unique_ptr<Iterator>;
        virtual ~Iterator() = default;
        virtual int32_t getNext() = 0;
    };
    virtual ~PopulateInterface() = default;
    virtual Iterator::UP lookup(uint64_t key) const = 0;
};

class BitVectorCache
{
public:
    using Key = uint64_t;
    using KeySet = vespalib::hash_set<Key>;
    using KeyAndCountSet = std::vector<std::pair<Key, size_t>>;
    using CountVector = CondensedBitVector::CountVector;
    using GenerationHolder = vespalib::GenerationHolder;

    explicit BitVectorCache(GenerationHolder &genHolder);
    ~BitVectorCache();
    void computeCountVector(KeySet & keys, CountVector & v) const;
    KeySet lookupCachedSet(const KeyAndCountSet & keys);
    void set(Key key, uint32_t index, bool v);
    bool get(Key key, uint32_t index) const;
    void removeIndex(uint32_t index);
    void adjustDocIdLimit(uint32_t docId);
    void populate(uint32_t count, const PopulateInterface &);
    bool needPopulation() const { return _needPopulation.load(std::memory_order_relaxed); }
    void requirePopulation() { _needPopulation = true; }
private:
    class KeyMeta {
    public:
        KeyMeta() noexcept
            : _lookupCount(0),
              _bitCount(0),
              _chunkId(-1),
              _chunkIndex(0)
        { }
        KeyMeta(const KeyMeta & rhs) noexcept
            : _lookupCount(rhs.lookupCount()),
              _bitCount(rhs._bitCount),
              _chunkId(rhs._chunkId),
              _chunkIndex(rhs._chunkIndex)
        {}
        KeyMeta & operator = (const KeyMeta & rhs) {
            _lookupCount.store(rhs.lookupCount(), std::memory_order_release);
            _bitCount = rhs._bitCount;
            _chunkId = rhs._chunkId;
            _chunkIndex = rhs._chunkIndex;
            return *this;
        }
        double       cost()  const { return _bitCount * lookupCount(); }
        bool     isCached()  const { return _chunkId >= 0; }
        size_t   bitCount()  const { return _bitCount; }
        size_t chunkIndex()  const { return _chunkIndex; }
        size_t    chunkId()  const { return _chunkId; }
        size_t lookupCount() const { return _lookupCount.load(std::memory_order_relaxed); }
        KeyMeta &  lookup() { _lookupCount++; return *this; }
        KeyMeta &   bitCount(uint32_t v) {   _bitCount = v; return *this; }
        KeyMeta &    chunkId(uint32_t v) {    _chunkId = v; return *this; }
        KeyMeta & chunkIndex(uint32_t v) { _chunkIndex = v; return *this; }
        KeyMeta & unCache()              { _chunkId = -1; return *this; }
    private:
        std::atomic<size_t>   _lookupCount;
        uint32_t _bitCount;
        int32_t  _chunkId;
        uint32_t _chunkIndex;
    };
    using Key2Index = vespalib::hash_map<Key, KeyMeta>;
    using SortedKeyMeta = std::vector<std::pair<Key, KeyMeta *>>;
    using ChunkV = std::vector<CondensedBitVector::SP>;

    VESPA_DLL_LOCAL static SortedKeyMeta getSorted(Key2Index & keys);
    VESPA_DLL_LOCAL static void populate(Key2Index & newKeys, CondensedBitVector & chunk, const PopulateInterface & lookup);
    VESPA_DLL_LOCAL bool hasCostChanged(const std::shared_lock<std::shared_mutex> &);

    std::atomic<uint64_t>     _lookupCount;
    std::atomic<bool>         _needPopulation;
    mutable std::shared_mutex _mutex;
    Key2Index                 _keys;
    ChunkV                    _chunks;
    GenerationHolder         &_genHolder;
};

}

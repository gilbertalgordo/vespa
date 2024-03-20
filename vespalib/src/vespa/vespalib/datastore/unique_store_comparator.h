// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "entry_comparator.h"
#include "unique_store_entry.h"
#include "datastore.h"
#include <vespa/vespalib/stllike/hash_fun.h>
#include <cmath>

namespace vespalib::datastore {

/**
 * Helper class for comparing elements in unique store.
 */
template <typename EntryT>
class UniqueStoreComparatorHelper {
public:
    static bool less(const EntryT& lhs, const EntryT& rhs) noexcept {
        return lhs < rhs;
    }
    static bool equal(const EntryT& lhs, const EntryT& rhs) noexcept {
        return lhs == rhs;
    }
    static size_t hash(const EntryT& rhs) noexcept {
        vespalib::hash<EntryT> hasher;
        return hasher(rhs);
    }
};

/**
 * Helper class for comparing floating point elements in unique store with
 * special handling of NAN.
 */
template <typename EntryT>
class UniqueStoreFloatingPointComparatorHelper
{
public:
    static bool less(EntryT lhs, const EntryT rhs) noexcept {
        if (std::isnan(lhs)) {
            return !std::isnan(rhs);
        } else if (std::isnan(rhs)) {
            return false;
        } else {
            return (lhs < rhs);
        }
    }
    static bool equal(EntryT lhs, const EntryT rhs) noexcept {
        if (std::isnan(lhs)) {
            return std::isnan(rhs);
        } else if (std::isnan(rhs)) {
            return false;
        } else {
            return (lhs == rhs);
        }
    }
    static size_t hash(EntryT rhs) noexcept {
        if (std::isnan(rhs)) {
            return 0;
        } else {
            union U { EntryT f; std::conditional_t<std::is_same_v<double, EntryT>, uint64_t, uint32_t> i; };
            U t;
            t.f = rhs;
            return t.i;
        }
    }
};

/**
 * Specialized helper class for comparing float elements in unique store with
 * special handling of NAN.
 */
template <>
class UniqueStoreComparatorHelper<float> : public UniqueStoreFloatingPointComparatorHelper<float> {
};

/**
 * Specialized helper class for comparing double elements in unique store with
 * special handling of NAN.
 */
template <>
class UniqueStoreComparatorHelper<double> : public UniqueStoreFloatingPointComparatorHelper<double> {
};

/**
 * Compare two entries based on entry refs.
 *
 * Valid entry ref is mapped to an entry in a data store.
 * Invalid entry ref is mapped to a temporary entry owned by comparator instance.
 */
template <typename EntryT, typename RefT>
class UniqueStoreComparator : public EntryComparator {
protected:
    using EntryType = EntryT;
    using WrappedEntryType = UniqueStoreEntry<EntryType>;
    using RefType = RefT;
    using DataStoreType = DataStoreT<RefT>;
    const DataStoreType &_store;
    const EntryType _lookup_value;

    const EntryType &get(EntryRef ref) const noexcept {
        if (ref.valid()) {
            RefType iRef(ref);
            return _store.template getEntry<WrappedEntryType>(iRef)->value();
        } else {
            return _lookup_value;
        }
    }
    UniqueStoreComparator(const DataStoreType &store, const EntryType &lookup_value) noexcept
        : _store(store),
          _lookup_value(lookup_value)
    {
    }
public:
    UniqueStoreComparator(const DataStoreType &store) noexcept
        : _store(store),
          _lookup_value()
    {
    }

    bool less(const EntryRef lhs, const EntryRef rhs) const noexcept override {
        const EntryType &lhsValue = get(lhs);
        const EntryType &rhsValue = get(rhs);
        return UniqueStoreComparatorHelper<EntryT>::less(lhsValue, rhsValue);
    }
    bool equal(const EntryRef lhs, const EntryRef rhs) const noexcept override {
        const EntryType &lhsValue = get(lhs);
        const EntryType &rhsValue = get(rhs);
        return UniqueStoreComparatorHelper<EntryT>::equal(lhsValue, rhsValue);
    }
    size_t hash(const EntryRef rhs) const noexcept override {
        const EntryType &rhsValue = get(rhs);
        return UniqueStoreComparatorHelper<EntryT>::hash(rhsValue);
    }

    UniqueStoreComparator<EntryT, RefT> make_for_lookup(const EntryType& lookup_value) const noexcept {
        return UniqueStoreComparator<EntryT, RefT>(_store, lookup_value);
    }
};

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <string>

namespace vespalib { class asciistream; }

namespace search::query {

class Range {
    std::string _range;

public:
    Range() noexcept : _range() {}
    Range(int64_t f, int64_t t);
    Range(std::string range) noexcept : _range(std::move(range)) {}

    const std::string & getRangeString() const { return _range; }
};

inline bool operator==(const Range &r1, const Range &r2) {
    return r1.getRangeString() == r2.getRangeString();
}

vespalib::asciistream &operator<<(vespalib::asciistream &out, const Range &range);

}

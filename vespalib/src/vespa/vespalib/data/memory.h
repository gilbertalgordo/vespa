// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cstring>
#include <ostream>
#include <string>

namespace vespalib {

/**
 * Simple wrapper referencing a read-only region of memory.
 **/
struct Memory
{
    const char *data;
    size_t      size;

    Memory() noexcept : data(nullptr), size(0) {}
    Memory(const char *d, size_t s) noexcept : data(d), size(s) {}
    Memory(const char *str) noexcept : data(str), size(strlen(str)) {}
    Memory(const std::string &str) noexcept
        : data(str.data()), size(str.size()) {}
    Memory(std::string_view str_ref) noexcept
        : data(str_ref.data()), size(str_ref.size()) {}
    std::string make_string() const;
    std::string_view make_stringview() const noexcept { return {data, size}; }
    bool operator == (const Memory &rhs) const noexcept {
        if (size != rhs.size) {
            return false;
        }
        if ((size == 0) || (data == rhs.data)) {
            return true;
        }
        return (memcmp(data, rhs.data, size) == 0);
    }
};

std::ostream &operator<<(std::ostream &os, const Memory &memory);

} // namespace vespalib

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <string>
#include <string_view>

namespace vespalib {

class StaticStringView;
namespace literals {
constexpr StaticStringView operator "" _ssv(const char *literal, size_t size);
} // literals

/**
 * Contains the view of a literal string
 **/
class StaticStringView {
private:
    std::string_view _view;
    friend constexpr StaticStringView literals::operator "" _ssv(const char *, size_t);
    constexpr StaticStringView(const char *literal, size_t size) noexcept
      : _view(literal, size) {}
public:
    constexpr std::string_view view() const noexcept { return _view; }
    constexpr operator std::string_view() const noexcept { return _view; }
    std::string_view ref() const noexcept { return {_view.data(), _view.size()}; }
};

namespace literals {
constexpr StaticStringView operator "" _ssv(const char *literal, size_t size) {
    return {literal, size};
}
} // literals

} // vespalib

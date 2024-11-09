// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <memory>
#include <string>

namespace vespalib {
    class Slime;
}

namespace config {

/**
 * Simple data container for slime object.
 */
class ConfigDataBuffer
{
public:
    ConfigDataBuffer(const ConfigDataBuffer &) = delete;
    ConfigDataBuffer & operator = (const ConfigDataBuffer &) = delete;
    ConfigDataBuffer(ConfigDataBuffer &&) noexcept = default;
    ConfigDataBuffer & operator = (ConfigDataBuffer &&) noexcept = default;
    ConfigDataBuffer();
    ~ConfigDataBuffer();
    vespalib::Slime & slimeObject() { return *_slime; }
    const vespalib::Slime & slimeObject() const { return *_slime; }
    const std::string & getEncodedString() const { return _encoded; }
    void setEncodedString(std::string_view encoded) { _encoded = encoded; }
private:
    std::unique_ptr<vespalib::Slime> _slime;
    std::string _encoded;
};

} // namespace config


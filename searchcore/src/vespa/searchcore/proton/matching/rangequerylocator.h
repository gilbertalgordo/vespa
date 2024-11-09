// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cstdint>
#include <string>

namespace search::queryeval { class Blueprint; }

namespace proton::matching {

class RangeLimitMetaInfo {
public:
    RangeLimitMetaInfo();
    RangeLimitMetaInfo(std::string_view low, std::string_view high, size_t estimate);
    ~RangeLimitMetaInfo();
    const std::string & low() const { return _low; }
    const std::string & high() const { return _high; }
    bool valid() const { return _valid; }
    size_t estimate() const { return _estimate; }
private:
    bool             _valid;
    size_t           _estimate;
    std::string _low;
    std::string _high;
};

class RangeQueryLocator {
public:
    virtual ~RangeQueryLocator() = default;
    virtual RangeLimitMetaInfo locate() const = 0;
};

class LocateRangeItemFromQuery : public RangeQueryLocator {
public:
    LocateRangeItemFromQuery(const search::queryeval::Blueprint & blueprint, uint32_t field_id)
        : _blueprint(blueprint),
          _field_id(field_id)
    {}
    RangeLimitMetaInfo locate() const override;
private:
    const search::queryeval::Blueprint & _blueprint;
    uint32_t _field_id;
};

}

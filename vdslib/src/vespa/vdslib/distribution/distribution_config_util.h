// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace storage::lib {

struct DistributionConfigUtil {
    // Converts an input string of the form "1.2.3" to a returned vector {1, 2, 3}
    static std::vector<uint16_t> getGroupPath(std::string_view path);
};

}

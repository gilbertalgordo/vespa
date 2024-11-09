// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "make_attribute_map_lookup_node.h"
#include <vespa/searchlib/expression/attribute_map_lookup_node.h>
#include <vespa/vespalib/stllike/asciistream.h>

namespace search::expression::test {

namespace {

std::string indirectKeyMarker("attribute(");

}

std::unique_ptr<AttributeNode>
makeAttributeMapLookupNode(std::string_view attributeName)
{
    vespalib::asciistream keyName;
    vespalib::asciistream valueName;
    auto leftBracePos = attributeName.find('{');
    auto baseName = attributeName.substr(0, leftBracePos);
    auto rightBracePos = attributeName.rfind('}');
    keyName << baseName << ".key";
    valueName << baseName << ".value" << attributeName.substr(rightBracePos + 1);
    if (rightBracePos != std::string::npos && rightBracePos > leftBracePos) {
        if (attributeName[leftBracePos + 1] == '"' && attributeName[rightBracePos - 1] == '"') {
            std::string key(attributeName.substr(leftBracePos + 2, rightBracePos - leftBracePos - 3));
            return std::make_unique<AttributeMapLookupNode>(attributeName, keyName.view(), valueName.view(), key, "");
        } else if (attributeName.substr(leftBracePos + 1, indirectKeyMarker.size()) == indirectKeyMarker && attributeName[rightBracePos - 1] == ')') {
            auto startPos = leftBracePos + 1 + indirectKeyMarker.size();
            std::string keySourceAttributeName(attributeName.substr(startPos, rightBracePos - 1 - startPos));
            return std::make_unique<AttributeMapLookupNode>(attributeName, keyName.view(), valueName.view(), "", keySourceAttributeName);
        }
    }
    return {};
}

}

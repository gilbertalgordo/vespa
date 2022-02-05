// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "types.h"
#include <vespa/vespalib/util/stringfmt.h>
#include <set>
#include <cerrno>

namespace config {

/**
 * To reduce the need for code in autogenerated config classes, these
 * helper functions exist to help parsing.
 */
class ConfigParser {
public:
    using vsvector = StringVector;
private:
    static vsvector getLinesForKey(vespalib::stringref key, const vsvector & config);

    static std::vector<vsvector> splitArray( const vsvector & config);
    static std::map<vespalib::string, vsvector> splitMap( const vsvector & config);

    static vespalib::string deQuote(const vespalib::string & source);
    static void throwNoDefaultValue(vespalib::stringref key);

    template<typename T>
    static T convert(const vsvector &);

    static vespalib::string arrayToString(const vsvector &);

    template<typename T, typename V>
    static T parseInternal(vespalib::stringref key, const V & config);
    template<typename T, typename V>
    static T parseInternal(vespalib::stringref key, const V & config, T defaultValue);

    template<typename T, typename V>
    static std::vector<T> parseArrayInternal(vespalib::stringref key, const V & config);
    template<typename T, typename V>
    static std::map<vespalib::string, T> parseMapInternal(vespalib::stringref key, const V & config);
    template<typename T, typename V>
    static T parseStructInternal(vespalib::stringref key, const V & config);

public:
    static void stripLinesForKey(vespalib::stringref key,
                                 std::set<vespalib::string>& config);
    static vespalib::string stripWhitespace(vespalib::stringref source);

    template<typename T>
    static T parse(vespalib::stringref key, const vsvector & config) {
        return parseInternal<T, vsvector>(key, config);
    }
    template<typename T>
    static T parse(vespalib::stringref key, const vsvector & config, T defaultValue) {
        return parseInternal(key, config, defaultValue);
    }

    template<typename T>
    static std::vector<T> parseArray(vespalib::stringref key, const vsvector & config) {
        return parseArrayInternal<T, vsvector>(key, config);
    }

    template<typename T>
    static std::map<vespalib::string, T> parseMap(vespalib::stringref key, const vsvector & config) {
        return parseMapInternal<T, vsvector>(key, config);
    }

    template<typename T>
    static T parseStruct(vespalib::stringref key, const vsvector & config) {
        return parseStructInternal<T, vsvector>(key, config);
    }

};

template<typename T, typename V>
T
ConfigParser::parseInternal(vespalib::stringref key, const V & config)
{
    V lines = getLinesForKey(key, config);

    if (lines.size() == 0) {
        throwNoDefaultValue(key);
    }
    return convert<T>(lines);
}

template<typename T, typename V>
T
ConfigParser::parseInternal(vespalib::stringref key, const V & config, T defaultValue)
{
    V lines = getLinesForKey(key, config);

    if (lines.size() == 0) {
        return defaultValue;
    }

    return convert<T>(lines);
}

template<typename T>
T
ConfigParser::convert(const vsvector & lines) {
    return T(lines);
}

template<typename T, typename V>
std::map<vespalib::string, T>
ConfigParser::parseMapInternal(vespalib::stringref key, const V & config)
{
    V lines = getLinesForKey(key, config);
    typedef std::map<vespalib::string, V> SplittedMap;
    SplittedMap s = splitMap(lines);
    std::map<vespalib::string, T> retval;
    for (typename SplittedMap::iterator it(s.begin()), mt(s.end()); it != mt; it++) {
        retval[it->first] = convert<T>(it->second);
    }
    return retval;
}

template<typename T, typename V>
std::vector<T>
ConfigParser::parseArrayInternal(vespalib::stringref key, const V & config)
{
    V lines = getLinesForKey(key, config);
    std::vector<V> split = splitArray(lines);

    std::vector<T> retval;
    for (uint32_t i = 0; i < split.size(); i++) {
        retval.push_back(convert<T>(split[i]));
    }

    return retval;
}

template<typename T, typename V>
T
ConfigParser::parseStructInternal(vespalib::stringref key, const V & config)
{
    V lines = getLinesForKey(key, config);

    return convert<T>(lines);
}

template<>
bool
ConfigParser::convert<bool>(const vsvector & config);

template<>
int32_t
ConfigParser::convert<int32_t>(const vsvector & config);

template<>
int64_t
ConfigParser::convert<int64_t>(const vsvector & config);

template<>
double
ConfigParser::convert<double>(const vsvector & config);

template<>
vespalib::string
ConfigParser::convert<vespalib::string>(const vsvector & config);

} // config


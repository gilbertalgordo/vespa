// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "httpurlpath.h"
#include <vespa/vespalib/text/stringtokenizer.h>

namespace storage::framework {

HttpUrlPath::HttpUrlPath(const std::string& urlpath)
    : _path(),
      _attributes(),
      _serverSpec()
{
    init(urlpath);
}

HttpUrlPath::HttpUrlPath(const std::string& urlpath,
                         const std::string& serverSpec)
    : _path(),
      _attributes(),
      _serverSpec(serverSpec)
{
    init(urlpath);
}

HttpUrlPath::HttpUrlPath(std::string path,
                         std::map<std::string, std::string> attributes,
                         std::string serverSpec)
    : _path(std::move(path)),
      _attributes(std::move(attributes)),
      _serverSpec(std::move(serverSpec))
{
}

HttpUrlPath::~HttpUrlPath() {}

void
HttpUrlPath::init(const std::string &urlpath)
{
    std::string::size_type pos = urlpath.find('?');
    if (pos == std::string::npos) {
        _path = urlpath;
    } else {
        _path = urlpath.substr(0, pos);
        std::string sub(urlpath.substr(pos+1));
        vespalib::StringTokenizer tokenizer(sub, "&", "");
        for (uint32_t i=0, n=tokenizer.size(); i<n; ++i) {
            std::string s(tokenizer[i]);
            pos = s.find('=');
            if (pos == std::string::npos) {
                _attributes[s] = "";
            } else {
                _attributes[s.substr(0,pos)] = s.substr(pos+1);
            }
        }
    }
}

bool
HttpUrlPath::hasAttribute(const std::string& id) const
{
    return (_attributes.find(id) != _attributes.end());
}

std::string
HttpUrlPath::getAttribute(const std::string& id,
                          const std::string& defaultValue) const
{
    auto it = _attributes.find(id);
    return (it == _attributes.end() ? defaultValue : it->second);
}

void
HttpUrlPath::print(std::ostream& out, bool, const std::string&) const
{
    out << _path;
    if (!_attributes.empty()) {
        out << "?";
        size_t cnt = 0;
        for (const auto &attr: _attributes) {
            if (cnt++ > 0) {
                out << "&";
            }
            out << attr.first;
            if (!attr.second.empty()) {
                out << "=";
                out << attr.second;
            }
        }
    }
}

}

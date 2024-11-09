// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <map>
#include <string>

namespace vespalib::net { class ConnectionAuthContext; }

namespace vespalib {

struct JsonGetHandler {
    class Response {
        int              _status_code;
        std::string _status_or_payload;
        std::string _content_type_override;

        Response(int status_code,
                 std::string status_or_payload,
                 std::string content_type_override);
    public:
        Response(); // By default, 500 Internal Server Error
        ~Response();
        Response(const Response&);
        Response& operator=(const Response&);
        Response(Response&&) noexcept;
        Response& operator=(Response&&) noexcept;

        [[nodiscard]] int status_code() const noexcept { return _status_code; }
        [[nodiscard]] bool ok() const noexcept { return _status_code == 200; }
        [[nodiscard]] bool failed() const noexcept { return _status_code != 200; }
        [[nodiscard]] std::string_view status_message() const noexcept {
            if (_status_code == 200) {
                return "OK";
            } else {
                return _status_or_payload;
            }
        }
        [[nodiscard]] std::string_view payload() const noexcept {
            if (_status_code == 200) {
                return _status_or_payload;
            } else {
                return {};
            }
        }
        [[nodiscard]] std::string_view content_type() const noexcept {
            if (_content_type_override.empty()) {
                return "application/json";
            } else {
                return _content_type_override;
            }
        }

        [[nodiscard]] static Response make_ok_with_json(std::string json);
        [[nodiscard]] static Response make_ok_with_content_type(std::string payload, std::string content_type);
        [[nodiscard]] static Response make_failure(int status_code, std::string status_message);
        [[nodiscard]] static Response make_not_found();
    };

    virtual Response get(const std::string &host,
                         const std::string &path,
                         const std::map<std::string,std::string> &params,
                         const net::ConnectionAuthContext &auth_ctx) const = 0;
    virtual ~JsonGetHandler() = default;
};

} // namespace vespalib

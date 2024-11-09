// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "certificate_verification_callback.h"
#include "peer_policies.h"
#include <memory>

namespace vespalib::net::tls {

class TransportSecurityOptions {
    std::string _ca_certs_pem;
    std::string _cert_chain_pem;
    std::string _private_key_pem;
    AuthorizedPeers  _authorized_peers;
    std::vector<std::string> _accepted_ciphers;
    bool _disable_hostname_validation;
public:
    struct Params {
        std::string _ca_certs_pem;
        std::string _cert_chain_pem;
        std::string _private_key_pem;
        AuthorizedPeers  _authorized_peers;
        std::vector<std::string> _accepted_ciphers;
        bool _disable_hostname_validation;

        Params();
        ~Params();
        Params(const Params&);
        Params& operator=(const Params&) = delete;
        Params(Params&&) noexcept;
        Params& operator=(Params&&) noexcept;

        Params& ca_certs_pem(std::string_view pem) { _ca_certs_pem = pem; return *this; }
        Params& cert_chain_pem(std::string_view pem) { _cert_chain_pem = pem; return *this; }
        Params& private_key_pem(std::string_view pem) { _private_key_pem = pem; return *this; }
        Params& authorized_peers(AuthorizedPeers auth) { _authorized_peers = std::move(auth); return *this; }
        Params& accepted_ciphers(std::vector<std::string> ciphers) {
            _accepted_ciphers = std::move(ciphers);
            return *this;
        }
        Params& disable_hostname_validation(bool disable) {
            _disable_hostname_validation = disable;
            return *this;
        }
    };

    explicit TransportSecurityOptions(Params params);
    TransportSecurityOptions(const TransportSecurityOptions&);
    TransportSecurityOptions& operator=(const TransportSecurityOptions&) = delete;
    TransportSecurityOptions(TransportSecurityOptions&&) noexcept;
    TransportSecurityOptions& operator=(TransportSecurityOptions&&) noexcept;

    ~TransportSecurityOptions();

    const std::string& ca_certs_pem() const noexcept { return _ca_certs_pem; }
    const std::string& cert_chain_pem() const noexcept { return _cert_chain_pem; }
    const std::string& private_key_pem() const noexcept { return _private_key_pem; }
    const AuthorizedPeers& authorized_peers() const noexcept { return _authorized_peers; }

    TransportSecurityOptions copy_without_private_key() const;
    const std::vector<std::string>& accepted_ciphers() const noexcept { return _accepted_ciphers; }
    bool disable_hostname_validation() const noexcept { return _disable_hostname_validation; }

private:
    TransportSecurityOptions(std::string ca_certs_pem,
                             std::string cert_chain_pem,
                             std::string private_key_pem,
                             AuthorizedPeers authorized_peers,
                             bool disable_hostname_validation);
};

// Zeroes out `size` bytes in `buf` in a way that shall never be optimized
// away by an eager compiler.
// TODO move to own crypto utility library
void secure_memzero(void* buf, size_t size) noexcept;

} // vespalib::net::tls

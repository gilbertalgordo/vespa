// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "private_key.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace vespalib::crypto {

/**
 * Represents an X509 certificate instance and provides utility methods
 * for generating new certificates on the fly. Certificates can be created
 * for both Certificate Authorities and regular hosts (leaves).
 *
 * This implementation aims to ensure that best cryptographic practices are
 * followed automatically. In particular:
 *   - The certificate digest is always SHA-256, never SHA-1 or MD5
 *   - The certificate serial number is a 160-bit secure random sequence
 *     (technically 159 bits since the MSB is always zero) rather than a
 *     collision-prone or predictable sequence number.
 *
 */
class X509Certificate {
public:
    virtual ~X509Certificate() = default;

    virtual std::string to_pem() const = 0;

    struct DistinguishedName {
        std::string _country;             // "C"
        std::string _state;               // "ST"
        std::string _locality;            // "L"
        std::string _organization;        // "O"
        std::string _organizational_unit; // "OU"
        // Should only be 1 entry in normal certs, but X509 supports more and
        // we want to be able to test this edge case.
        std::vector<std::string> _common_names; // "CN"

        DistinguishedName() noexcept;
        DistinguishedName(const DistinguishedName&);
        DistinguishedName& operator=(const DistinguishedName&) = delete;
        DistinguishedName(DistinguishedName&&) noexcept;
        DistinguishedName& operator=(DistinguishedName&&) noexcept;
        ~DistinguishedName();

        // TODO could add rvalue overloads as well...
        DistinguishedName& country(std::string_view c)      { _country = c; return *this; }
        DistinguishedName& state(std::string_view st)       { _state = st; return *this; }
        DistinguishedName& locality(std::string_view l)     { _locality = l; return *this; }
        DistinguishedName& organization(std::string_view o) { _organization = o; return *this; }
        DistinguishedName& organizational_unit(std::string_view ou) {
            _organizational_unit = ou;
            return *this;
        }
        DistinguishedName& add_common_name(std::string_view cn) {
            _common_names.emplace_back(cn);
            return *this;
        }
    };

    struct SubjectInfo {
        DistinguishedName dn;
        std::vector<std::string> subject_alt_names;

        SubjectInfo() noexcept;
        explicit SubjectInfo(DistinguishedName dn_) noexcept;
        SubjectInfo(const SubjectInfo &);
        SubjectInfo & operator=(const SubjectInfo &) = delete;
        SubjectInfo(SubjectInfo &&) noexcept;
        SubjectInfo & operator=(SubjectInfo &&) noexcept;
        ~SubjectInfo();

        SubjectInfo& add_subject_alt_name(std::string san) {
            subject_alt_names.emplace_back(std::move(san));
            return *this;
        }
    };

    struct Params {
        Params() noexcept;
        ~Params();

        Params(const Params&);
        Params& operator=(const Params&) = delete;
        Params(Params&&) noexcept;
        Params& operator=(Params&&) noexcept;

        SubjectInfo subject_info;
        // TODO make public key, but private key has both.
        std::shared_ptr<PrivateKey> subject_key;
        std::shared_ptr<X509Certificate> issuer; // May be nullptr for self-signed certs
        std::shared_ptr<PrivateKey> issuer_key;
        std::chrono::seconds valid_for = std::chrono::hours(24);
        bool is_ca = false;

        static Params self_signed(SubjectInfo subject, std::shared_ptr<PrivateKey> key);
        // TODO only need _public_ key from subject, but this is simplified
        static Params issued_by(SubjectInfo subject,
                                std::shared_ptr<PrivateKey> subject_key,
                                std::shared_ptr<X509Certificate> issuer,
                                std::shared_ptr<PrivateKey> issuer_key);
    };

    // Generates an X509 certificate using a SHA-256 digest
    static std::shared_ptr<X509Certificate> generate_from(Params params);
protected:
    X509Certificate() = default;
};

/*
 * Simple wrapper for storing both a X509 certificate and the private key
 * that signed it. Useful for testing.
 */
struct CertKeyWrapper {
    std::shared_ptr<X509Certificate> cert;
    std::shared_ptr<PrivateKey> key;

    CertKeyWrapper(std::shared_ptr<X509Certificate> cert_,
                   std::shared_ptr<PrivateKey> key_);
    CertKeyWrapper(CertKeyWrapper &&) noexcept;
    CertKeyWrapper & operator=(CertKeyWrapper &&) noexcept;
    ~CertKeyWrapper();
};

}

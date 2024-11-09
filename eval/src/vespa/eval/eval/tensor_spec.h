// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/vespalib/util/approx.h>
#include <memory>
#include <map>
#include <string>

namespace vespalib::slime {
    struct Cursor;
    struct Inspector;
}

namespace vespalib::eval {

struct Value;

/**
 * An implementation-independent specification of the type and
 * contents of a tensor.
 **/
class TensorSpec
{
public:
    struct Label {
        size_t index;
        std::string name;
        static constexpr size_t npos = -1;
        Label(size_t index_in) noexcept : index(index_in), name() {}
        Label(std::string name_in) noexcept : index(npos), name(std::move(name_in)) {}
        Label(const char *name_in) : index(npos), name(name_in) {}
        bool is_mapped() const noexcept { return (index == npos); }
        bool is_indexed() const noexcept { return (index != npos); }
        bool operator==(const Label &rhs) const noexcept {
            return ((index == rhs.index) &&
                    (name == rhs.name));
        }
        bool operator<(const Label &rhs) const noexcept {
            if (index != rhs.index) {
                return (index < rhs.index);
            }
            return (name < rhs.name);
        }
    };
    struct Value {
        double value;
        explicit Value(double value_in) noexcept : value(value_in) {}
        operator double() const noexcept { return value; }
        static bool both_nan(double a, double b) noexcept {
            return (std::isnan(a) && std::isnan(b));
        }
        bool operator==(const Value &rhs) const noexcept {
            return (both_nan(value, rhs.value) || approx_equal(value, rhs.value));
        }
    };
    using Address = std::map<std::string,Label>;
    using Cells = std::map<Address,Value>;
private:
    std::string _type;
    Cells _cells;
public:
    TensorSpec(std::string type_spec) noexcept;
    TensorSpec(const TensorSpec &);
    TensorSpec & operator = (const TensorSpec &);
    ~TensorSpec();
    double as_double() const;
    TensorSpec &add(Address address, double value);
    const std::string &type() const { return _type; }
    const Cells &cells() const { return _cells; }
    std::string to_string() const;
    TensorSpec normalize() const;
    void to_slime(slime::Cursor &tensor) const;
    std::string to_expr() const;
    static TensorSpec from_slime(const slime::Inspector &tensor);
    static TensorSpec from_value(const eval::Value &value);
    static TensorSpec from_expr(const std::string &expr);
    static std::string diff(const TensorSpec &lhs, const std::string &lhs_desc,
                                 const TensorSpec &rhs, const std::string &rhs_desc);
};

bool operator==(const TensorSpec &lhs, const TensorSpec &rhs);
std::ostream &operator<<(std::ostream &out, const TensorSpec &tensor);

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "currentindex.h"
#include <vespa/vespalib/stllike/hash_map.h>
#include <vespa/vespalib/stllike/hash_set.h>
#include <string>
#include <utility>

namespace search::expression {

class CurrentIndexSetup {
public:
    class Usage {
    private:
        friend class CurrentIndexSetup;
        vespalib::hash_set<std::string> _unbound;
        void notify_unbound_struct_usage(std::string_view name);
    public:
        Usage();
        ~Usage();
        [[nodiscard]] bool has_single_unbound_struct() const noexcept {
            return (_unbound.size() == 1);
        }
        std::string_view get_unbound_struct_name() const;
        class Bind {
        private:
            CurrentIndexSetup &_setup;
        public:
            Bind(CurrentIndexSetup &setup, Usage &usage) noexcept;
            ~Bind();
        };
    };
private:
    vespalib::hash_map<std::string, const CurrentIndex *> _bound;
    Usage *_usage;
    [[nodiscard]] Usage *capture(Usage *usage) noexcept {
        return std::exchange(_usage, usage);
    }
public:
    CurrentIndexSetup();
    ~CurrentIndexSetup();
    [[nodiscard]] const CurrentIndex *resolve(std::string_view field_name) const;    
    void bind(std::string_view struct_name, const CurrentIndex &index);
};

}

// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "jsonexception.h"
#include <string>

namespace vespalib {

VESPA_IMPLEMENT_EXCEPTION_SPINE(JsonStreamException);

JsonStreamException::JsonStreamException(std::string_view reason, std::string_view history,
                                         std::string_view location, int skipStack)
    : Exception(std::string(reason) + (history.empty() ? "" : "\nHistory:\n" + std::string(history)),
                location, skipStack + 1),
      _reason(reason)
{ }

JsonStreamException::JsonStreamException(const JsonStreamException &) = default;
JsonStreamException & JsonStreamException::operator = (const JsonStreamException &) = default;
JsonStreamException::~JsonStreamException() = default;

}

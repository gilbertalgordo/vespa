// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * \class storage::spi::DocumentSelection
 * \ingroup spi
 *
 * \brief
 */

#pragma once

#include <string>

namespace document { class Document; }
namespace storage::spi {

class DocumentSelection
{
    std::string _documentSelection;
 public:
    explicit DocumentSelection(const std::string& docSel)
        : _documentSelection(docSel) {}

    bool match(const document::Document&) const { return true; }

    const std::string& getDocumentSelection() const {
        return _documentSelection;
    }
};

}

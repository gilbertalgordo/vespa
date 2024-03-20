// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.document.serialization;

import com.yahoo.document.Document;
import com.yahoo.document.DocumentId;
import com.yahoo.document.DocumentRemove;
import com.yahoo.document.DocumentType;
import com.yahoo.document.DocumentUpdate;

/**
 * @author ravishar
 */
public interface DocumentWriter extends FieldWriter {

    /** Writes a document. */
    void write(Document document);

    void write(DocumentId id);

    void write(DocumentType type);

    void write(DocumentRemove documentRemove);

    void write(DocumentUpdate documentUpdate);

}

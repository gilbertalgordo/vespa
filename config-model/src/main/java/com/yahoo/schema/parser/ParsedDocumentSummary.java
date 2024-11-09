// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

package com.yahoo.schema.parser;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * This class holds the extracted information after parsing a
 * "document-summary" block, using simple data structures as far as
 * possible.  Do not put advanced logic here!
 *
 * @author arnej27959
 */
public class ParsedDocumentSummary extends ParsedBlock {

    private boolean omitSummaryFeatures;
    private boolean fromDisk;
    private final List<String> inherited = new ArrayList<>();
    private final Map<String, ParsedSummaryField> fields = new LinkedHashMap<>();

    public ParsedDocumentSummary(String name) {
        super(name, "document-summary");
    }

    boolean getOmitSummaryFeatures() { return omitSummaryFeatures; }
    boolean getFromDisk() { return fromDisk; }
    List<ParsedSummaryField> getSummaryFields() { return List.copyOf(fields.values()); }
    List<String> getInherited() { return List.copyOf(inherited); }

    public ParsedSummaryField addField(ParsedSummaryField field) {
        String fieldName = field.name();
        verifyThat(! fields.containsKey(fieldName), "already has field", fieldName);
        return fields.put(fieldName, field);
    }

    public void setFromDisk(boolean value) {
        this.fromDisk = value;
    }

    public void setOmitSummaryFeatures(boolean value) {
        this.omitSummaryFeatures = value;
    }

    public void inherit(String other) {
        inherited.add(other);
    }

}

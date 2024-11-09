// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.schema.parser;

import com.yahoo.schema.document.Case;
import com.yahoo.schema.document.MatchType;
import com.yahoo.schema.document.MatchAlgorithm;

import java.util.Optional;

/**
 * This class holds the extracted information after parsing a "match"
 * block, using simple data structures as far as possible.  Do not put
 * advanced logic here!
 *
 * @author arnej27959
 */
public class ParsedMatchSettings {

    private MatchType matchType = null;
    private Case matchCase = null;
    private MatchAlgorithm matchAlgorithm = null;
    private String exactTerminator = null;
    private Integer gramSize = null;
    private Integer maxLength = null;
    private Integer maxTermOccurrences = null;
    private Integer maxTokenLength = null;

    Optional<MatchType> getMatchType() { return Optional.ofNullable(matchType); }
    Optional<Case> getMatchCase() { return Optional.ofNullable(matchCase); }
    Optional<MatchAlgorithm> getMatchAlgorithm() { return Optional.ofNullable(matchAlgorithm); }
    Optional<String> getExactTerminator() { return Optional.ofNullable(exactTerminator); }
    Optional<Integer> getGramSize() { return Optional.ofNullable(gramSize); }
    Optional<Integer> getMaxLength() { return Optional.ofNullable(maxLength); }
    Optional<Integer> getMaxTermOccurrences() { return Optional.ofNullable(maxTermOccurrences); }
    Optional<Integer> getMaxTokenLength() { return Optional.ofNullable(maxTokenLength); }

    // TODO - consider allowing each set only once:
    public void setType(MatchType value) { this.matchType = value; }
    public void setCase(Case value) { this.matchCase = value; }
    public void setAlgorithm(MatchAlgorithm value) { this.matchAlgorithm = value; }
    public void setExactTerminator(String value) { this.exactTerminator = value; }
    public void setGramSize(int value) { this.gramSize = value; }
    public void setMaxLength(int value) { this.maxLength = value; }
    public void setMaxTermOccurrences(int value) { this.maxTermOccurrences = value; }
    public void setMaxTokenLength(int value) { this.maxTokenLength = value; }

}

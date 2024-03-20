// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.schema.document;

import com.yahoo.schema.processing.NGramMatch;

import java.io.Serializable;
import java.util.OptionalInt;

/**
 * Defines how a field should be matched.
 * Matching objects can be compared based on their content, but they are <i>not</i> immutable.
 *
 * @author bratseth
 */
public class Matching implements Cloneable, Serializable {

    public static final MatchType defaultType = MatchType.TEXT;

    private MatchType type = MatchType.TEXT;
    private Case casing = Case.UNCASED;

    /** The basic match algorithm */
    private MatchAlgorithm algorithm = MatchAlgorithm.NORMAL;

    private boolean typeUserSet = false;

    private boolean algorithmUserSet = false;

    /** The gram size is the n in n-gram, or empty if not set. Should only be set with gram matching. */
    private OptionalInt gramSize = OptionalInt.empty();

    /** Maximum number of characters to consider when searching in this field. Used for limiting resources, especially in streaming search. */
    private Integer maxLength;
    /** Maximum number of occurrences for each term */
    private Integer maxTermOccurrences;

    private String exactMatchTerminator = null;

    /** Creates a matching of type "text" */
    public Matching() {}

    public Matching(MatchType type) {
        this.type = type;
    }

    public MatchType getType() { return type; }
    public Case getCase() { return casing; }

    public Matching setType(MatchType type) {
        this.type = type;
        typeUserSet = true;
        return this;
    }

    public Matching setCase(Case casing) {
        this.casing = casing;
        return this;
    }

    public Integer maxLength() { return maxLength; }
    public Matching maxLength(int maxLength) { this.maxLength = maxLength; return this; }
    public Integer maxTermOccurrences() { return maxTermOccurrences; }
    public Matching maxTermOccurrences(int maxTermOccurrences) { this.maxTermOccurrences = maxTermOccurrences; return this; }
    public boolean isTypeUserSet() { return typeUserSet; }

    public MatchAlgorithm getAlgorithm() { return algorithm; }

    public void setAlgorithm(MatchAlgorithm algorithm) {
        this.algorithm = algorithm;
        algorithmUserSet = true;
    }

    public boolean isAlgorithmUserSet() { return algorithmUserSet; }

    public boolean isPrefix() { return algorithm == MatchAlgorithm.PREFIX; }

    public boolean isSubstring() { return algorithm == MatchAlgorithm.SUBSTRING; }

    public boolean isSuffix() { return algorithm == MatchAlgorithm.SUFFIX; }

    /** Returns the gram size, or empty if not set. Should only be set with gram matching. */
    public OptionalInt getGramSize() { return gramSize; }

    public void setGramSize(int gramSize) { this.gramSize = OptionalInt.of(gramSize); }

    /**
     * Merge data from another matching object
     */
    public void merge(Matching m) {
        if (m == null) return;
        if (m.isAlgorithmUserSet()) {
            this.setAlgorithm(m.getAlgorithm());
        }
        if (m.isTypeUserSet()) {
            this.setType(m.getType());
            if (m.getType() == MatchType.GRAM)
              gramSize = m.gramSize;
        }
        if (m.getExactMatchTerminator() != null) {
            this.setExactMatchTerminator(m.getExactMatchTerminator());
        }
    }

    /**
     * If exact matching is used, this returns the terminator string
     * which terminates an exact matched sequence in queries. If exact
     * matching is not used, or no terminator is set, this is null
     */
    public String getExactMatchTerminator() { return exactMatchTerminator; }

    /**
     * Sets the terminator string which terminates an exact matched
     * sequence in queries (used if type is EXACT).
     */
    public void setExactMatchTerminator(String exactMatchTerminator) {
        this.exactMatchTerminator = exactMatchTerminator;
    }

    @Override
    public String toString() {
        return type + " matching [" + (type == MatchType.GRAM ? "gram size " + gramSize.orElse(NGramMatch.DEFAULT_GRAM_SIZE) : "supports " + algorithm) +
               "], [exact-terminator " + exactMatchTerminator + "]";
    }

    @Override
    public Matching clone() {
        try {
            return (Matching)super.clone();
        }
        catch (CloneNotSupportedException e) {
            throw new RuntimeException("Programming error");
        }
    }

    @Override
    public boolean equals(Object o) {
        if (! (o instanceof Matching other)) return false;

        if ( ! other.type.equals(this.type)) return false;
        if ( ! other.algorithm.equals(this.algorithm)) return false;
        if ( this.exactMatchTerminator == null && other.exactMatchTerminator != null) return false;
        if ( this.exactMatchTerminator != null && ( ! this.exactMatchTerminator.equals(other.exactMatchTerminator)) )
            return false;
        if ( ! gramSize.equals(other.gramSize)) return false;
        return true;
    }

    @Override public int hashCode() {
        return java.util.Objects.hash(type, algorithm, exactMatchTerminator, gramSize);
    }

}

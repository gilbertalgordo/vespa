// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.schema.parser;

import com.yahoo.schema.document.Stemming;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * This class holds the extracted information after parsing a "field"
 * block, using simple data structures as far as possible.  Do not put
 * advanced logic here!
 * @author arnej27959
 **/
public class ParsedField extends ParsedBlock {

    private ParsedType type;
    private boolean hasBolding = false;
    private boolean isFilter = false;
    private int overrideId = 0;
    private boolean isLiteral = false;
    private boolean isNormal = false;
    private Integer weight;
    private String normalizing = null;
    private final ParsedMatchSettings matchInfo = new ParsedMatchSettings();
    private Stemming stemming = null;
    private ParsedIndexingOp indexingOp = null;
    private ParsedSorting sortSettings = null;
    private final Map<String, ParsedAttribute> attributes = new LinkedHashMap<>();
    private final Map<String, ParsedIndex> fieldIndexes = new LinkedHashMap<>();
    private final Map<String, String> aliases = new LinkedHashMap<>();
    private final Map<String, String> rankTypes = new LinkedHashMap<>();
    private final Map<String, ParsedField> structFields = new LinkedHashMap<>();
    private final Map<String, ParsedSummaryField> summaryFields = new LinkedHashMap<>();
    private final List<DictionaryOption> dictionaryOptions = new ArrayList<>();
    private final List<String> queryCommands = new ArrayList<>();

    public ParsedField(String name, ParsedType type) {
        super(name, "field");
        this.type = type;
    }

    public ParsedType getType() { return this.type; }
    boolean hasBolding() { return this.hasBolding; }
    boolean hasFilter() { return this.isFilter; }
    boolean hasLiteral() { return this.isLiteral; }
    boolean hasNormal() { return this.isNormal; }
    boolean hasIdOverride() { return overrideId != 0; }
    int idOverride() { return overrideId; }
    List<DictionaryOption> getDictionaryOptions() { return List.copyOf(dictionaryOptions); }
    List<ParsedAttribute> getAttributes() { return List.copyOf(attributes.values()); }
    List<ParsedIndex> getIndexes() { return List.copyOf(fieldIndexes.values()); }
    List<ParsedSummaryField> getSummaryFields() { return List.copyOf(summaryFields.values()); }
    List<ParsedField> getStructFields() { return List.copyOf(structFields.values()); }
    List<String> getAliases() { return List.copyOf(aliases.keySet()); }
    List<String> getQueryCommands() { return List.copyOf(queryCommands); }
    String lookupAliasedFrom(String alias) { return aliases.get(alias); }
    public ParsedMatchSettings matchSettings() { return this.matchInfo; }
    Optional<Integer> getWeight() { return Optional.ofNullable(weight); }
    Optional<Stemming> getStemming() { return Optional.ofNullable(stemming); }
    Optional<String> getNormalizing() { return Optional.ofNullable(normalizing); }
    Optional<ParsedIndexingOp> getIndexing() { return Optional.ofNullable(indexingOp); }
    Optional<ParsedSorting> getSorting() { return Optional.ofNullable(sortSettings); }
    Map<String, String> getRankTypes() { return Collections.unmodifiableMap(rankTypes); }

    /** get an existing summary field for modification, or create it */
    public ParsedSummaryField summaryFieldFor(String name) {
        if (summaryFields.containsKey(name)) {
            return summaryFields.get(name);
        }
        var sf = new ParsedSummaryField(name, getType());
        summaryFields.put(name, sf);
        return sf;
    }

    /** get an existing summary field for modification, or create it */
    public ParsedSummaryField summaryFieldFor(String name, ParsedType type) {
        if (summaryFields.containsKey(name)) {
            var sf = summaryFields.get(name);
            if (sf.getType() == null) {
                sf.setType(type);
            } else {
                // TODO check that types are properly equal here
                String oldName = sf.getType().name();
                String newName = type.name();
                verifyThat(newName.equals(oldName), "type mismatch for summary field", name, ":", oldName, "/", newName);
            }
            return sf;
        }
        var sf = new ParsedSummaryField(name, type);
        summaryFields.put(name, sf);
        return sf;
    }

    public void addAlias(String from, String to) {
        verifyThat(! aliases.containsKey(to), "already has alias", to);
        aliases.put(to, from);
    }

    public void addIndex(ParsedIndex index) {
        String idxName = index.name();
        verifyThat(! fieldIndexes.containsKey(idxName), "already has index", idxName);
        fieldIndexes.put(idxName, index);
    }

    public void addRankType(String index, String rankType) {
        rankTypes.put(index, rankType);
    }

    public void dictionary(DictionaryOption option) {
        dictionaryOptions.add(option);
    }

    public void setBolding(boolean value) { this.hasBolding = value; }
    public void setFilter(boolean value) { this.isFilter = value; }
    public void setId(int id) { this.overrideId = id; }
    public void setLiteral(boolean value) { this.isLiteral = value; }
    public void setNormal(boolean value) { this.isNormal = value; }
    public void setNormalizing(String value) { this.normalizing = value; }
    public void setStemming(Stemming stemming) { this.stemming = stemming; }
    public void setWeight(int weight) { this.weight = weight; }

    public ParsedAttribute attributeFor(String attrName) {
        return attributes.computeIfAbsent(attrName, n -> new ParsedAttribute(n));
    }

    public void setIndexingOperation(ParsedIndexingOp idxOp) {
        verifyThat(indexingOp == null, "already has indexing");
        indexingOp = idxOp;
    }

    public ParsedSorting sortInfo() {
        if (sortSettings == null) sortSettings = new ParsedSorting(name(), "field.sorting");
        return this.sortSettings;
    }

    public void addQueryCommand(String command) {
        queryCommands.add(command);
    }

    public void addStructField(ParsedField structField) {
        String fieldName = structField.name();
        verifyThat(! structFields.containsKey(fieldName), "already has struct-field", fieldName);
        structFields.put(fieldName, structField);
    }

    void addSummaryField(ParsedSummaryField summaryField) {
        String fieldName = summaryField.name();
        verifyThat(! summaryFields.containsKey(fieldName), "already has summary field", fieldName);
        if (summaryField.getType() == null) {
            summaryField.setType(getType());
        }
        summaryFields.put(fieldName, summaryField);
    }
}

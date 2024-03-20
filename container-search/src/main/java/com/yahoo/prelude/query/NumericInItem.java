// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.query;

import com.yahoo.compress.IntegerCompressor;

import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;

/*
 * Class representing an IN operator with a set of 64-bit
 * integer values.
 *
 * @author toregge
 */
public class NumericInItem extends InItem {
    private Set<Long> tokens;

    public NumericInItem(String indexName) {
        super(indexName);
        tokens = new HashSet<>(1000);
    }

    @Override
    public Item.ItemType getItemType() {
        return Item.ItemType.NUMERIC_IN;
    }

    @Override
    public int encode(ByteBuffer buffer) {
        encodeThis(buffer);
        return 1;
    }

    @Override
    protected void encodeThis(ByteBuffer buffer) {
        super.encodeThis(buffer);
        IntegerCompressor.putCompressedPositiveNumber(tokens.size(), buffer);
        putString(getIndexName(), buffer);
        for (var token : tokens) {
            buffer.putLong(token);
        }
    }

    @Override
    public int getTermCount() {
        return 1;
    }

    @Override
    protected void appendBodyString(StringBuilder buffer) {
        buffer.append(getIndexName());
        buffer.append("{");
        for (var token : tokens) {
            buffer.append(token.toString());
            if (token < Integer.MIN_VALUE || token > Integer.MAX_VALUE) {
                buffer.append("L");
            }
            buffer.append(",");
        }
        if (!tokens.isEmpty()) {
            buffer.deleteCharAt(buffer.length() - 1); // remove extra ","
        }
        buffer.append("}");
    }

    public void addToken(long token) {
        tokens.add(token);
    }

    public Collection<Long> getTokens() { return Set.copyOf(tokens); }

    @Override
    public boolean equals(Object o) {
        if (o == this) return true;
        if ( ! super.equals(o)) return false;
        var other = (NumericInItem)o;
        if ( ! Objects.equals(this.tokens, other.tokens)) return false;
        return true;
    }

    @Override
    public int hashCode() {
        return Objects.hash(super.hashCode(), tokens);
    }

}

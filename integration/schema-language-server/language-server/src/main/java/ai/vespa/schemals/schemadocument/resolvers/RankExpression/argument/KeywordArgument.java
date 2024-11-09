package ai.vespa.schemals.schemadocument.resolvers.RankExpression.argument;

import java.util.Optional;

import org.eclipse.lsp4j.Diagnostic;

import ai.vespa.schemals.context.ParseContext;
import ai.vespa.schemals.index.Symbol;
import ai.vespa.schemals.index.Symbol.SymbolStatus;
import ai.vespa.schemals.index.Symbol.SymbolType;
import ai.vespa.schemals.tree.rankingexpression.RankNode;

public class KeywordArgument implements Argument {

    private String argument;
    private String displayStr;

    public KeywordArgument(String argument, String displayStr) {
        this.argument = argument;
        this.displayStr = displayStr;
    }

    public KeywordArgument(String argument) {
        this(argument, argument);
    }
    
    @Override
    public String displayString() {
        return displayStr;
    }

    public String getArgument() { return argument; }

    @Override
    public int getStrictness() {
        return 8;
    }

    @Override
    public boolean validateArgument(RankNode node) {
        return node.getSchemaNode().getText().equals(argument);
    }

    @Override
    public Optional<Diagnostic> parseArgument(ParseContext context, RankNode node) {

        if (!validateArgument(node)) {
            return Optional.empty();
        }

        RankNode leaf = node;

        while (leaf.getChildren().size() > 0) {
            leaf = leaf.getChildren().get(0);
        }

        Symbol symbol = leaf.getSymbol();

        if (symbol == null) {
            return Optional.empty();
        }

        if (symbol.getStatus() == SymbolStatus.REFERENCE) {
            context.schemaIndex().deleteSymbolReference(symbol);
        }

        symbol.setType(SymbolType.DIMENSION);
        symbol.setStatus(SymbolStatus.BUILTIN_REFERENCE);

        return Optional.empty();
    }

}

package ai.vespa.schemals.lsp.schema.completion.provider;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import org.eclipse.lsp4j.CompletionItem;

import ai.vespa.schemals.parser.Token.TokenType;
import ai.vespa.schemals.parser.ast.identifierStr;
import ai.vespa.schemals.parser.ast.identifierWithDashStr;
import ai.vespa.schemals.context.EventCompletionContext;
import ai.vespa.schemals.schemadocument.SchemaDocumentLexer;
import ai.vespa.schemals.schemadocument.SchemaDocument;
import ai.vespa.schemals.tree.SchemaNode;
import ai.vespa.schemals.index.Symbol;
import ai.vespa.schemals.index.Symbol.SymbolType;
import ai.vespa.schemals.lsp.schema.completion.utils.*;

/**
 * InheritsCompletionProvider 
 * Completion of the text " inherits " with <b>choice</b> on what to inherit. Note that choice is different from 
 * completion, choices are embedded in a snippet, while {@link InheritanceCompletion} takes care of standalone 
 * completion of these same items.
 */
public class InheritsCompletion implements CompletionProvider {
	private Optional<SchemaNode> match(EventCompletionContext context) {
        SchemaDocumentLexer lexer = context.document.lexer();
        SchemaNode match = lexer.matchBackwards(context.position, 1, false, TokenType.INHERITS);
        if (match != null)return Optional.empty();

        match = lexer.matchBackwards(context.position, 1, false, TokenType.DOCUMENT, TokenType.IDENTIFIER);
        if (match != null && match.getRange().getStart().getLine() == context.position.getLine()) {
            return Optional.of(match);
        }

        match = lexer.matchBackwards(context.position, 1, false, TokenType.SCHEMA, TokenType.IDENTIFIER);
        if (match != null && match.getRange().getStart().getLine() == context.position.getLine()) {
            return Optional.of(match);
        }

        match = lexer.matchBackwards(context.position, 1, false, TokenType.STRUCT, TokenType.IDENTIFIER);
        if (match != null && match.getRange().getStart().getLine() == context.position.getLine()) {
            return Optional.of(match);
        }

        match = lexer.matchBackwards(context.position, 1, false, TokenType.ANNOTATION, TokenType.IDENTIFIER);
        if (match != null && match.getRange().getStart().getLine() == context.position.getLine()) {
            return Optional.of(match);
        }

        match = lexer.matchBackwards(context.position, 1, false, TokenType.RANK_PROFILE, TokenType.IDENTIFIER);
        if (match != null && match.getRange().getStart().getLine() == context.position.getLine()) {
            return Optional.of(match);
        }

        return Optional.empty();
	}

	@Override
	public List<CompletionItem> getCompletionItems(EventCompletionContext context) {
        Optional<SchemaNode> matched = match(context);
        if (matched.isEmpty()) return List.of();

        List<String> suggestions = new ArrayList<>();

        TokenType nodeType = matched.get().getDirtyType();
        SymbolType equivalentSymbolType;

        switch (nodeType) {
            case DOCUMENT:
                equivalentSymbolType = SymbolType.DOCUMENT;
                break;
            case SCHEMA:
                equivalentSymbolType = SymbolType.SCHEMA;
                break;
            case RANK_PROFILE:
                equivalentSymbolType = SymbolType.RANK_PROFILE;
                break;
            case STRUCT:
                equivalentSymbolType = SymbolType.STRUCT;
                break;
            case ANNOTATION:
                equivalentSymbolType = SymbolType.ANNOTATION;
                break;
            default:
                return List.of();
        }

        String myIdentifierString = (matched.get().getNextSibling() != null 
                && (
                    matched.get().getNextSibling().isASTInstance(identifierStr.class)
                    ||  matched.get().getNextSibling().isASTInstance(identifierWithDashStr.class)
                )) ? matched.get().getNextSibling().getText() : "";

        if (nodeType == TokenType.DOCUMENT || nodeType == TokenType.RANK_PROFILE || nodeType == TokenType.SCHEMA) {
            suggestions = context.schemaIndex.listSymbolsInScope(null, equivalentSymbolType)
                .stream()
                .map(symbol -> symbol.getShortIdentifier())
                .filter(str -> !str.equals(myIdentifierString))
                .collect(Collectors.toCollection(ArrayList::new)); // to make it modifiable
        } else if (nodeType == TokenType.STRUCT || nodeType == TokenType.ANNOTATION) {
            Optional<Symbol> scope = context.schemaIndex.findSymbol(null, SymbolType.DOCUMENT, ((SchemaDocument)context.document).getSchemaIdentifier());

            if (scope.isPresent()) {
                suggestions = context.schemaIndex.listSymbolsInScope(scope.get(), equivalentSymbolType)
                    .stream()
                    .map(symbol -> symbol.getShortIdentifier())
                    .filter(str -> !str.equals(myIdentifierString))
                    .collect(Collectors.toCollection(ArrayList::new)); // to make it modifiable
            }
        }

        if (nodeType == TokenType.RANK_PROFILE && !suggestions.contains("default")) {
            suggestions.add("default");
        }

        if (suggestions.isEmpty()) {
            return List.of(CompletionUtils.constructSnippet("inherits", "inherits $0 "));
        }

        String choiceString = "${1|" + String.join(",", suggestions) + "|}";
        return List.of(CompletionUtils.constructSnippet("inherits", "inherits " + choiceString));
	}
}

package ai.vespa.schemals.index;

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import ai.vespa.schemals.index.Symbol.SymbolType;
import ai.vespa.schemals.parser.ast.DOT;

public class SchemaIndex {

    private PrintStream logger;

    private Map<SymbolType, List<Symbol>> symbolDefinitions = new HashMap<>();
    private Map<Symbol, List<Symbol>> symbolReferences = new HashMap<>();
    private Map<Symbol, Symbol> definitionOfReference = new HashMap<>();

    private InheritanceGraph<String> documentInheritanceGraph;
    private InheritanceGraph<Symbol> structInheritanceGraph;
    private InheritanceGraph<Symbol> rankProfileInheritanceGraph;
    
    public SchemaIndex(PrintStream logger) {
        this.logger = logger;
        this.documentInheritanceGraph = new InheritanceGraph<>();
        this.structInheritanceGraph = new InheritanceGraph<>();
        this.rankProfileInheritanceGraph = new InheritanceGraph<>();
    }

    /**
     * Returns the inheritance graph for documents in the index.
     *
     * @return the inheritance graph for documents
     */
    public InheritanceGraph<String> getDocumentInheritanceGraph() { return documentInheritanceGraph; }

    /**
     * Clears the index for symbols in the specified file
     *
     * @param fileURI the URI of the document to be cleared
     */
    public void clearDocument(String fileURI) {}

    /**
     * Searches for the specified symbol in the index.
     *
     * @param symbol The symbol to find, should be UNRESOLVED
     * @return An Optional containing the found symbol, or an empty Optional if the symbol is not found.
     */
    public Optional<Symbol> findSymbol(Symbol symbol) {
        var hits = findSymbols(symbol);
        if (hits.isEmpty())return Optional.empty();
        return Optional.of(hits.get(0));
    }

    /**
     * Searches for symbols in the schema index that match the given symbol.
     *
     * @param querySymbol The symbol to search for, should be UNRESOLVED
     * @return A list of symbols that match the given symbol.
     */
    public List<Symbol> findSymbols(Symbol querySymbol) {
        List<Symbol> result = new ArrayList<>();

        // First candidates are all symbols with correct type and correct short identifier
        List<Symbol> candidates = symbolDefinitions.get(querySymbol.getType())
                                                   .stream()
                                                   .filter(symbolDefinition -> symbolDefinition.getShortIdentifier().equals(querySymbol.getShortIdentifier()))
                                                   .toList();

        Symbol scope = querySymbol.getScope();

        while (scope != null) {
            for (Symbol candidate : candidates) {
                // Check if candidate is in this scope
                if (isInScope(candidate, scope))result.add(candidate);
            }

            if (!result.isEmpty()) return result;
            scope = scope.getScope();
        }

        return result;
    }

    private boolean isInScope(Symbol symbol, Symbol scope) {
        if (scope.getType() == SymbolType.RANK_PROFILE) {
            return !rankProfileInheritanceGraph.findFirstMatches(scope, 
                    rankProfileDefinitionNode -> Boolean.valueOf(rankProfileDefinitionNode.equals(symbol.getScope()))).isEmpty();
        } else if (scope.getType() == SymbolType.STRUCT) {
            return !structInheritanceGraph.findFirstMatches(scope, 
                    structDefinitionNode -> Boolean.valueOf(structDefinitionNode.equals(symbol.getScope()))).isEmpty();
        } else if (scope.getType() == SymbolType.SCHEMA || scope.getType() == SymbolType.DOCUMENT) {
            return !documentInheritanceGraph.findFirstMatches(scope.getFileURI(), 
                ancestorURI -> {
                    return Boolean.valueOf(symbol.getFileURI().equals(ancestorURI));
                }
            ).isEmpty();
        }

        return scope.equals(symbol.getScope());
    }

    /**
     * Retrieves the definition of a symbol from a map
     *
     * @param symbol The symbol to retrieve the definition for.
     * @return An Optional containing the definition of the symbol, or an empty Optional if the symbol is not found.
     */
    public Optional<Symbol> getSymbolDefinition(Symbol symbol) {
        return Optional.empty();
    }

    /**
     * Returns a list of symbol references for the given symbol.
     *
     * @param symbol The symbol for which to retrieve the references.
     * @return A list of symbol references.
     */
    public List<Symbol> getSymbolReferences(Symbol symbol) {
        return new ArrayList<>();
    }

    /**
     * Inserts a symbol definition into the schema index.
     *
     * @param symbol the symbol to be inserted
     */
    public void insertSymbolDefinition(Symbol symbol) {

    }

    /**
     * Inserts a symbol reference into the schema index.
     * Make sure that he definition exists before inserting
     *
     * @param definition the symbol being defined
     * @param reference the symbol being referenced
     */
    public void insertSymbolReference(Symbol definition, Symbol reference) {

    }

    /**
     * Tries to register document inheritance between a child document and a parent document.
     *
     * @param childURI  the URI of the child document
     * @param parentURI the URI of the parent document
     * @return true if the document inheritance was successfully registered, false otherwise
     */
    public boolean tryRegisterDocumentInheritance(String childURI, String parentURI) {
        return documentInheritanceGraph.addInherits(childURI, parentURI);
    }

    /**
     * Tries to register struct inheritance between the child symbol and the parent symbol.
     *
     * @param childSymbol The child symbol representing the struct.
     * @param parentSymbol The parent symbol representing the inherited struct.
     * @return {@code true} if the struct inheritance was successfully registered, {@code false} otherwise.
     */
    public boolean tryRegisterStructInheritance(Symbol childSymbol, Symbol parentSymbol) {
        return structInheritanceGraph.addInherits(childSymbol, parentSymbol);
    }

    /**
     * Tries to register the inheritance relationship between a child rank profile and a parent rank profile.
     *
     * @param childSymbol The symbol representing the child rank profile.
     * @param parentSymbol The symbol representing the parent rank profile.
     * @return {@code true} if the inheritance relationship was successfully registered, {@code false} otherwise.
     */
    public boolean tryRegisterRankProfileInheritance(Symbol childSymbol, Symbol parentSymbol) {
        return rankProfileInheritanceGraph.addInherits(childSymbol, parentSymbol);
    }

    /**
     * Searches for symbols in the specified scope of the given type.
     *
     * @param scope The symbol representing the scope to search in.
     * @param type The type of symbols to find.
     * @return A list of symbols found in the specified scope and of the given type.
     */
    public List<Symbol> findSymbolsInScope(Symbol scope, SymbolType type) {
        return new ArrayList<>();
    }
}

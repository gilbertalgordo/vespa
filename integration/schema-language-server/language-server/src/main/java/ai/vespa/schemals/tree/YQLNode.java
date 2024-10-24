package ai.vespa.schemals.tree;

import org.eclipse.lsp4j.Position;
import org.eclipse.lsp4j.Range;

import ai.vespa.schemals.parser.yqlplus.Node;
import ai.vespa.schemals.tree.YQL.YQLUtils;
import ai.vespa.schemals.tree.grouping.GroupingUtils;

public class YQLNode extends ai.vespa.schemals.tree.Node {
    
    private Node originalYQLNode;
    private ai.vespa.schemals.parser.grouping.Node originalGroupingNode;

    public YQLNode(Node node, Position offset) {
        super(LanguageType.YQLPlus, CSTUtils.addPositionToRange(offset, YQLUtils.getNodeRange(node)), node.isDirty());
        originalYQLNode = node;

        for (Node child : node.children()) {
            addChild(new YQLNode(child, offset));
        }
    }

    public YQLNode(ai.vespa.schemals.parser.grouping.Node node, Position rangeOffset) {
        super(LanguageType.GROUPING, CSTUtils.addPositionToRange(rangeOffset, GroupingUtils.getNodeRange(node)), node.isDirty());
        originalGroupingNode = node;

        for (ai.vespa.schemals.parser.grouping.Node child : node.children()) {
            addChild(new YQLNode(child, rangeOffset));
        }
    }

    public YQLNode(Range range) {
        super(LanguageType.CUSTOM, range, false);
    }

    public Range setRange(Range range) {
        this.range = range;
        return range;
    }

    public String getText() {
        throw new UnsupportedOperationException("Not implemented");
    }

    @Override
    public boolean isYQLNode() { return true; }

    @Override
    public YQLNode getYQLNode() { return this; }

    @Override
    public Class<?> getASTClass() {
        if (language == LanguageType.CUSTOM) return YQLNode.class;
        if (language == LanguageType.YQLPlus) return originalYQLNode.getClass();
        if (language == LanguageType.GROUPING) return originalGroupingNode.getClass();
        
        throw new RuntimeException("The YQLNode has an invalid languageType");
    }

    @Override
    public int getBeginOffset() {
        if (language == LanguageType.YQLPlus) return originalYQLNode.getBeginOffset();
        if (language == LanguageType.GROUPING) return originalGroupingNode.getBeginOffset();

        throw new RuntimeException("Could not find the begin offset of YQLNode.");
    }

    public String toString() {
        Range range = getRange();
        Position start = range.getStart();
        Position end = range.getEnd();
        return "YQLNode(" + getASTClass() + ", " + start.getLine() + "," + start.getCharacter() + "->" + end.getLine() + "," + end.getCharacter() + ")";
    }
}

#pragma once
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <memory>

// ─── Node Types ───────────────────────────────────────────────────────────────
enum class NodeType {
    Document,
    Heading,       // h1-h6
    Paragraph,
    CodeBlock,     // fenced ```lang
    InlineCode,    // `code`
    Bold,
    Italic,
    BoldItalic,
    Link,
    Image,
    HorizontalRule,
    BlockQuote,
    ListItem,
    OrderedList,
    UnorderedList,
    Table,
    TableRow,
    TableCell,
    Chart,         // ```chart ... ```
    Text,
    LineBreak,
    HtmlBlock,
};

struct MarkdownNode {
    NodeType type = NodeType::Text;
    QString  text;            // raw text content
    int      level = 0;       // heading level 1-6
    QString  language;        // code block language
    QString  href;            // link/image url
    QString  alt;             // image alt text
    QString  id;              // anchor id generated from heading text
    bool     isHeader = false;// table cell header
    int      colSpan = 1;
    QVariantMap chartData;    // parsed chart definition

    QVector<std::shared_ptr<MarkdownNode>> children;

    using Ptr = std::shared_ptr<MarkdownNode>;
    static Ptr make(NodeType t) { return std::make_shared<MarkdownNode>(MarkdownNode{t}); }
};

// ─── TOC Entry ────────────────────────────────────────────────────────────────
struct TocEntry {
    int     level;   // 1-6
    QString title;
    QString anchor;  // matches MarkdownNode::id
    int     nodeIndex; // position in flat node list for scroll
};

// ─── Parser ───────────────────────────────────────────────────────────────────
class MarkdownParser {
public:
    MarkdownParser() = default;

    MarkdownNode::Ptr parse(const QString &markdown);
    QVector<TocEntry> extractToc() const { return m_toc; }

private:
    QVector<TocEntry> m_toc;
    int m_nodeIndex = 0;

    // block-level passes
    QVector<MarkdownNode::Ptr> parseBlocks(const QStringList &lines);
    MarkdownNode::Ptr          parseHeading(const QString &line);
    MarkdownNode::Ptr          parseFencedCode(const QStringList &lines, int &i);
    MarkdownNode::Ptr          parseBlockQuote(const QStringList &lines, int &i);
    MarkdownNode::Ptr          parseList(const QStringList &lines, int &i, bool ordered);
    MarkdownNode::Ptr          parseTable(const QStringList &lines, int &i);
    MarkdownNode::Ptr          parseParagraph(const QStringList &lines, int &i);

    // inline passes
    QVector<MarkdownNode::Ptr> parseInline(const QString &text);

    // helpers
    static QString generateAnchor(const QString &heading);
    static bool isOrderedListItem(const QString &line, int &indent, int &num, QString &content);
    static bool isUnorderedListItem(const QString &line, int &indent, QString &content);
    static bool isTableSeparator(const QString &line);
};

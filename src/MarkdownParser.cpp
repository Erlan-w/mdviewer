#include "MarkdownParser.h"
#include <QRegularExpression>
#include <QStringList>
#include <QCryptographicHash>

// ─── Public ───────────────────────────────────────────────────────────────────
MarkdownNode::Ptr MarkdownParser::parse(const QString &markdown) {
    m_toc.clear();
    m_nodeIndex = 0;
    auto doc = MarkdownNode::make(NodeType::Document);
    QStringList lines = markdown.split('\n');
    auto blocks = parseBlocks(lines);
    doc->children = blocks;
    return doc;
}

// ─── Block Parsing ────────────────────────────────────────────────────────────
QVector<MarkdownNode::Ptr> MarkdownParser::parseBlocks(const QStringList &lines) {
    QVector<MarkdownNode::Ptr> nodes;
    int i = 0;
    while (i < lines.size()) {
        const QString &line = lines[i];
        const QString trimmed = line.trimmed();

        if (trimmed.isEmpty()) { ++i; continue; }

        // Heading
        if (trimmed.startsWith('#')) {
            auto node = parseHeading(trimmed);
            if (node) { nodes.append(node); ++i; continue; }
        }

        // Setext headings (== or --)
        if (i + 1 < lines.size()) {
            const QString next = lines[i + 1].trimmed();
            if (next.count('=') == next.size() && !next.isEmpty()) {
                auto n = MarkdownNode::make(NodeType::Heading); n->level = 1;
                n->text = trimmed;
                n->id = generateAnchor(trimmed);
                n->children = parseInline(trimmed);
                m_toc.append({1, trimmed, n->id, m_nodeIndex++});
                nodes.append(n); i += 2; continue;
            }
            if (next.count('-') == next.size() && next.size() >= 2) {
                auto n = MarkdownNode::make(NodeType::Heading); n->level = 2;
                n->text = trimmed;
                n->id = generateAnchor(trimmed);
                n->children = parseInline(trimmed);
                m_toc.append({2, trimmed, n->id, m_nodeIndex++});
                nodes.append(n); i += 2; continue;
            }
        }

        // Fenced code block
        if (trimmed.startsWith("```") || trimmed.startsWith("~~~")) {
            nodes.append(parseFencedCode(lines, i)); continue;
        }

        // Horizontal rule
        static QRegularExpression hrRe(R"(^(\*{3,}|-{3,}|_{3,})\s*$)");
        if (hrRe.match(trimmed).hasMatch()) {
            nodes.append(MarkdownNode::make(NodeType::HorizontalRule));
            ++i; continue;
        }

        // BlockQuote
        if (trimmed.startsWith('>')) {
            nodes.append(parseBlockQuote(lines, i)); continue;
        }

        // Table
        if (trimmed.contains('|') && i + 1 < lines.size() && isTableSeparator(lines[i+1])) {
            nodes.append(parseTable(lines, i)); continue;
        }

        // Ordered list
        int indent, num; QString content;
        if (isOrderedListItem(trimmed, indent, num, content)) {
            nodes.append(parseList(lines, i, true)); continue;
        }

        // Unordered list
        if (isUnorderedListItem(trimmed, indent, content)) {
            nodes.append(parseList(lines, i, false)); continue;
        }

        // Paragraph
        nodes.append(parseParagraph(lines, i));
    }
    return nodes;
}

MarkdownNode::Ptr MarkdownParser::parseHeading(const QString &line) {
    static QRegularExpression re(R"(^(#{1,6})\s+(.+?)(?:\s+#+\s*)?$)");
    auto m = re.match(line);
    if (!m.hasMatch()) return nullptr;
    auto node = MarkdownNode::make(NodeType::Heading);
    node->level = m.captured(1).size();
    node->text  = m.captured(2);
    node->id    = generateAnchor(node->text);
    node->children = parseInline(node->text);
    m_toc.append({node->level, node->text, node->id, m_nodeIndex++});
    return node;
}

MarkdownNode::Ptr MarkdownParser::parseFencedCode(const QStringList &lines, int &i) {
    QString fence = lines[i].trimmed();
    QString fenceChar = fence.startsWith("```") ? "```" : "~~~";
    QString lang = fence.mid(fenceChar.size()).trimmed().toLower();
    ++i;
    QStringList codeLines;
    while (i < lines.size() && !lines[i].trimmed().startsWith(fenceChar)) {
        codeLines.append(lines[i]);
        ++i;
    }
    if (i < lines.size()) ++i; // consume closing fence

    QString code = codeLines.join('\n');

    // Chart block
    if (lang == "chart" || lang == "mermaid" || lang == "chartjs") {
        auto node = MarkdownNode::make(NodeType::Chart);
        node->text     = code;
        node->language = lang;
        // Parse simple chart definition: type:bar\ntitle:Sales\ndata:10,20,30\nlabels:A,B,C
        QVariantMap cd;
        cd["raw"] = code;
        cd["type"] = "bar";
        for (const QString &cl : codeLines) {
            auto parts = cl.split(':', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                cd[parts[0].trimmed().toLower()] = parts.mid(1).join(':').trimmed();
            }
        }
        node->chartData = cd;
        return node;
    }

    auto node = MarkdownNode::make(NodeType::CodeBlock);
    node->text     = code;
    node->language = lang;
    return node;
}

MarkdownNode::Ptr MarkdownParser::parseBlockQuote(const QStringList &lines, int &i) {
    QStringList content;
    while (i < lines.size() && lines[i].trimmed().startsWith('>')) {
        QString l = lines[i].trimmed();
        l = l.mid(1); if (l.startsWith(' ')) l = l.mid(1);
        content.append(l);
        ++i;
    }
    auto node = MarkdownNode::make(NodeType::BlockQuote);
    node->children = parseBlocks(content);
    return node;
}

MarkdownNode::Ptr MarkdownParser::parseList(const QStringList &lines, int &i, bool ordered) {
    auto list = MarkdownNode::make(ordered ? NodeType::OrderedList : NodeType::UnorderedList);
    int indent, num; QString content;
    while (i < lines.size()) {
        const QString &line = lines[i].trimmed();
        bool isItem = ordered ? isOrderedListItem(line, indent, num, content)
                              : isUnorderedListItem(line, indent, content);
        if (!isItem && !line.startsWith("  ")) break;
        if (isItem) {
            auto item = MarkdownNode::make(NodeType::ListItem);
            item->children = parseInline(content);
            list->children.append(item);
            ++i;
        } else {
            // continuation line — append to last item
            if (!list->children.isEmpty()) {
                auto &last = list->children.last();
                auto extra = MarkdownNode::make(NodeType::Text);
                extra->text = " " + line.trimmed();
                last->children.append(extra);
            }
            ++i;
        }
    }
    return list;
}

MarkdownNode::Ptr MarkdownParser::parseTable(const QStringList &lines, int &i) {
    auto table = MarkdownNode::make(NodeType::Table);
    // Header row
    {
        auto row = MarkdownNode::make(NodeType::TableRow);
        QStringList cells = lines[i].split('|', Qt::SkipEmptyParts);
        for (auto &c : cells) {
            auto cell = MarkdownNode::make(NodeType::TableCell);
            cell->isHeader = true;
            cell->children = parseInline(c.trimmed());
            row->children.append(cell);
        }
        table->children.append(row);
        ++i; // separator
        ++i;
    }
    // Data rows
    while (i < lines.size()) {
        const QString &line = lines[i].trimmed();
        if (!line.contains('|')) break;
        auto row = MarkdownNode::make(NodeType::TableRow);
        QStringList cells = line.split('|', Qt::SkipEmptyParts);
        for (auto &c : cells) {
            auto cell = MarkdownNode::make(NodeType::TableCell);
            cell->children = parseInline(c.trimmed());
            row->children.append(cell);
        }
        table->children.append(row);
        ++i;
    }
    return table;
}

MarkdownNode::Ptr MarkdownParser::parseParagraph(const QStringList &lines, int &i) {
    QStringList pLines;
    while (i < lines.size()) {
        const QString &line = lines[i];
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) { ++i; break; }
        if (trimmed.startsWith('#') || trimmed.startsWith("```") ||
            trimmed.startsWith("~~~") || trimmed.startsWith('>') ||
            trimmed.startsWith("---") || trimmed.startsWith("***") ||
            trimmed.startsWith("___")) break;
        pLines.append(trimmed);
        ++i;
    }
    auto node = MarkdownNode::make(NodeType::Paragraph);
    node->children = parseInline(pLines.join(' '));
    return node;
}

// ─── Inline Parsing ───────────────────────────────────────────────────────────
QVector<MarkdownNode::Ptr> MarkdownParser::parseInline(const QString &text) {
    QVector<MarkdownNode::Ptr> nodes;
    int pos = 0;
    const int len = text.size();

    auto flushText = [&](int from, int to) {
        if (from < to) {
            auto n = MarkdownNode::make(NodeType::Text);
            n->text = text.mid(from, to - from);
            nodes.append(n);
        }
    };

    while (pos < len) {
        // Image  ![alt](url)
        if (pos + 1 < len && text[pos] == '!' && text[pos+1] == '[') {
            int altEnd = text.indexOf(']', pos + 2);
            if (altEnd != -1 && altEnd + 1 < len && text[altEnd+1] == '(') {
                int urlEnd = text.indexOf(')', altEnd + 2);
                if (urlEnd != -1) {
                    auto n = MarkdownNode::make(NodeType::Image);
                    n->alt  = text.mid(pos + 2, altEnd - pos - 2);
                    n->href = text.mid(altEnd + 2, urlEnd - altEnd - 2);
                    nodes.append(n);
                    pos = urlEnd + 1; continue;
                }
            }
        }

        // Link  [text](url)
        if (text[pos] == '[') {
            int labelEnd = text.indexOf(']', pos + 1);
            if (labelEnd != -1 && labelEnd + 1 < len && text[labelEnd+1] == '(') {
                int urlEnd = text.indexOf(')', labelEnd + 2);
                if (urlEnd != -1) {
                    auto n = MarkdownNode::make(NodeType::Link);
                    n->text = text.mid(pos + 1, labelEnd - pos - 1);
                    n->href = text.mid(labelEnd + 2, urlEnd - labelEnd - 2);
                    n->children = parseInline(n->text);
                    nodes.append(n);
                    pos = urlEnd + 1; continue;
                }
            }
        }

        // Inline code `code`
        if (text[pos] == '`') {
            int end = text.indexOf('`', pos + 1);
            if (end != -1) {
                auto n = MarkdownNode::make(NodeType::InlineCode);
                n->text = text.mid(pos + 1, end - pos - 1);
                nodes.append(n);
                pos = end + 1; continue;
            }
        }

        // Bold+Italic ***
        if (pos + 2 < len && text[pos] == '*' && text[pos+1] == '*' && text[pos+2] == '*') {
            int end = text.indexOf("***", pos + 3);
            if (end != -1) {
                auto n = MarkdownNode::make(NodeType::BoldItalic);
                n->text = text.mid(pos + 3, end - pos - 3);
                n->children = parseInline(n->text);
                nodes.append(n); pos = end + 3; continue;
            }
        }

        // Bold **
        if (pos + 1 < len && text[pos] == '*' && text[pos+1] == '*') {
            int end = text.indexOf("**", pos + 2);
            if (end != -1) {
                auto n = MarkdownNode::make(NodeType::Bold);
                n->text = text.mid(pos + 2, end - pos - 2);
                n->children = parseInline(n->text);
                nodes.append(n); pos = end + 2; continue;
            }
        }

        // Italic *
        if (text[pos] == '*' && (pos == 0 || text[pos-1] != '*')) {
            int end = text.indexOf('*', pos + 1);
            if (end != -1 && (end + 1 >= len || text[end+1] != '*')) {
                auto n = MarkdownNode::make(NodeType::Italic);
                n->text = text.mid(pos + 1, end - pos - 1);
                n->children = parseInline(n->text);
                nodes.append(n); pos = end + 1; continue;
            }
        }

        // Bold __
        if (pos + 1 < len && text[pos] == '_' && text[pos+1] == '_') {
            int end = text.indexOf("__", pos + 2);
            if (end != -1) {
                auto n = MarkdownNode::make(NodeType::Bold);
                n->text = text.mid(pos + 2, end - pos - 2);
                n->children = parseInline(n->text);
                nodes.append(n); pos = end + 2; continue;
            }
        }

        // Italic _
        if (text[pos] == '_') {
            int end = text.indexOf('_', pos + 1);
            if (end != -1) {
                auto n = MarkdownNode::make(NodeType::Italic);
                n->text = text.mid(pos + 1, end - pos - 1);
                n->children = parseInline(n->text);
                nodes.append(n); pos = end + 1; continue;
            }
        }

        // plain text char
        int start = pos;
        while (pos < len && text[pos] != '[' && text[pos] != '!' &&
               text[pos] != '`' && text[pos] != '*' && text[pos] != '_') ++pos;
        flushText(start, pos);
    }
    return nodes;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
QString MarkdownParser::generateAnchor(const QString &heading) {
    QString a = heading.toLower();
    a.replace(QRegularExpression(R"([^\w\s-])"), "");
    a.replace(QRegularExpression(R"(\s+)"), "-");
    a.replace(QRegularExpression(R"(-+)"), "-");
    a = a.trimmed();
    if (a.startsWith('-')) a = a.mid(1);
    if (a.endsWith('-'))   a.chop(1);
    return a;
}

bool MarkdownParser::isOrderedListItem(const QString &line, int &indent, int &num, QString &content) {
    static QRegularExpression re(R"(^(\s*)(\d+)[.)]\s+(.*)$)");
    auto m = re.match(line);
    if (!m.hasMatch()) return false;
    indent  = m.captured(1).size();
    num     = m.captured(2).toInt();
    content = m.captured(3);
    return true;
}

bool MarkdownParser::isUnorderedListItem(const QString &line, int &indent, QString &content) {
    static QRegularExpression re(R"(^(\s*)[*\-+]\s+(.*)$)");
    auto m = re.match(line);
    if (!m.hasMatch()) return false;
    indent  = m.captured(1).size();
    content = m.captured(2);
    return true;
}

bool MarkdownParser::isTableSeparator(const QString &line) {
    static QRegularExpression re(R"(^\s*\|?\s*:?-+:?\s*(\|\s*:?-+:?\s*)*\|?\s*$)");
    return re.match(line.trimmed()).hasMatch();
}

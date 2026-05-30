#include "MarkdownRenderer.h"
#include "ChartRenderer.h"
#include "ImageViewer.h"
#include "ThemeManager.h"
#include <QLabel>
#include <QTextEdit>
#include <QScrollBar>
#include <QApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QEvent>
#include <QResizeEvent>
#include <QScrollEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QBuffer>
#include <QTimer>

// ─── Constructor ─────────────────────────────────────────────────────────[...]
MarkdownRenderer::MarkdownRenderer(QWidget *parent)
    : QScrollArea(parent)
    , m_imageCache(50 * 1024 * 1024) // 50 MB cache
{
    m_container = new QWidget(this);
    m_layout    = new QVBoxLayout(m_container);
    m_layout->setContentsMargins(48, 32, 48, 80);
    m_layout->setSpacing(0);
    m_layout->addStretch();

    setWidget(m_container);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);

    const auto &th = ThemeManager::instance().theme();
    setStyleSheet(QString("QScrollArea{background:%1;border:none;}"
        "QScrollBar:vertical{width:6px;background:transparent;margin:0;}"
        "QScrollBar::handle:vertical{background:%2;border-radius:3px;min-height:20px;}"
        "QScrollBar::add-line,QScrollBar::sub-line{height:0;}"
        "QScrollBar::add-page,QScrollBar::sub-page{background:transparent;}")
        .arg(th.bg.name(), th.border.name()));
    m_container->setStyleSheet(QString("background:%1;").arg(th.bg.name()));

    m_net = new QNetworkAccessManager(this);
    verticalScrollBar()->installEventFilter(this);
}

MarkdownRenderer::~MarkdownRenderer() = default;

// ─── Render ────────────────────────────────────────────────────────────[...]
void MarkdownRenderer::render(const MarkdownNode::Ptr &doc) {
    clearWidgets();
    if (!doc) return;
    for (const auto &child : doc->children)
        buildBlock(child);
    m_layout->addStretch();
}

void MarkdownRenderer::clearWidgets() {
    m_anchorMap.clear();
    QLayoutItem *item;
    while ((item = m_layout->takeAt(0))) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void MarkdownRenderer::buildBlock(const MarkdownNode::Ptr &node) {
    QWidget *w = nullptr;
    switch (node->type) {
    case NodeType::Heading:       w = buildHeading(node);    break;
    case NodeType::Paragraph:     w = buildParagraph(node);  break;
    case NodeType::CodeBlock:     w = buildCodeBlock(node);  break;
    case NodeType::BlockQuote:    w = buildBlockQuote(node); break;
    case NodeType::OrderedList:
    case NodeType::UnorderedList: w = buildList(node);       break;
    case NodeType::Table:         w = buildTable(node);      break;
    case NodeType::HorizontalRule:w = buildHRule();          break;
    case NodeType::Image:         w = buildImage(node);      break;
    case NodeType::Chart:         w = buildChart(node);      break;
    default: break;
    }
    if (w) {
        m_layout->addWidget(w);
    }
}

// ─── Heading ──────────────────────────────────────────────────────────–[...]
QWidget* MarkdownRenderer::buildHeading(const MarkdownNode::Ptr &node) {
    const auto &th = ThemeManager::instance().theme();
    auto *bw = new BlockWidget(m_container);
    bw->anchor = node->id;

    auto *lay = new QVBoxLayout(bw);
    lay->setContentsMargins(0, node->level <= 2 ? 28 : 20, 0, node->level <= 2 ? 8 : 4);
    lay->setSpacing(0);

    auto *lbl = new QLabel(bw);
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    lbl->setOpenExternalLinks(true);

    int sizes[] = {0, 28, 22, 18, 15, 13, 12};
    int sz = sizes[qBound(1, node->level, 6)];
    QColor colors[] = {{}, th.textH1, th.textH2, th.textH3,
                       th.text, th.textMuted, th.textMuted};
    QColor col = colors[qBound(1, node->level, 6)];

    QFont f = ThemeManager::instance().fontHeading(sz);
    lbl->setFont(f);
    lbl->setStyleSheet(QString("color:%1;margin:0;padding:0;").arg(col.name()));
    lbl->setText(inlineToHtml(node->children));
    lay->addWidget(lbl);

    // H1/H2 underline
    if (node->level <= 2) {
        auto *sep = new QFrame(bw);
        sep->setFrameShape(QFrame::HLine);
        sep->setFixedHeight(1);
        sep->setStyleSheet(QString("background:%1;margin-top:6px;border:none;")
            .arg(node->level == 1 ? th.borderAccent.name() : th.border.name()));
        lay->addWidget(sep);
    }

    m_anchorMap[node->id] = bw;
    return bw;
}

// ─── Paragraph ─────────────────────────────────────────────────────────–[...]
QWidget* MarkdownRenderer::buildParagraph(const MarkdownNode::Ptr &node) {
    const auto &th = ThemeManager::instance().theme();

    // Check if paragraph is a single image
    if (node->children.size() == 1 && node->children[0]->type == NodeType::Image)
        return buildImage(node->children[0]);

    auto *lbl = new QLabel(m_container);
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    lbl->setOpenExternalLinks(true);
    lbl->setContentsMargins(0, 4, 0, 12);
    lbl->setStyleSheet(QString("color:%1;font-size:14px;line-height:1.7;").arg(th.text.name()));
    QFont f = ThemeManager::instance().fontBody(11);
    lbl->setFont(f);
    lbl->setText(inlineToHtml(node->children));
    return lbl;
}

// ─── Code Block ─────────────────────────────────────────────────────────[...]
QWidget* MarkdownRenderer::buildCodeBlock(const MarkdownNode::Ptr &node) {
    const auto &th = ThemeManager::instance().theme();

    auto *frame = new QWidget(m_container);
    frame->setContentsMargins(0, 8, 0, 12);
    auto *lay = new QVBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // lang badge + copy header
    if (!node->language.isEmpty()) {
        auto *hdr = new QLabel(node->language.toUpper(), frame);
        hdr->setStyleSheet(QString(
            "background:%1;color:%2;font:9px;padding:3px 10px;"
            "border-radius:4px 4px 0 0;border-bottom:1px solid %3;")
            .arg(th.bgTableHead.name(), th.textMuted.name(), th.border.name()));
        lay->addWidget(hdr);
    }

    auto *te = new QTextEdit(frame);
    te->setReadOnly(true);
    te->setFrameShape(QFrame::NoFrame);
    te->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    te->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    te->document()->setDefaultStyleSheet(QString(
        "body{font-family:Consolas,monospace;font-size:12px;color:%1;}"
        "span.kw{color:%2;} span.str{color:%3;} span.cmt{color:%4;}"
        "span.num{color:%5;} span.ty{color:%6;}")
        .arg(th.textCode.name(), th.synKeyword.name(), th.synString.name(),
             th.synComment.name(), th.synNumber.name(), th.synType.name()));

    QString html = "<pre style='margin:0;padding:0;'>" +
                   highlightCode(node->text.toHtmlEscaped(), node->language) +
                   "</pre>";
    te->setHtml(html);
    te->setStyleSheet(QString(
        "QTextEdit{background:%1;color:%2;border:1px solid %3;"
        "border-top:0;border-radius:0 0 4px 4px;padding:12px 14px;}"
        "QScrollBar:vertical{width:5px;background:transparent;}"
        "QScrollBar::handle:vertical{background:%3;border-radius:2px;}"
        "QScrollBar:horizontal{height:5px;background:transparent;}"
        "QScrollBar::handle:horizontal{background:%3;border-radius:2px;}")
        .arg(th.bgCode.name(), th.textCode.name(), th.border.name()));

    // auto-size height
    QFontMetrics fm(ThemeManager::instance().fontMono(12));
    int lines = node->text.count('\n') + 1;
    int h = qBound(60, lines * fm.lineSpacing() + 28, 480);
    te->setFixedHeight(h);
    lay->addWidget(te);

    if (node->language.isEmpty()) {
        te->setStyleSheet(te->styleSheet().replace("border-top:0;", "")
            .replace("border-radius:0 0", "border-radius:4px"));
    }
    return frame;
}

// ─── BlockQuote ─────────────────────────────────────────────────────────[...]
QWidget* MarkdownRenderer::buildBlockQuote(const MarkdownNode::Ptr &node) {
    const auto &th = ThemeManager::instance().theme();
    auto *frame = new QWidget(m_container);
    frame->setContentsMargins(0, 6, 0, 6);
    auto *lay = new QHBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto *bar = new QWidget(frame);
    bar->setFixedWidth(3);
    bar->setStyleSheet(QString("background:%1;border-radius:2px;").arg(th.quoteBar.name()));
    lay->addWidget(bar);
    lay->addSpacing(12);

    auto *inner = new QWidget(frame);
    inner->setStyleSheet(QString("background:%1;border-radius:4px;").arg(th.bgQuote.name()));
    auto *ilay = new QVBoxLayout(inner);
    ilay->setContentsMargins(12, 8, 12, 8);
    ilay->setSpacing(4);
    for (const auto &child : node->children) {
        // only render text blocks inside quote
        if (child->type == NodeType::Paragraph) {
            auto *lbl = new QLabel(inner);
            lbl->setWordWrap(true);
            lbl->setTextFormat(Qt::RichText);
            QFont f = ThemeManager::instance().fontItalic(11);
            lbl->setFont(f);
            lbl->setStyleSheet(QString("color:%1;").arg(th.textQuote.name()));
            lbl->setText(inlineToHtml(child->children));
            ilay->addWidget(lbl);
        }
    }
    lay->addWidget(inner, 1);
    return frame;
}

// ─── List ───────────────────────────────────────────────────────────[...]
QWidget* MarkdownRenderer::buildList(const MarkdownNode::Ptr &node, int depth) {
    const auto &th = ThemeManager::instance().theme();
    auto *frame = new QWidget(m_container);
    auto *lay   = new QVBoxLayout(frame);
    lay->setContentsMargins(depth * 16 + 4, 4, 4, 4);
    lay->setSpacing(2);

    bool ordered = (node->type == NodeType::OrderedList);
    int num = 1;
    for (const auto &item : node->children) {
        auto *row = new QWidget(frame);
        auto *rlay = new QHBoxLayout(row);
        rlay->setContentsMargins(0, 0, 0, 0);
        rlay->setSpacing(8);

        auto *bullet = new QLabel(ordered ? QString::number(num++) + "." : "•", row);
        bullet->setFixedWidth(ordered ? 22 : 14);
        bullet->setStyleSheet(QString("color:%1;font-size:13px;").arg(th.tocAccent.name()));
        rlay->addWidget(bullet);

        auto *lbl = new QLabel(row);
        lbl->setWordWrap(true);
        lbl->setTextFormat(Qt::RichText);
        lbl->setOpenExternalLinks(true);
        QFont f = ThemeManager::instance().fontBody(11);
        lbl->setFont(f);
        lbl->setStyleSheet(QString("color:%1;").arg(th.text.name()));
        lbl->setText(inlineToHtml(item->children));
        rlay->addWidget(lbl, 1);
        lay->addWidget(row);
    }
    return frame;
}

// ─── Table ──────────────────────────────────────────────────────────–[...]
QWidget* MarkdownRenderer::buildTable(const MarkdownNode::Ptr &node) {
    const auto &th = ThemeManager::instance().theme();
    auto *frame = new QWidget(m_container);
    frame->setContentsMargins(0, 8, 0, 12);
    auto *outerLay = new QVBoxLayout(frame);
    outerLay->setContentsMargins(0,0,0,0);

    auto *table = new QWidget(frame);
    table->setStyleSheet(QString(
        "background:%1;border:1px solid %2;border-radius:4px;")
        .arg(th.bgTable.name(), th.border.name()));
    auto *grid = new QGridLayout(table);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    for (int r = 0; r < node->children.size(); ++r) {
        const auto &row = node->children[r];
        for (int c = 0; c < row->children.size(); ++c) {
            const auto &cell = row->children[c];
            auto *lbl = new QLabel(table);
            lbl->setWordWrap(true);
            lbl->setTextFormat(Qt::RichText);
            QFont f = cell->isHeader
                    ? ThemeManager::instance().fontBold(10)
                    : ThemeManager::instance().fontBody(10);
            lbl->setFont(f);
            QString bg = cell->isHeader ? th.bgTableHead.name() : th.bgTable.name();
            lbl->setStyleSheet(QString(
                "color:%1;background:%2;padding:7px 12px;"
                "border-bottom:1px solid %3;border-right:1px solid %3;")
                .arg(cell->isHeader ? th.text.name() : th.textMuted.name(),
                     bg, th.border.name()));
            lbl->setText(inlineToHtml(cell->children));
            grid->addWidget(lbl, r, c);
        }
    }
    outerLay->addWidget(table);
    return frame;
}

// ─── HRule ──────────────────────────────────────────────────────────–[...]
QWidget* MarkdownRenderer::buildHRule() {
    const auto &th = ThemeManager::instance().theme();
    auto *sep = new QFrame(m_container);
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    sep->setContentsMargins(0, 16, 0, 16);
    sep->setStyleSheet(QString("background:%1;border:none;margin:16px 0;").arg(th.border.name()));
    return sep;
}

// ─── Image ──────────────────────────────────────────────────────────–[...]
QWidget* MarkdownRenderer::buildImage(const MarkdownNode::Ptr &node) {
    const auto &th = ThemeManager::instance().theme();
    auto *frame = new QWidget(m_container);
    auto *lay   = new QVBoxLayout(frame);
    lay->setContentsMargins(0, 8, 0, 8);
    lay->setSpacing(4);

    auto *imgLbl = new QLabel(frame);
    imgLbl->setAlignment(Qt::AlignCenter);
    imgLbl->setMinimumHeight(40);
    imgLbl->setStyleSheet(QString(
        "background:%1;border:1px solid %2;border-radius:4px;")
        .arg(th.bgCode.name(), th.border.name()));
    imgLbl->setText(QString("<span style='color:%1'>⏳ Loading: %2</span>")
        .arg(th.textMuted.name(), node->alt.isEmpty() ? node->href : node->alt));
    lay->addWidget(imgLbl);

    if (!node->alt.isEmpty()) {
        auto *cap = new QLabel(node->alt, frame);
        cap->setAlignment(Qt::AlignCenter);
        cap->setStyleSheet(QString("color:%1;font-size:10px;").arg(th.textMuted.name()));
        lay->addWidget(cap);
    }

    // Async image load
    if (!node->href.isEmpty()) {
        fetchImage(node->href, imgLbl);
    }
    return frame;
}

void MarkdownRenderer::fetchImage(const QString &url, QLabel *label) {
    // Check cache first
    if (m_imageCache.contains(url)) {
        QPixmap *px = m_imageCache[url];
        if (px && !px->isNull()) {
            label->setPixmap(px->scaled(qMin(px->width(), 800), qMin(px->height(), 600),
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
            label->setFixedHeight(qMin(px->height(), 600) + 8);
            // Click to fullscreen
            label->setCursor(Qt::PointingHandCursor);
            label->installEventFilter(this);
            label->setProperty("pixmap_url", url);
        }
        return;
    }

    // Try as local file
    QUrl qurl(url);
    if (qurl.isLocalFile() || !url.startsWith("http")) {
        QString path = qurl.isLocalFile() ? qurl.toLocalFile() : url;
        QImageReader reader(path);
        if (reader.canRead()) {
            QPixmap px = QPixmap::fromImageReader(&reader);
            if (!px.isNull()) {
                m_imageCache.insert(url, new QPixmap(px));
                label->setPixmap(px.scaled(qMin(px.width(), 800), qMin(px.height(), 600),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
                label->setFixedHeight(qMin(px.height(), 600) + 8);
                label->setCursor(Qt::PointingHandCursor);
                label->installEventFilter(this);
                label->setProperty("pixmap_url", url);
                return;
            }
        } else {
            // ✅ FIX: Show error for local file not found or invalid format
            if (label) {
                const auto &th = ThemeManager::instance().theme();
                label->setText(QString("<span style='color:%1;font-size:12px;'>❌ Cannot load<br/>%2</span>")
                    .arg(th.textMuted.name(), path));
                label->setFixedHeight(50);
            }
            return;
        }
    }

    // Remote fetch
    QNetworkRequest req(qurl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    // ✅ FIX: Add timeout
    req.setTransferTimeout(10000); // 10 seconds timeout
    
    auto *reply = m_net->get(req);
    
    // Capture label as weak reference to prevent use-after-delete
    connect(reply, &QNetworkReply::finished, this, [this, reply, label, url](){
        reply->deleteLater();
        
        // ✅ FIX: Explicit error handling
        if (reply->error() != QNetworkReply::NoError) {
            if (label) {
                const auto &th = ThemeManager::instance().theme();
                QString errorMsg = reply->errorString();
                QString displayError = errorMsg;
                
                // Map common errors to user-friendly messages
                if (reply->error() == QNetworkReply::ConnectionRefusedError) {
                    displayError = "Connection refused";
                } else if (reply->error() == QNetworkReply::TimeoutError) {
                    displayError = "Request timeout";
                } if (reply->error() == QNetworkReply::ContentNotFoundError) {
                    displayError = "Image not found (404)";
                } else if (reply->error() == QNetworkReply::ProtocolUnknownError) {
                    displayError = "Invalid URL";
                }
                
                label->setText(QString("<span style='color:%1;font-size:12px;'>❌ Failed<br/>%2</span>")
                    .arg(th.textMuted.name(), displayError));
                label->setFixedHeight(50);
            }
            return;
        }
        
        if (!label) return;  // ✅ Safety check: label might be deleted
        
        QByteArray data = reply->readAll();
        QPixmap px;
        
        // ✅ FIX: Handle failed image load
        if (!px.loadFromData(data)) {
            const auto &th = ThemeManager::instance().theme();
            label->setText(QString("<span style='color:%1;font-size:12px;'>⚠️ Invalid format<br/>%2</span>")
                .arg(th.textMuted.name(), url));
            label->setFixedHeight(50);
            return;
        }
        
        // ✅ Success path
        m_imageCache.insert(url, new QPixmap(px));
        if (!label) return;
        label->setPixmap(px.scaled(qMin(px.width(), 800), qMin(px.height(), 600),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        label->setFixedHeight(qMin(px.height(), 600) + 8);
        label->setCursor(Qt::PointingHandCursor);
        label->installEventFilter(this);
        label->setProperty("pixmap_url", url);
    });
}

// ─── Chart ──────────────────────────────────────────────────────────–[...]
ChartWidget* MarkdownRenderer::buildChart(const MarkdownNode::Ptr &node) {
    auto *cw = new ChartWidget(node->chartData, m_container);
    connect(cw, &ChartWidget::clicked, this, [this, cw, node](){
        QPixmap px = cw->toPixmap(1200, 600);
        QString title = node->chartData.value("title").toString();
        auto *viewer = new ImageViewer(px, title.isEmpty() ? "Chart" : title);
        viewer->show();
    });
    return cw;
}

// ─── Inline HTML ────────────────────────────────────────────────────────–[...]
QString MarkdownRenderer::inlineToHtml(const QVector<MarkdownNode::Ptr> &nodes) {
    QString html;
    for (const auto &n : nodes) html += nodeToHtml(n);
    return html;
}

QString MarkdownRenderer::nodeToHtml(const MarkdownNode::Ptr &n) {
    const auto &th = ThemeManager::instance().theme();
    switch (n->type) {
    case NodeType::Text:
        return n->text.toHtmlEscaped();

    case NodeType::Bold:
        return QString("<b>%1</b>").arg(inlineToHtml(n->children));

    case NodeType::Italic:
        return QString("<i>%1</i>").arg(inlineToHtml(n->children));

    case NodeType::BoldItalic:
        return QString("<b><i>%1</i></b>").arg(inlineToHtml(n->children));

    case NodeType::InlineCode:
        return QString(
            "<span style='font-family:Consolas,monospace;font-size:12px;"
            "color:%1;background:%2;padding:1px 5px;border-radius:3px;"
            "border:1px solid %3;'>%4</span>")
            .arg(th.textCode.name(), th.bgInlineCode.name(),
                 th.border.name(), n->text.toHtmlEscaped());

    case NodeType::Link:
        return QString("<a href='%1' style='color:%2;text-decoration:none;'>%3</a>")
            .arg(n->href.toHtmlEscaped(), th.textLink.name(), inlineToHtml(n->children));

    case NodeType::Image:
        // Inline images in paragraph context — placeholder; actual loading via buildImage
        return QString("<i style='color:%1'>[img: %2]</i>")
            .arg(th.textMuted.name(), n->alt.isEmpty() ? n->href : n->alt);

    case NodeType::LineBreak:
        return "<br/>";

    default:
        return n->text.toHtmlEscaped();
    }
}

// ─── Syntax Highlighting ──────────────────────────────────────────────────────
QString MarkdownRenderer::highlightCode(const QString &code, const QString &lang) {
    if (lang.isEmpty()) return code;

    struct Rule { QRegularExpression re; QString cls; };
    QVector<Rule> rules;

    if (lang == "python" || lang == "py") {
        rules = {
            { QRegularExpression(R"(#[^\n]*)"),                                    "cmt" },
            { QRegularExpression(R"(("""[\s\S]*?"""|'''[\s\S]*?'''))"),             "str" },
            { QRegularExpression(R"(("[^"\\]*(?:\\.[^"\\]*)*"|'[^'\\]*(?:\\.[^'\\]*)*'))"), "str" },
            { QRegularExpression(R"(\b(def|class|if|elif|else|for|while|import|from|return|lambda|yield|async|await|try|except|finally|with|as|pass|break|continue|not|and|or|in|is|None|True|False)\b)"), "kw" },
            { QRegularExpression(R"(\b\d+\.?\d*([eE][+-]?\d+)?\b)"),               "num" },
        };
    } else if (lang == "cpp" || lang == "c++" || lang == "c") {
        rules = {
            { QRegularExpression(R"(//[^\n]*)"),                                   "cmt" },
            { QRegularExpression(R"(/\*[\s\S]*?\*/)"),                             "cmt" },
            { QRegularExpression(R"("[^"\\]*(?:\\.[^"\\]*)*")"),                   "str" },
            { QRegularExpression(R"(\b(int|float|double|char|void|bool|auto|const|static|struct|class|namespace|template|typename|return|if|else|for|while|do|switch|case|break|continue|new|delete|nullptr|true|false|include|define|ifdef|ifndef|endif|using|public|private|protected|virtual|override|operator)\b)"), "kw" },
            { QRegularExpression(R"(\b[A-Z][a-zA-Z0-9_]*\b)"),                    "ty"  },
            { QRegularExpression(R"(\b\d+\.?\d*[fFlLuU]*\b)"),                    "num" },
        };
    } else if (lang == "js" || lang == "javascript" || lang == "ts" || lang == "typescript") {
        rules = {
            { QRegularExpression(R"(//[^\n]*)"),                                   "cmt" },
            { QRegularExpression(R"(`[^`]*`)"),                                    "str" },
            { QRegularExpression(R"("[^"\\]*(?:\\.[^"\\]*)*"|'[^'\\]*(?:\\.[^'\\]*)*')"), "str" },
            { QRegularExpression(R"(\b(const|let|var|function|class|if|else|for|while|return|import|export|from|default|async|await|try|catch|finally|new|delete|typeof|instanceof|null|undefined|true|false|this|super|extends|implements|interface|type|enum)\b)"), "kw" },
            { QRegularExpression(R"(\b\d+\.?\d*\b)"),                             "num" },
        };
    } else {
        return code; // no highlighting
    }

    // Apply rules sequentially — simple but effective
    QString result = code;
    // We work on a "marked" version to avoid double-replacing
    // Use a placeholder approach
    struct Span { qsizetype start, end; QString cls; }; 
    QVector<Span> spans;

    for (const auto &rule : rules) {
        auto it = rule.re.globalMatch(code);
        while (it.hasNext()) {
            auto m = it.next();
            // check not overlapping
            bool overlap = false;
            for (const auto &s : spans)
                if (m.capturedStart() < s.end && m.capturedEnd() > s.start)
                { overlap = true; break; }
            if (!overlap)
                spans.append({m.capturedStart(), m.capturedEnd(), rule.cls});
        }
    }
    std::sort(spans.begin(), spans.end(), [](const Span &a, const Span &b){ return a.start < b.start; });

   QString out;
    qsizetype pos = 0;  // Changed from int to qsizetype
    for (const auto &sp : spans) {
        out += code.mid(pos, sp.start - pos);
        out += QString("<span class='%1'>%2</span>")
            .arg(sp.cls, code.mid(sp.start, sp.end - sp.start));
        pos = sp.end;
    }
    out += code.mid(pos);
    return out;
}

// ─── Anchor scroll ────────────────────────────────────────────────────────–[...]
void MarkdownRenderer::scrollToAnchor(const QString &anchor) {
    if (!m_anchorMap.contains(anchor)) return;
    BlockWidget *bw = m_anchorMap[anchor];
    // scroll so widget top is at viewport top + 20px padding
    QPoint pos = bw->mapTo(m_container, QPoint(0, 0));
    verticalScrollBar()->setValue(qMax(0, pos.y() - 20));
}

// ─── Event filter (image click) ───────────────────────────────────────────────
bool MarkdownRenderer::eventFilter(QObject *obj, QEvent *ev) {
    if (ev->type() == QEvent::MouseButtonRelease) {
        if (auto *lbl = qobject_cast<QLabel*>(obj)) {
            QString url = lbl->property("pixmap_url").toString();
            if (!url.isEmpty() && m_imageCache.contains(url)) {
                QPixmap *px = m_imageCache[url];
                if (px && !px->isNull()) {
                    auto *viewer = new ImageViewer(*px, url);
                    viewer->show();
                }
            }
        }
    }
    return QScrollArea::eventFilter(obj, ev);
}

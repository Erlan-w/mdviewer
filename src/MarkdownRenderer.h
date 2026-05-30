#pragma once
#include "MarkdownParser.h"
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>          // ADD THIS LINE
#include <QMap>
#include <QNetworkAccessManager>
#include <QCache>

// Forward declarations
class ChartWidget;
class ImageViewer;

// ─── Block Widget base ────────────────────────────────────────────────────────
class BlockWidget : public QWidget {
    Q_OBJECT
public:
    explicit BlockWidget(QWidget *p = nullptr) : QWidget(p) {}
    QString anchor; // for headings
signals:
    void anchorVisible(const QString &anchor);
};

// ─── MarkdownRenderer ─────────────────────────────────────────────────────────
class MarkdownRenderer : public QScrollArea {
    Q_OBJECT
public:
    explicit MarkdownRenderer(QWidget *parent = nullptr);
    ~MarkdownRenderer() override;

    void render(const MarkdownNode::Ptr &doc);
    void scrollToAnchor(const QString &anchor);

signals:
    void headingVisible(const QString &anchor); // for TOC highlight

private:
    QWidget     *m_container;
    QVBoxLayout *m_layout;
    QNetworkAccessManager *m_net;

    QMap<QString, BlockWidget*> m_anchorMap;  // anchor -> widget

    // cache for remote images
    QCache<QString, QPixmap> m_imageCache;

    void clearWidgets();
    void buildBlock(const MarkdownNode::Ptr &node);
    QWidget* buildHeading(const MarkdownNode::Ptr &node);
    QWidget* buildParagraph(const MarkdownNode::Ptr &node);
    QWidget* buildCodeBlock(const MarkdownNode::Ptr &node);
    QWidget* buildBlockQuote(const MarkdownNode::Ptr &node);
    QWidget* buildList(const MarkdownNode::Ptr &node, int depth = 0);
    QWidget* buildTable(const MarkdownNode::Ptr &node);
    QWidget* buildHRule();
    QWidget* buildImage(const MarkdownNode::Ptr &node);
    ChartWidget* buildChart(const MarkdownNode::Ptr &node);

    // inline rendering into a QLabel-like rich text widget
    QString inlineToHtml(const QVector<MarkdownNode::Ptr> &nodes);
    QString nodeToHtml(const MarkdownNode::Ptr &node);

    // syntax highlighting → HTML spans
    QString highlightCode(const QString &code, const QString &lang);

    void fetchImage(const QString &url, QLabel *label);

    bool eventFilter(QObject *obj, QEvent *ev) override;
};

#include "MainWindow.h"
#include "MarkdownParser.h"
#include "MarkdownRenderer.h"
#include "TocWidget.h"
#include "ThemeManager.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QKeyEvent>
#include <QFile>
#include <QTextStream>
#include <QShortcut>
#include <QScreen>
#include <QResizeEvent>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_parser(new MarkdownParser())
    , m_watcher(new QFileSystemWatcher(this))
    , m_reloadTimer(new QTimer(this))
{
    setWindowTitle("Markdown Viewer");
    setAcceptDrops(true);

    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(400);

    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->availableGeometry());
    }
    showMaximized();

    buildUi();
    applyTheme();

    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &MainWindow::onFileChanged);
    connect(m_reloadTimer, &QTimer::timeout, this, &MainWindow::onReload);

    // Shortcuts
    auto *ctrlO = new QShortcut(QKeySequence::Open, this);
    connect(ctrlO, &QShortcut::activated, this, &MainWindow::onOpenFile);
    auto *ctrlR = new QShortcut(QKeySequence::Refresh, this);
    connect(ctrlR, &QShortcut::activated, this, &MainWindow::onReload);
    auto *f5 = new QShortcut(Qt::Key_F5, this);
    connect(f5, &QShortcut::activated, this, &MainWindow::onReload);
}

MainWindow::~MainWindow() {
    delete m_parser;
}

// ─── Build UI ─────────────────────────────────────────────────────────────────
void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *rootLay = new QVBoxLayout(central);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    // ── Header (ultra-minimalist) ──
    m_header = new QWidget(central);
    m_header->setFixedHeight(36);
    auto *hlay = new QHBoxLayout(m_header);
    hlay->setContentsMargins(6, 0, 12, 0);
    hlay->setSpacing(6);

    // Hamburger button
    m_hamburger = new QPushButton("☰", m_header);
    m_hamburger->setFixedSize(30, 28);
    m_hamburger->setToolTip("Toggle TOC (Ctrl+\\)");
    m_hamburger->setFlat(true);
    connect(m_hamburger, &QPushButton::clicked, this, [this]{ m_toc->toggle(); });

    // Open button
    m_openBtn = new QPushButton("Open", m_header);
    m_openBtn->setFixedHeight(26);
    m_openBtn->setToolTip("Open File (Ctrl+O)");
    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);

    // Reload button
    m_reloadBtn = new QPushButton("↺", m_header);
    m_reloadBtn->setFixedSize(28, 26);
    m_reloadBtn->setToolTip("Reload (F5)");
    connect(m_reloadBtn, &QPushButton::clicked, this, &MainWindow::onReload);

    m_titleLabel = new QLabel("Markdown Viewer", m_header);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel("", m_header);
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_statusLabel->setFixedWidth(120);

    hlay->addWidget(m_hamburger);
    hlay->addWidget(m_openBtn);
    hlay->addWidget(m_reloadBtn);
    hlay->addStretch();
    hlay->addWidget(m_titleLabel, 1, Qt::AlignCenter);
    hlay->addStretch();
    hlay->addWidget(m_statusLabel);
    rootLay->addWidget(m_header);

    // ── Splitter: TOC + Content ──
    m_splitter = new QSplitter(Qt::Horizontal, central);
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);

    m_toc = new TocWidget(m_splitter);
    m_renderer = new MarkdownRenderer(m_splitter);

    m_splitter->addWidget(m_toc);
    m_splitter->addWidget(m_renderer);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    rootLay->addWidget(m_splitter, 1);

    // Wire TOC → scroll
    connect(m_toc, &TocWidget::anchorClicked,
            m_renderer, &MarkdownRenderer::scrollToAnchor);

    // Wire scroll → TOC highlight
    connect(m_renderer, &MarkdownRenderer::headingVisible,
            m_toc, &TocWidget::setActiveAnchor);

    // Hamburger shortcut
    auto *tocShortcut = new QShortcut(QKeySequence("Ctrl+\\"), this);
    connect(tocShortcut, &QShortcut::activated, this, [this]{ m_toc->toggle(); });

    // Status bar
    statusBar()->hide();
}

void MainWindow::applyTheme() {
    const auto &th = ThemeManager::instance().theme();
    setStyleSheet(QString("QMainWindow,QWidget{background:%1;color:%2;}")
        .arg(th.bg.name(), th.text.name()));

    m_header->setStyleSheet(QString(
        "QWidget{background:%1;border-bottom:1px solid %2;}")
        .arg(th.headerBg.name(), th.border.name()));

    QString btnStyle = QString(
        "QPushButton{background:%1;color:%2;border:1px solid %3;"
        "border-radius:4px;padding:2px 10px;font:11px;}"
        "QPushButton:hover{background:%4;border-color:%5;}"
        "QPushButton:pressed{background:%6;}")
        .arg(th.bgAlt.name(), th.textMuted.name(), th.border.name(),
             th.bgCode.name(), th.borderAccent.name(), th.bgTable.name());
    m_openBtn->setStyleSheet(btnStyle);
    m_reloadBtn->setStyleSheet(btnStyle);

    m_hamburger->setStyleSheet(QString(
        "QPushButton{background:transparent;color:%1;border:none;font:16px;}"
        "QPushButton:hover{color:white;}")
        .arg(th.textMuted.name()));

    m_titleLabel->setStyleSheet(QString(
        "color:%1;font:12px;font-weight:500;")
        .arg(th.textMuted.name()));

    m_statusLabel->setStyleSheet(QString(
        "color:%1;font:10px;")
        .arg(th.textMuted.name()));

    m_splitter->setStyleSheet(QString(
        "QSplitter::handle{background:%1;}")
        .arg(th.border.name()));
}

// ─── File Operations ──────────────────────────────────────────────────────────
void MainWindow::onOpenFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Markdown File", {},
        "Markdown Files (*.md *.markdown *.txt);;All Files (*)");
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::openFile(const QString &path) {
    m_currentPath = path;
    loadMarkdown(path);

    // Watch for changes
    if (!m_watcher->files().isEmpty())
        m_watcher->removePaths(m_watcher->files());
    m_watcher->addPath(path);
}

void MainWindow::onReload() {
    if (!m_currentPath.isEmpty()) loadMarkdown(m_currentPath);
}

void MainWindow::onFileChanged(const QString &) {
    m_reloadTimer->start(); // debounce
}

void MainWindow::loadMarkdown(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_statusLabel->setText("Error opening file");
        return;
    }
    QTextStream in(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    QString content = in.readAll();
    f.close();

    auto doc = m_parser->parse(content);
    auto toc = m_parser->extractToc();

    m_renderer->render(doc);
    m_toc->setEntries(toc);

    updateTitle(QFileInfo(path).fileName());
    m_statusLabel->setText(QString("%1 headings").arg(toc.size()));
}

void MainWindow::updateTitle(const QString &filename) {
    m_titleLabel->setText(filename);
    setWindowTitle(filename + " — Markdown Viewer");
}

// ─── Drag & Drop ──────────────────────────────────────────────────────────────
void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e) {
    const auto urls = e->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString path = urls.first().toLocalFile();
        if (path.endsWith(".md", Qt::CaseInsensitive) ||
            path.endsWith(".markdown", Qt::CaseInsensitive) ||
            path.endsWith(".txt", Qt::CaseInsensitive))
            openFile(path);
    }
}

void MainWindow::resizeEvent(QResizeEvent *e) {
    QMainWindow::resizeEvent(e);
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape && isFullScreen()) showMaximized();
    QMainWindow::keyPressEvent(e);
}

#include "ImageViewer.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QScrollBar>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QApplication>
#include <QScreen>

// ─── ZoomableLabel ────────────────────────────────────────────────────────────
ZoomableLabel::ZoomableLabel(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
    setStyleSheet("background: transparent;");
}

void ZoomableLabel::setPixmap(const QPixmap &px) {
    m_original = px;
    m_zoom = 1.0;
    applyZoom();
}

void ZoomableLabel::resetZoom() {
    m_zoom = 1.0;
    applyZoom();
}

void ZoomableLabel::applyZoom() {
    if (m_original.isNull()) return;
    QSize sz(int(m_original.width() * m_zoom), int(m_original.height() * m_zoom));
    QLabel::setPixmap(m_original.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    resize(sz);
    emit zoomChanged(m_zoom);
}

void ZoomableLabel::wheelEvent(QWheelEvent *e) {
    double delta = e->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
    m_zoom = qBound(0.1, m_zoom * delta, 10.0);
    applyZoom();
    e->accept();
}

void ZoomableLabel::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPos = e->globalPosition().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ZoomableLabel::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging) return;
    QPoint delta = e->globalPosition().toPoint() - m_lastPos;
    m_lastPos = e->globalPosition().toPoint();
    if (auto sa = qobject_cast<QScrollArea*>(parent() ? parent()->parent() : nullptr)) {
        sa->horizontalScrollBar()->setValue(sa->horizontalScrollBar()->value() - delta.x());
        sa->verticalScrollBar()->setValue(sa->verticalScrollBar()->value() - delta.y());
    }
}

void ZoomableLabel::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }
}

void ZoomableLabel::paintEvent(QPaintEvent *e) {
    QLabel::paintEvent(e);
}

// ─── ImageViewer ─────────────────────────────────────────────────────────────
ImageViewer::ImageViewer(const QPixmap &px, const QString &title, QWidget *parent)
    : QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_DeleteOnClose);
    const auto &th = ThemeManager::instance().theme();
    setStyleSheet(QString("background:%1;").arg(th.bg.name()));

    // Screen size
    QScreen *screen = QApplication::primaryScreen();
    QRect geo = screen ? screen->availableGeometry() : QRect(0,0,1280,800);
    setGeometry(geo);
    showFullScreen();

    // Layout
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header bar ──
    auto *bar = new QWidget(this);
    bar->setFixedHeight(40);
    bar->setStyleSheet(QString("background:%1;").arg(th.headerBg.name()));
    auto *hlay = new QHBoxLayout(bar);
    hlay->setContentsMargins(12, 0, 12, 0);

    auto *titleLbl = new QLabel(title.isEmpty() ? "Image Viewer" : title, bar);
    titleLbl->setStyleSheet(QString("color:%1;font:12px;").arg(th.textMuted.name()));

    m_zoomLabel = new QLabel("100%", bar);
    m_zoomLabel->setStyleSheet(QString("color:%1;font:11px;min-width:50px;").arg(th.textMuted.name()));

    auto *resetBtn = new QPushButton("1:1", bar);
    resetBtn->setFixedSize(32, 24);
    resetBtn->setStyleSheet(QString(
        "QPushButton{background:%1;color:%2;border:1px solid %3;border-radius:3px;font:10px;}"
        "QPushButton:hover{background:%4;}")
        .arg(th.bgAlt.name(), th.textMuted.name(), th.border.name(), th.bgCode.name()));
    connect(resetBtn, &QPushButton::clicked, this, [this]{ m_label->resetZoom(); });

    auto *closeBtn = new QPushButton("✕", bar);
    closeBtn->setFixedSize(28, 24);
    closeBtn->setStyleSheet(QString(
        "QPushButton{background:transparent;color:%1;border:none;font:14px;}"
        "QPushButton:hover{color:white;}")
        .arg(th.textMuted.name()));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    hlay->addWidget(titleLbl);
    hlay->addStretch();
    hlay->addWidget(m_zoomLabel);
    hlay->addWidget(resetBtn);
    hlay->addSpacing(8);
    hlay->addWidget(closeBtn);
    root->addWidget(bar);

    // ── Scroll area ──
    m_scroll = new QScrollArea(this);
    m_scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}"
                            "QScrollBar:vertical,QScrollBar:horizontal{width:6px;height:6px;"
                            "background:transparent;}"
                            "QScrollBar::handle{background:#333;border-radius:3px;}");
    m_scroll->setAlignment(Qt::AlignCenter);
    m_scroll->setWidgetResizable(false);

    m_label = new ZoomableLabel(m_scroll);
    m_label->setPixmap(px);
    m_scroll->setWidget(m_label);
    root->addWidget(m_scroll, 1);

    connect(m_label, &ZoomableLabel::zoomChanged, this, &ImageViewer::updateZoomLabel);

    // Shortcuts
    auto *esc = new QShortcut(Qt::Key_Escape, this);
    connect(esc, &QShortcut::activated, this, &QDialog::close);
    auto *plus = new QShortcut(Qt::Key_Plus, this);
    connect(plus, &QShortcut::activated, this, [this]{ QWheelEvent e({},{},QPoint(0,120),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false); QApplication::sendEvent(m_label,&e); });
    auto *minus = new QShortcut(Qt::Key_Minus, this);
    connect(minus, &QShortcut::activated, this, [this]{ QWheelEvent e({},{},QPoint(0,-120),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false); QApplication::sendEvent(m_label,&e); });
}

void ImageViewer::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) close();
    else QDialog::keyPressEvent(e);
}

void ImageViewer::updateZoomLabel(double factor) {
    m_zoomLabel->setText(QString("%1%").arg(int(factor * 100)));
}

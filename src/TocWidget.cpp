#include "TocWidget.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QEasingCurve>

TocWidget::TocWidget(QWidget *parent) : QWidget(parent) {
    setFixedWidth(240);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // header
    auto *hdr = new QLabel("  Contents", this);
    hdr->setFixedHeight(36);
    const auto &th = ThemeManager::instance().theme();
    hdr->setStyleSheet(QString(
        "background:%1;color:%2;font:11px;font-weight:600;"
        "border-bottom:1px solid %3;padding-left:4px;")
        .arg(th.tocBg.name(), th.textMuted.name(), th.border.name()));
    lay->addWidget(hdr);

    m_list = new QListWidget(this);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFrameShape(QFrame::NoFrame);
    lay->addWidget(m_list, 1);

    m_anim = new QPropertyAnimation(this, "panelWidth", this);
    m_anim->setDuration(180);
    m_anim->setEasingCurve(QEasingCurve::InOutQuart);

    applyStyle();

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_entries.size())
            emit anchorClicked(m_entries[idx].anchor);
    });
}

void TocWidget::setEntries(const QVector<TocEntry> &entries) {
    m_entries = entries;
    m_list->clear();
    for (int i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];
        QString indent(qMax(0, (e.level - 1) * 2), ' ');
        auto *item = new QListWidgetItem(indent + e.title, m_list);
        item->setData(Qt::UserRole, i);
        item->setToolTip(e.title);

        // font by level
        QFont f = ThemeManager::instance().fontBody(e.level <= 2 ? 11 : 10);
        if (e.level == 1) f.setWeight(QFont::DemiBold);
        item->setFont(f);

        const auto &th = ThemeManager::instance().theme();
        QColor c = (e.level == 1) ? th.text
                 : (e.level == 2) ? th.tocText
                 :                  th.textMuted;
        item->setForeground(c);
        m_list->addItem(item);
    }
}

void TocWidget::setActiveAnchor(const QString &anchor) {
    for (int i = 0; i < m_list->count(); ++i) {
        auto *item = m_list->item(i);
        int idx = item->data(Qt::UserRole).toInt();
        bool active = (idx >= 0 && idx < m_entries.size() &&
                       m_entries[idx].anchor == anchor);
        const auto &th = ThemeManager::instance().theme();
        item->setBackground(active ? th.tocActive : Qt::transparent);
        if (active) m_list->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
}

void TocWidget::applyStyle() {
    const auto &th = ThemeManager::instance().theme();
    setStyleSheet(QString("background:%1;").arg(th.tocBg.name()));
    m_list->setStyleSheet(QString(R"(
        QListWidget {
            background: %1;
            border: none;
            outline: none;
        }
        QListWidget::item {
            padding: 5px 8px 5px 12px;
            border-left: 2px solid transparent;
            color: %2;
        }
        QListWidget::item:hover {
            background: %3;
            border-left: 2px solid %4;
        }
        QListWidget::item:selected {
            background: %5;
            border-left: 2px solid %4;
            color: white;
        }
        QScrollBar:vertical {
            width: 4px; background: transparent;
        }
        QScrollBar::handle:vertical {
            background: %6; border-radius: 2px;
        }
    )").arg(th.tocBg.name(), th.tocText.name(), th.tocHover.name(),
            th.tocAccent.name(), th.tocActive.name(), th.border.name()));
}

void TocWidget::animateOpen() {
    m_open = true;
    m_anim->stop();
    m_anim->setStartValue(width());
    m_anim->setEndValue(240);
    m_anim->start();
}

void TocWidget::animateClose() {
    m_open = false;
    m_anim->stop();
    m_anim->setStartValue(width());
    m_anim->setEndValue(0);
    m_anim->start();
}

void TocWidget::toggle() {
    if (m_open) animateClose(); else animateOpen();
}

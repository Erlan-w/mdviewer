#pragma once
#include "MarkdownParser.h"
#include <QWidget>
#include <QListWidget>
#include <QPropertyAnimation>

class TocWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int panelWidth READ panelWidth WRITE setPanelWidth)
public:
    explicit TocWidget(QWidget *parent = nullptr);

    void setEntries(const QVector<TocEntry> &entries);
    void setActiveAnchor(const QString &anchor);

    int  panelWidth() const  { return width(); }
    void setPanelWidth(int w){ setFixedWidth(w); }

    void animateOpen();
    void animateClose();
    bool isOpen() const { return m_open; }
    void toggle();

signals:
    void anchorClicked(const QString &anchor);

private:
    QListWidget          *m_list;
    QPropertyAnimation   *m_anim;
    bool                  m_open = true;
    QVector<TocEntry>     m_entries;

    void applyStyle();
};

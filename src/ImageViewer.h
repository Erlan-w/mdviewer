#pragma once
#include <QDialog>
#include <QPixmap>
#include <QLabel>
#include <QScrollArea>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>

// ─── ZoomableLabel ────────────────────────────────────────────────────────────
class ZoomableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ZoomableLabel(QWidget *parent = nullptr);
    void setPixmap(const QPixmap &px);
    void resetZoom();
    double zoomFactor() const { return m_zoom; }

protected:
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

signals:
    void zoomChanged(double factor);

private:
    void applyZoom();

    QPixmap m_original;
    double  m_zoom = 1.0;
    bool    m_dragging = false;
    QPoint  m_lastPos;
};

// ─── ImageViewer Dialog ───────────────────────────────────────────────────────
class ImageViewer : public QDialog {
    Q_OBJECT
public:
    explicit ImageViewer(const QPixmap &px, const QString &title = {}, QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    ZoomableLabel *m_label;
    QScrollArea   *m_scroll;
    QLabel        *m_zoomLabel;

    void updateZoomLabel(double factor);
};

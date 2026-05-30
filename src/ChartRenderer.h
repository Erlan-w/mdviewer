#pragma once
#include <QWidget>
#include <QVariantMap>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using namespace Qt;
#else
QT_CHARTS_USE_NAMESPACE
#endif

// Inline chart widget embedded in the document
class ChartWidget : public QChartView {
    Q_OBJECT
public:
    explicit ChartWidget(const QVariantMap &data, QWidget *parent = nullptr);

    QPixmap toPixmap(int w = 800, int h = 400) const;

signals:
    void clicked();          // emitted when user clicks → open fullscreen

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void buildChart(const QVariantMap &data);
    void buildBar(QChart *c, const QVariantMap &d);
    void buildLine(QChart *c, const QVariantMap &d);
    void buildPie(QChart *c, const QVariantMap &d);
    void applyDarkTheme(QChart *c);

    bool m_pressed = false;
};

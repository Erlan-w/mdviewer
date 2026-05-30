#include "ChartRenderer.h"
#include "ThemeManager.h"
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QMouseEvent>
#include <QStringList>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using namespace Qt;
#else
QT_CHARTS_USE_NAMESPACE
#endif

ChartWidget::ChartWidget(const QVariantMap &data, QWidget *parent)
    : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(260);
    setMaximumHeight(320);
    setCursor(Qt::PointingHandCursor);

    auto *c = new QChart();
    buildChart(data);
    applyDarkTheme(chart());

    const auto &th = ThemeManager::instance().theme();
    setStyleSheet(QString("background:%1;border:1px solid %2;border-radius:6px;")
        .arg(th.bgCode.name(), th.border.name()));
}

void ChartWidget::buildChart(const QVariantMap &d) {
    QString type = d.value("type", "bar").toString().toLower();
    auto *c = new QChart();
    c->setAnimationOptions(QChart::SeriesAnimations);
    c->setTitle(d.value("title").toString());

    if      (type == "line") buildLine(c, d);
    else if (type == "pie")  buildPie(c, d);
    else                     buildBar(c, d);

    setChart(c);
    applyDarkTheme(c);
}

void ChartWidget::buildBar(QChart *c, const QVariantMap &d) {
    auto *series = new QBarSeries();
    auto *set    = new QBarSet(d.value("legend", "Data").toString());

    QStringList vals  = d.value("data").toString().split(',', Qt::SkipEmptyParts);
    QStringList labs  = d.value("labels").toString().split(',', Qt::SkipEmptyParts);

    QList<qreal> nums;
    for (auto &v : vals) { bool ok; double x = v.trimmed().toDouble(&ok); if(ok) nums << x; }
    *set << nums;
    series->append(set);
    c->addSeries(series);

    auto *axisX = new QBarCategoryAxis();
    QStringList catLabels;
    for (int i = 0; i < nums.size(); ++i)
        catLabels << (i < labs.size() ? labs[i].trimmed() : QString::number(i+1));
    axisX->append(catLabels);
    c->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    c->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}

void ChartWidget::buildLine(QChart *c, const QVariantMap &d) {
    auto *series = new QLineSeries();
    series->setName(d.value("legend", "Data").toString());

    QStringList vals = d.value("data").toString().split(',', Qt::SkipEmptyParts);
    for (int i = 0; i < vals.size(); ++i) {
        bool ok; double y = vals[i].trimmed().toDouble(&ok);
        if (ok) series->append(i, y);
    }
    c->addSeries(series);
    c->createDefaultAxes();
}

void ChartWidget::buildPie(QChart *c, const QVariantMap &d) {
    auto *series = new QPieSeries();
    QStringList vals = d.value("data").toString().split(',', Qt::SkipEmptyParts);
    QStringList labs = d.value("labels").toString().split(',', Qt::SkipEmptyParts);
    for (int i = 0; i < vals.size(); ++i) {
        bool ok; double v = vals[i].trimmed().toDouble(&ok);
        if (!ok) continue;
        QString lbl = (i < labs.size()) ? labs[i].trimmed() : QString::number(i+1);
        series->append(lbl, v);
    }
    c->addSeries(series);
    c->legend()->setVisible(true);
}

void ChartWidget::applyDarkTheme(QChart *c) {
    const auto &th = ThemeManager::instance().theme();
    c->setBackgroundBrush(QBrush(th.bgCode));
    c->setPlotAreaBackgroundBrush(QBrush(th.bgAlt));
    c->setPlotAreaBackgroundVisible(true);
    c->setTitleBrush(QBrush(th.text));
    QFont tf = ThemeManager::instance().fontBold(11);
    c->setTitleFont(tf);

    if (c->legend()) {
        c->legend()->setLabelColor(th.textMuted);
        c->legend()->setBackgroundVisible(false);
    }

    for (auto *axis : c->axes()) {
        axis->setLabelsBrush(QBrush(th.textMuted));
        axis->setLinePenColor(th.border);
        axis->setGridLinePen(QPen(th.border, 1));
        QFont af = ThemeManager::instance().fontBody(9);
        axis->setLabelsFont(af);
        if (auto *la = qobject_cast<QBarCategoryAxis*>(axis))
            la->setLinePenColor(th.border);
    }
    setBackgroundBrush(QBrush(th.bgCode));
}

QPixmap ChartWidget::toPixmap(int w, int h) const {
    QPixmap pm(w, h);
    pm.fill(ThemeManager::instance().theme().bgCode);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    chart()->scene()->render(&p, QRectF(0,0,w,h), chart()->scene()->sceneRect());
    return pm;
}

void ChartWidget::mousePressEvent(QMouseEvent *e) {
    m_pressed = true;
    QChartView::mousePressEvent(e);
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (m_pressed) { m_pressed = false; emit clicked(); }
    QChartView::mouseReleaseEvent(e);
}

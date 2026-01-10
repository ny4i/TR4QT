#include "StatisticsWindow.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QDateTime>
#include <QRandomGenerator>

namespace TR4QT {

StatisticsWindow::StatisticsWindow(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    updatePlots();
}

void StatisticsWindow::setupUI() {
    setWindowTitle("Statistics");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Create tab widget for different plot types
    QTabWidget* tabWidget = new QTabWidget(this);

    // QSOs over time plot
    m_qsosOverTimePlot = new QCustomPlot();
    tabWidget->addTab(m_qsosOverTimePlot, "QSOs Over Time");

    // QSOs by band plot
    m_qsosByBandPlot = new QCustomPlot();
    tabWidget->addTab(m_qsosByBandPlot, "QSOs by Band");

    mainLayout->addWidget(tabWidget);

    // Set window size
    resize(UIDefaults::STATISTICS_WIDTH, UIDefaults::STATISTICS_HEIGHT);
    setMinimumSize(UIDefaults::STATISTICS_MIN_WIDTH, UIDefaults::STATISTICS_MIN_HEIGHT);
}

void StatisticsWindow::createQSOsOverTimePlot() {
    m_qsosOverTimePlot->clearGraphs();

    // Add a graph
    m_qsosOverTimePlot->addGraph();
    m_qsosOverTimePlot->graph(0)->setPen(QPen(Qt::blue, 2));
    m_qsosOverTimePlot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));

    // Generate example data - QSOs per hour over a contest
    QVector<double> timeKeys, qsoValues;
    QDateTime contestStart = QDateTime::currentDateTime().addSecs(-24 * 3600); // 24 hours ago

    for (int hour = 0; hour < 24; ++hour) {
        timeKeys.append(contestStart.addSecs(hour * 3600).toSecsSinceEpoch());
        // Simulate varying QSO rate (higher during peak hours)
        double qsoCount = 30 + 20 * qSin(hour * 0.3) + QRandomGenerator::global()->bounded(15);
        qsoValues.append(qsoCount);
    }

    m_qsosOverTimePlot->graph(0)->setData(timeKeys, qsoValues);

    // Configure time axis
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("hh:mm");
    m_qsosOverTimePlot->xAxis->setTicker(dateTicker);
    m_qsosOverTimePlot->xAxis->setLabel("Time");

    // Configure value axis
    m_qsosOverTimePlot->yAxis->setLabel("QSOs per Hour");
    m_qsosOverTimePlot->yAxis->setRange(0, 80);

    // Enable interactions
    m_qsosOverTimePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    // Rescale to fit data
    m_qsosOverTimePlot->rescaleAxes();
    m_qsosOverTimePlot->replot();
}

void StatisticsWindow::createQSOsByBandPlot() {
    m_qsosByBandPlot->clearPlottables();

    // Create bar chart (constructor automatically adds it to the plot)
    QCPBars* bars = new QCPBars(m_qsosByBandPlot->xAxis, m_qsosByBandPlot->yAxis);

    // Set bar appearance
    bars->setPen(QPen(QColor(0, 100, 200)));
    bars->setBrush(QColor(0, 150, 255, 180));

    // Example data - QSOs by band
    QVector<double> bandKeys = {1, 2, 3, 4, 5, 6, 7};
    QVector<double> qsoCounts = {45, 78, 123, 89, 156, 67, 34};
    QVector<QString> bandLabels = {"160m", "80m", "40m", "20m", "15m", "10m", "6m"};

    bars->setData(bandKeys, qsoCounts);

    // Configure x-axis with band labels
    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    for (int i = 0; i < bandLabels.size(); ++i) {
        textTicker->addTick(bandKeys[i], bandLabels[i]);
    }
    m_qsosByBandPlot->xAxis->setTicker(textTicker);
    m_qsosByBandPlot->xAxis->setLabel("Band");
    m_qsosByBandPlot->xAxis->setRange(0, 8);

    // Configure y-axis
    m_qsosByBandPlot->yAxis->setLabel("Number of QSOs");
    m_qsosByBandPlot->yAxis->setRange(0, 180);

    // Enable interactions
    m_qsosByBandPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    m_qsosByBandPlot->rescaleAxes();
    m_qsosByBandPlot->replot();
}

void StatisticsWindow::updatePlots() {
    // Create example plots
    createQSOsOverTimePlot();
    createQSOsByBandPlot();
}

} // namespace TR4QT

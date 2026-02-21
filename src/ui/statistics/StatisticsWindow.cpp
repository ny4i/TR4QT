/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "StatisticsWindow.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QCloseEvent>

namespace TR4QT {

StatisticsWindow::StatisticsWindow(QWidget* parent)
    : PersistentWindow<QWidget>("Windows/Statistics", parent, "StatisticsWindow")
    , m_rateCalculator(new RateCalculator(this))
    , m_historyUpdateTimer(new QTimer(this))
{
    setWindowTitle("Statistics");
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, false);

    setupUI();

    // Connect rate calculator signals
    connect(m_rateCalculator, &RateCalculator::ratesUpdated,
            this, &StatisticsWindow::onRatesUpdated);
    connect(m_rateCalculator, &RateCalculator::bandModeStatsUpdated,
            this, &StatisticsWindow::onBandModeStatsUpdated);

    // Start auto-update for rates (every 5 seconds)
    m_rateCalculator->startAutoUpdate(5000);

    // History update timer (less frequent)
    connect(m_historyUpdateTimer, &QTimer::timeout, this, &StatisticsWindow::onUpdateTimer);
    m_historyUpdateTimer->start(HISTORY_UPDATE_INTERVAL_MS);

    LOG_DEBUG("StatisticsWindow", "Statistics window created");
}

StatisticsWindow::~StatisticsWindow() {
    // Stop timers before destruction to prevent callbacks during shutdown
    if (m_historyUpdateTimer) {
        m_historyUpdateTimer->stop();
    }
    if (m_rateCalculator) {
        m_rateCalculator->stopAutoUpdate();
    }
}

void StatisticsWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // Top toolbar: View mode selector and rate options
    QHBoxLayout* toolbarLayout = new QHBoxLayout();

    // View mode selector
    QLabel* viewLabel = new QLabel("View:", this);
    toolbarLayout->addWidget(viewLabel);

    m_viewModeCombo = new QComboBox(this);
    m_viewModeCombo->addItem("Solo", ViewSolo);
    m_viewModeCombo->addItem("Multi-Op", ViewMultiOp);
    m_viewModeCombo->addItem("SO2R", ViewSO2R);
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatisticsWindow::onViewModeChanged);
    toolbarLayout->addWidget(m_viewModeCombo);

    toolbarLayout->addSpacing(20);

    // Time bucket selector
    QLabel* bucketLabel = new QLabel("Bucket:", this);
    toolbarLayout->addWidget(bucketLabel);

    m_timeBucketCombo = new QComboBox(this);
    m_timeBucketCombo->addItem("20 min", 20);
    m_timeBucketCombo->addItem("30 min", 30);
    m_timeBucketCombo->addItem("60 min", 60);
    m_timeBucketCombo->setCurrentIndex(2);  // Default to 60 min
    connect(m_timeBucketCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatisticsWindow::onTimeBucketChanged);
    toolbarLayout->addWidget(m_timeBucketCombo);

    toolbarLayout->addSpacing(20);

    // Include dupes checkbox
    m_includeDupesCheck = new QCheckBox("Include Dupes", this);
    m_includeDupesCheck->setChecked(false);
    connect(m_includeDupesCheck, &QCheckBox::stateChanged,
            this, &StatisticsWindow::onIncludeDupesChanged);
    toolbarLayout->addWidget(m_includeDupesCheck);

    toolbarLayout->addSpacing(20);

    // Goal rate spinner
    QLabel* goalLabel = new QLabel("Goal:", this);
    toolbarLayout->addWidget(goalLabel);

    m_goalRateSpin = new QSpinBox(this);
    m_goalRateSpin->setRange(0, 500);
    m_goalRateSpin->setValue(0);
    m_goalRateSpin->setSuffix(" Q/hr");
    m_goalRateSpin->setSpecialValueText("Off");
    m_goalRateSpin->setToolTip("Set goal rate line on graph (0 = off)");
    connect(m_goalRateSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &StatisticsWindow::onGoalRateChanged);
    toolbarLayout->addWidget(m_goalRateSpin);

    toolbarLayout->addStretch();

    mainLayout->addLayout(toolbarLayout);

    // Rate display widget (top section)
    m_rateDisplay = new RateDisplayWidget(this);
    mainLayout->addWidget(m_rateDisplay);

    // Separator
    QFrame* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(sep1);

    // Band summary table (middle section)
    m_bandModel = new BandSummaryModel(this);
    m_bandTable = new QTableView(this);
    m_bandTable->setModel(m_bandModel);
    m_bandTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bandTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_bandTable->setAlternatingRowColors(true);
    m_bandTable->verticalHeader()->setVisible(false);
    m_bandTable->horizontalHeader()->setStretchLastSection(true);
    m_bandTable->setMinimumHeight(150);

    // Set reasonable column widths
    m_bandTable->setColumnWidth(BandSummaryModel::ColBand, 50);
    m_bandTable->setColumnWidth(BandSummaryModel::ColMode, 50);
    m_bandTable->setColumnWidth(BandSummaryModel::ColQsos, 60);
    m_bandTable->setColumnWidth(BandSummaryModel::ColMults, 60);
    m_bandTable->setColumnWidth(BandSummaryModel::ColPoints, 70);
    m_bandTable->setColumnWidth(BandSummaryModel::ColScore, 80);
    m_bandTable->setColumnWidth(BandSummaryModel::ColBestHr, 60);

    mainLayout->addWidget(m_bandTable, 1);

    // Totals row
    m_totalsLabel = new QLabel("Totals: 0 QSOs, 0 Mults, 0 Points, Score: 0", this);
    m_totalsLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    mainLayout->addWidget(m_totalsLabel);

    // Bottom section: Operator and Station tables
    m_bottomSplitter = new QSplitter(Qt::Horizontal, this);

    // Operator section (multi-op)
    m_operatorSection = new QWidget(this);
    QVBoxLayout* opLayout = new QVBoxLayout(m_operatorSection);
    opLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* opLabel = new QLabel("Operators", this);
    opLabel->setStyleSheet("font-weight: bold;");
    opLayout->addWidget(opLabel);

    m_operatorModel = new OperatorStatsModel(this);
    m_operatorTable = new QTableView(this);
    m_operatorTable->setModel(m_operatorModel);
    m_operatorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_operatorTable->setAlternatingRowColors(true);
    m_operatorTable->verticalHeader()->setVisible(false);
    m_operatorTable->horizontalHeader()->setStretchLastSection(true);
    opLayout->addWidget(m_operatorTable);

    m_bottomSplitter->addWidget(m_operatorSection);

    // Station section (SO2R)
    m_stationSection = new QWidget(this);
    QVBoxLayout* stLayout = new QVBoxLayout(m_stationSection);
    stLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* stLabel = new QLabel("Stations", this);
    stLabel->setStyleSheet("font-weight: bold;");
    stLayout->addWidget(stLabel);

    m_stationModel = new StationStatsModel(this);
    m_stationTable = new QTableView(this);
    m_stationTable->setModel(m_stationModel);
    m_stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stationTable->setAlternatingRowColors(true);
    m_stationTable->verticalHeader()->setVisible(false);
    m_stationTable->horizontalHeader()->setStretchLastSection(true);
    stLayout->addWidget(m_stationTable);

    m_bottomSplitter->addWidget(m_stationSection);

    mainLayout->addWidget(m_bottomSplitter);

    // Initially hide operator/station sections (Solo mode)
    m_operatorSection->hide();
    m_stationSection->hide();

    // Set minimum size
    setMinimumSize(400, 500);
    resize(500, 600);
}

void StatisticsWindow::setContestStartTime(const QDateTime& startTime) {
    m_rateCalculator->setContestStartTime(startTime);
}

void StatisticsWindow::addQso(const QDateTime& timestamp,
                              BandType band,
                              ModeType mode,
                              const QString& operatorCall,
                              const QString& stationId) {
    m_rateCalculator->addQso(timestamp, band, mode, operatorCall, stationId);
}

void StatisticsWindow::loadHistory(const QVector<RateCalculator::QsoRecord>& records) {
    m_rateCalculator->loadDetailedHistory(records);

    // Update history graph
    auto history = m_rateCalculator->getHourlyRateHistory(HISTORY_HOURS);
    m_rateDisplay->updateHistory(history);
}

void StatisticsWindow::loadHistoryFromQSOs(const QList<QSO>& qsos) {
    QVector<RateCalculator::QsoRecord> records;
    records.reserve(qsos.size());
    for (const QSO& qso : qsos) {
        RateCalculator::QsoRecord record;
        record.timestamp = qso.timestamp;
        record.band = qso.band;
        record.mode = qso.mode;
        record.operatorCall = qso.operatorCall;
        record.stationId = RateCalculator::QsoRecord::DEFAULT_STATION_ID;
        record.qsoPoints = qso.qsoPoints;
        record.isDupe = qso.isDupe;
        records.append(record);
    }
    loadHistory(records);
}

void StatisticsWindow::clearStats() {
    m_rateCalculator->clear();
    m_bandModel->clear();
    m_operatorModel->clear();
    m_stationModel->clear();

    // Clear scoring totals
    m_totalMults = 0;
    m_totalPoints = 0;
    m_totalScore = 0;

    updateTotalsRow();
}

void StatisticsWindow::updateMultipliers(BandType band, ModeType mode, int multipliers) {
    m_bandModel->updateMultipliers(band, mode, multipliers);
    updateTotalsRow();
}

void StatisticsWindow::updatePoints(BandType band, ModeType mode, int points, int score) {
    m_bandModel->updatePoints(band, mode, points, score);
    updateTotalsRow();
}

void StatisticsWindow::updateScoringData(int totalMults, int totalPoints, int totalScore) {
    // Store totals for display
    // Per-band-mode points come from QSO data via RateCalculator's bandModeStatsUpdated signal
    m_totalMults = totalMults;
    m_totalPoints = totalPoints;
    m_totalScore = totalScore;

    updateTotalsRow();
}

void StatisticsWindow::setGoalRate(int rate) {
    m_rateDisplay->setGoalRate(rate);
    if (m_goalRateSpin && m_goalRateSpin->value() != rate) {
        m_goalRateSpin->blockSignals(true);
        m_goalRateSpin->setValue(rate);
        m_goalRateSpin->blockSignals(false);
    }
}

void StatisticsWindow::setViewMode(ViewMode mode) {
    m_viewMode = mode;
    m_viewModeCombo->setCurrentIndex(static_cast<int>(mode));

    updateOperatorVisibility();
    updateStationVisibility();
}

void StatisticsWindow::onViewModeChanged(int index) {
    m_viewMode = static_cast<ViewMode>(index);
    updateOperatorVisibility();
    updateStationVisibility();
}

void StatisticsWindow::onTimeBucketChanged(int index) {
    int minutes = m_timeBucketCombo->itemData(index).toInt();
    m_rateCalculator->setTimeBucket(minutes);
    // Trigger recalculation
    emit m_rateCalculator->ratesUpdated(m_rateCalculator->getCurrentSnapshot());
}

void StatisticsWindow::onIncludeDupesChanged(int state) {
    // setIncludeDupes triggers full recalculation and emits signals
    m_rateCalculator->setIncludeDupes(state == Qt::Checked);
}

void StatisticsWindow::onGoalRateChanged(int value) {
    m_rateDisplay->setGoalRate(value);
}

void StatisticsWindow::onRatesUpdated(const RateSnapshot& snapshot) {
    m_rateDisplay->updateRates(snapshot);
}

void StatisticsWindow::onBandModeStatsUpdated(const QVector<BandModeStats>& stats) {
    m_bandModel->updateStats(stats);
    updateTotalsRow();

    // Also update operator and station models
    m_operatorModel->updateStats(m_rateCalculator->getOperatorStats());
    m_stationModel->updateStats(m_rateCalculator->getStationStats());

    // Show/hide based on actual data
    updateOperatorVisibility();
    updateStationVisibility();
}

void StatisticsWindow::onUpdateTimer() {
    // Update history graph periodically
    auto history = m_rateCalculator->getHourlyRateHistory(HISTORY_HOURS);
    m_rateDisplay->updateHistory(history);
}

void StatisticsWindow::updateTotalsRow() {
    int qsos = m_bandModel->getTotalQsos();

    // Use stored scoring totals from contest scorer (more accurate than model aggregation)
    // Fall back to model totals if scoring data hasn't been provided
    int mults = (m_totalMults > 0) ? m_totalMults : m_bandModel->getTotalMults();
    int points = (m_totalPoints > 0) ? m_totalPoints : m_bandModel->getTotalPoints();
    int score = (m_totalScore > 0) ? m_totalScore : m_bandModel->getTotalScore();

    m_totalsLabel->setText(QString("Totals: %1 QSOs, %2 Mults, %3 Points, Score: %4")
                           .arg(qsos).arg(mults).arg(points).arg(score));
}

void StatisticsWindow::updateOperatorVisibility() {
    bool showOperators = (m_viewMode == ViewMultiOp) ||
                         (m_operatorModel->getOperatorCount() > 1);
    m_operatorSection->setVisible(showOperators);
}

void StatisticsWindow::updateStationVisibility() {
    bool showStations = (m_viewMode == ViewSO2R) ||
                        (m_stationModel->getStationCount() > 1);
    m_stationSection->setVisible(showStations);
}

void StatisticsWindow::closeEvent(QCloseEvent* event) {
    // Write Visible=false and save geometry via base class
    PersistentWindow<QWidget>::closeEvent(event);
    // Don't destroy, just hide — ignore the close event
    hide();
    event->ignore();
}

} // namespace TR4QT

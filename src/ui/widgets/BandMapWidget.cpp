#include "BandMapWidget.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include "../../data/LOTWUserRepository.h"
#include "../../data/SpotRepository.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QToolTip>
#include <QHelpEvent>
#include <QTimer>
#include <algorithm>

namespace TR4QT {

BandMapWidget::BandMapWidget(QWidget* parent)
    : QAbstractScrollArea(parent)
    , m_currentFrequency(0)
    , m_selectedIndex(-1)
    , m_columnCount(1)
    , m_columnWidth(200)
    , m_sortMode(BandMapSortMode::Frequency)  // Default: sort by frequency
    , m_showOnlyLotwUsers(false)
    , m_showAllBands(false)  // Default: show current band only
    , m_refreshTimer(new QTimer(this))
{
    // Load settings
    AppSettings& settings = AppSettings::instance();
    m_showOnlyLotwUsers = settings.getShowOnlyLotwUsers();
    m_showAllBands = settings.getShowAllBands();
    setMinimumWidth(200);
    setMinimumHeight(50);  // Allow very short windows (reduced from 300)
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // White background with black text (system default style)
    QPalette pal = viewport()->palette();
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Text, Qt::black);
    viewport()->setAutoFillBackground(true);
    viewport()->setPalette(pal);

    // Enable scrollbars as needed (both vertical and horizontal)
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable context menu
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // Load spots from database on startup
    loadSpotsFromDatabase();

    // Setup refresh timer for periodic cleanup and aging updates
    int refreshInterval = settings.getSpotRefreshIntervalMs();
    m_refreshTimer->setInterval(refreshInterval);
    connect(m_refreshTimer, &QTimer::timeout, this, &BandMapWidget::onRefreshTimer);
    m_refreshTimer->start();
    LOG_DEBUG("BandMapWidget", QString("Refresh timer started with %1ms interval").arg(refreshInterval));

    // Enable mouse tracking for tooltips
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    // Initialize scrollbar ranges
    updateScrollBars();

    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &BandMapWidget::applyTheme);
    applyTheme();
}

void BandMapWidget::addSpot(const Spot& spot) {
    // Apply LOTW filter if enabled
    if (m_showOnlyLotwUsers && !spot.isLotwUser) {
        // Remove spot if it exists but is not LOTW user when filter is active
        LOG_DEBUG("BandMapWidget", QString("Filtering out non-LOTW spot: %1").arg(spot.callsign));
        removeSpot(spot.callsign);
        return;
    }

    // Apply band filter if enabled (show current band only)
    if (!m_showAllBands && m_currentFrequency > 0) {
        QString currentBand = getBandFromFrequency(m_currentFrequency);
        QString spotBand = getBandFromFrequency(spot.frequency);
        LOG_DEBUG("BandMapWidget", QString("Band filter check: m_showAllBands=%1, current freq=%2 Hz (%3), spot %4 freq=%5 Hz (%6)")
            .arg(m_showAllBands).arg(m_currentFrequency).arg(currentBand)
            .arg(spot.callsign).arg(spot.frequency).arg(spotBand));
        if (!currentBand.isEmpty() && !spotBand.isEmpty() && currentBand != spotBand) {
            // Remove spot if it exists but is on different band when filter is active
            LOG_DEBUG("BandMapWidget", QString("Filtering out spot on different band: %1 on %2 (current band: %3)")
                .arg(spot.callsign).arg(spotBand).arg(currentBand));
            removeSpot(spot.callsign);
            return;
        }
    } else {
        LOG_DEBUG("BandMapWidget", QString("Band filter disabled or no current freq: m_showAllBands=%1, m_currentFrequency=%2")
            .arg(m_showAllBands).arg(m_currentFrequency));
    }

    // Check if spot already exists (update it)
    for (int i = 0; i < m_allSpots.size(); ++i) {
        if (m_allSpots[i].callsign == spot.callsign) {
            m_allSpots[i] = spot;
            rebuildDisplayList();
            return;
        }
    }

    // Add new spot
    m_allSpots.append(spot);
    rebuildDisplayList();
}

void BandMapWidget::removeSpot(const QString& callsign) {
    for (int i = 0; i < m_allSpots.size(); ++i) {
        if (m_allSpots[i].callsign == callsign) {
            m_allSpots.removeAt(i);
            rebuildDisplayList();
            return;
        }
    }
}

void BandMapWidget::clearSpots() {
    m_allSpots.clear();
    m_selectedIndex = -1;
    rebuildDisplayList();
}

void BandMapWidget::setCurrentFrequency(freq_t freq) {
    QString band = getBandFromFrequency(freq);
    LOG_DEBUG("BandMapWidget", QString("Current frequency changed: %1 Hz (%2 MHz) - Band: %3")
        .arg(freq).arg(freq / 1000000.0, 0, 'f', 3).arg(band.isEmpty() ? "unknown" : band));
    m_currentFrequency = freq;
    viewport()->update();
}

void BandMapWidget::refreshLotwStatus() {
    // Re-check LOTW status for all spots using current settings
    AppSettings& settings = AppSettings::instance();
    if (!settings.getEnableLotwLookup()) {
        // LOTW lookup disabled - mark all as not LOTW
        for (Spot& spot : m_allSpots) {
            spot.isLotwUser = false;
        }
        LOG_DEBUG("BandMapWidget", "Refreshed LOTW status: lookup disabled, cleared all LOTW flags");
    } else {
        // Re-check each spot
        LOTWUserRepository lotwRepo;
        int lotwCount = 0;
        for (Spot& spot : m_allSpots) {
            spot.isLotwUser = lotwRepo.isLotwUser(spot.callsign);
            if (spot.isLotwUser) {
                lotwCount++;
            }
        }
        LOG_DEBUG("BandMapWidget", QString("Refreshed LOTW status: %1 out of %2 spots are LOTW users")
            .arg(lotwCount).arg(m_allSpots.size()));
    }

    // Rebuild display list (this will apply LOTW filter if enabled)
    rebuildDisplayList();
}

void BandMapWidget::sortSpots() {
    if (m_sortMode == BandMapSortMode::Frequency) {
        // Sort by frequency (ascending)
        std::sort(m_displaySpots.begin(), m_displaySpots.end(),
                 [](const Spot& a, const Spot& b) {
                     return a.frequency < b.frequency;
                 });
    } else {
        // Sort alphabetically by callsign
        std::sort(m_displaySpots.begin(), m_displaySpots.end(),
                 [](const Spot& a, const Spot& b) {
                     return a.callsign < b.callsign;
                 });
    }
}

void BandMapWidget::calculateColumnLayout() {
    // Column-first layout: Calculate columns needed based on available height
    // Each column needs minimum 150 pixels (for "14025.0 M W1ABC" format)
    const int MIN_COLUMN_WIDTH = 150;
    m_columnWidth = MIN_COLUMN_WIDTH;

    // Calculate how many rows fit in available height
    QFontMetrics fm(QFont("Monospace", 9));
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewport()->height() - FOOTER_HEIGHT;
    int lineHeight = rowHeight();
    int maxRowsPerColumn = qMax(1, availableHeight / lineHeight);

    // Calculate how many columns needed to show all spots
    m_columnCount = (m_displaySpots.size() + maxRowsPerColumn - 1) / maxRowsPerColumn;
    if (m_columnCount < 1) m_columnCount = 1;

    // Note: If total width (m_columnCount * m_columnWidth) exceeds viewport width,
    // horizontal scrollbar will appear automatically
}

void BandMapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), Qt::white);

    QFont font("Monospace", 9);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int lineHeight = rowHeight();
    int scrollY = verticalScrollBar()->value();
    int scrollX = horizontalScrollBar()->value();
    int viewportHeight = viewport()->height();
    int viewportWidth = viewport()->width();

    // Reserve space at bottom for spot count
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewportHeight - FOOTER_HEIGHT;
    int rowsPerColumn = availableHeight / lineHeight;
    if (rowsPerColumn < 1) rowsPerColumn = 1;

    // Draw each spot in column-first layout (top-to-bottom, then left-to-right)
    for (int i = 0; i < m_displaySpots.size(); ++i) {
        const Spot& spot = m_displaySpots[i];

        // Column-first layout: spots flow top-to-bottom within each column
        int col = i / rowsPerColumn;  // Which column
        int row = i % rowsPerColumn;  // Which row in that column

        int x = col * m_columnWidth - scrollX;  // Account for horizontal scroll
        int y = row * lineHeight - scrollY;     // Account for vertical scroll

        // Skip if this spot would be off-screen (horizontally or vertically)
        if (x + m_columnWidth < 0 || x >= viewportWidth ||
            y + lineHeight < 0 || y >= availableHeight) {
            continue;
        }

        // Get age-based colors
        QColor textColor = getSpotTextColor(spot);
        QColor bgColor = getSpotBackgroundColor(spot);

        // Draw background for new/aging spots
        if (bgColor != Qt::white) {
            painter.fillRect(x + 2, y + 1, m_columnWidth - 4, lineHeight - 2, bgColor);
        }

        painter.setPen(textColor);

        // Draw frequency (left side of column)
        QString freqStr = formatFrequency(spot.frequency);
        painter.drawText(x + 5, y + fm.ascent() + 2, freqStr);

        // Draw "M" marker for multipliers
        if (spot.isMultiplier) {
            painter.setPen(Qt::red);
            painter.drawText(x + 50, y + fm.ascent() + 2, "M");
            painter.setPen(textColor);
        }

        // Draw "L" marker for LOTW users
        if (spot.isLotwUser) {
            painter.setPen(ThemeManager::instance().color(ColorRole::LotwUserText));
            painter.drawText(x + 60, y + fm.ascent() + 2, "L");
            painter.setPen(textColor);
        }

        // Draw callsign (right side of column entry)
        painter.drawText(x + 80, y + fm.ascent() + 2, spot.callsign);

        // Highlight selected spot with blue rectangle
        if (i == m_selectedIndex) {
            painter.setPen(QPen(Qt::blue, 2));
            painter.drawRect(x + 2, y + 1, m_columnWidth - 4, lineHeight - 2);
        }
    }

    // Draw status information at bottom (always visible, not scrolled)
    painter.setPen(Qt::darkBlue);
    QString statusStr;

    // If a spot is selected, show its details
    if (m_selectedIndex >= 0 && m_selectedIndex < m_displaySpots.size()) {
        const Spot& spot = m_displaySpots[m_selectedIndex];

        // Format: "W1ABC @ 14.200 MHz | comment text | LOTW | MULT"
        statusStr = QString("%1 @ %2 MHz")
            .arg(spot.callsign)
            .arg(spot.frequency / 1000000.0, 0, 'f', 3);

        if (!spot.comment.trimmed().isEmpty()) {
            statusStr += QString(" | %1").arg(spot.comment.trimmed());
        }

        if (spot.isLotwUser) {
            statusStr += " | LOTW";
        }

        if (spot.isMultiplier) {
            statusStr += " | MULT";
        }
    } else {
        // No spot selected, just show count
        statusStr = QString("%1 spots").arg(m_displaySpots.size());
    }

    int statusY = viewportHeight - fm.height() - 5;

    // Draw status text (left-aligned to show full information)
    painter.drawText(10, statusY + fm.ascent(), statusStr);
}

void BandMapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int spotIndex = findSpotAtPosition(event->pos());
        if (spotIndex >= 0 && spotIndex < m_displaySpots.size()) {
            m_selectedIndex = spotIndex;
            // Single click: QSY to frequency AND populate callsign
            emit qsyRequested(m_displaySpots[spotIndex].frequency);
            emit callsignSelected(m_displaySpots[spotIndex].callsign);
            viewport()->update();
        }
    }
}

void BandMapWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int spotIndex = findSpotAtPosition(event->pos());
        if (spotIndex >= 0 && spotIndex < m_displaySpots.size()) {
            emit callsignSelected(m_displaySpots[spotIndex].callsign);
        }
    }
}

bool BandMapWidget::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        int spotIndex = findSpotAtPosition(viewport()->mapFromGlobal(helpEvent->globalPos()));

        if (spotIndex >= 0 && spotIndex < m_displaySpots.size()) {
            const Spot& spot = m_displaySpots[spotIndex];

            // Show tooltip only for LOTW users
            if (spot.isLotwUser) {
                LOTWUserRepository lotwRepo;
                LOTWUser lotwUser = lotwRepo.findByCallsign(spot.callsign);

                if (!lotwUser.callsign.isEmpty()) {
                    QString tooltipText = QString("LOTW: Last upload %1 %2")
                        .arg(lotwUser.lastUploadDate)
                        .arg(lotwUser.lastUploadTime);
                    QToolTip::showText(helpEvent->globalPos(), tooltipText);
                } else {
                    QToolTip::hideText();
                    event->ignore();
                }

                return true;
            }
        }

        QToolTip::hideText();
        event->ignore();
        return true;
    }

    return QAbstractScrollArea::event(event);
}

void BandMapWidget::resizeEvent(QResizeEvent* event) {
    QAbstractScrollArea::resizeEvent(event);
    calculateColumnLayout();
    updateScrollBars();
    viewport()->update();
}

void BandMapWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);

    // Check if user right-clicked on a specific spot
    int spotIndex = findSpotAtPosition(event->pos());
    if (spotIndex >= 0 && spotIndex < m_displaySpots.size()) {
        const Spot& spot = m_displaySpots[spotIndex];
        QString deleteText = QString("Delete Spot: %1").arg(spot.callsign);
        QAction* deleteAction = menu.addAction(deleteText);

        // Capture callsign by value for the lambda
        QString callsign = spot.callsign;
        connect(deleteAction, &QAction::triggered, this, [this, callsign]() {
            removeSpot(callsign);
        });

        menu.addSeparator();
    }

    // Sort options
    QAction* sortByFreqAction = menu.addAction("Sort by Frequency");
    sortByFreqAction->setCheckable(true);
    sortByFreqAction->setChecked(m_sortMode == BandMapSortMode::Frequency);
    connect(sortByFreqAction, &QAction::triggered, this, [this]() {
        m_sortMode = BandMapSortMode::Frequency;
        sortSpots();
        viewport()->update();
    });

    QAction* sortByCallAction = menu.addAction("Sort by Callsign");
    sortByCallAction->setCheckable(true);
    sortByCallAction->setChecked(m_sortMode == BandMapSortMode::Callsign);
    connect(sortByCallAction, &QAction::triggered, this, [this]() {
        m_sortMode = BandMapSortMode::Callsign;
        sortSpots();
        viewport()->update();
    });

    menu.addSeparator();

    // LOTW filter
    QAction* lotwFilterAction = menu.addAction("Show only LOTW users");
    lotwFilterAction->setCheckable(true);
    lotwFilterAction->setChecked(m_showOnlyLotwUsers);
    connect(lotwFilterAction, &QAction::triggered, this, [this](bool checked) {
        m_showOnlyLotwUsers = checked;

        // Save setting
        AppSettings& settings = AppSettings::instance();
        settings.setShowOnlyLotwUsers(checked);

        // Rebuild display list to apply/remove filter
        rebuildDisplayList();
    });

    // Band filter
    QAction* bandFilterAction = menu.addAction("Show all bands");
    bandFilterAction->setCheckable(true);
    bandFilterAction->setChecked(m_showAllBands);
    connect(bandFilterAction, &QAction::triggered, this, [this](bool checked) {
        LOG_DEBUG("BandMapWidget", QString("Band filter toggled: Show all bands = %1").arg(checked));
        m_showAllBands = checked;

        // Save setting
        AppSettings& settings = AppSettings::instance();
        settings.setShowAllBands(checked);

        // Rebuild display list to apply/remove filter
        rebuildDisplayList();
    });

    menu.addSeparator();

    QAction* clearAction = menu.addAction("Clear All Spots");
    connect(clearAction, &QAction::triggered, this, &BandMapWidget::clearSpots);

    menu.exec(event->globalPos());
}

int BandMapWidget::findSpotAtPosition(const QPoint& pos) {
    int lineHeight = rowHeight();
    int scrollY = verticalScrollBar()->value();
    int scrollX = horizontalScrollBar()->value();

    QFontMetrics fm(QFont("Monospace", 9));
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewport()->height() - FOOTER_HEIGHT;
    int rowsPerColumn = qMax(1, availableHeight / lineHeight);

    // Determine which column was clicked (accounting for horizontal scroll)
    int col = (pos.x() + scrollX) / m_columnWidth;
    if (col < 0 || col >= m_columnCount) {
        return -1;
    }

    // Determine which row in that column was clicked (accounting for vertical scroll)
    int row = (pos.y() + scrollY) / lineHeight;
    if (row < 0 || row >= rowsPerColumn) {
        return -1;
    }

    // Calculate spot index from column-first layout (top-to-bottom, then left-to-right)
    int spotIndex = (col * rowsPerColumn) + row;

    if (spotIndex >= 0 && spotIndex < m_displaySpots.size()) {
        return spotIndex;
    }

    return -1;
}

int BandMapWidget::rowHeight() const {
    QFont font("Monospace", 9);
    QFontMetrics fm(font);
    return fm.height() + 4;  // 2px padding top and bottom
}

QString BandMapWidget::formatFrequency(freq_t freq) const {
    // Convert Hz to MHz with 3 decimal places (like TR4W)
    // Examples: 7.051, 14.074, 28.200
    // Max frequency: 435.000 MHz = 7 chars
    // Format with fixed width (right-aligned, 7 chars) for consistent column
    double freqMhz = freq / 1000000.0;
    QString result = QString("%1").arg(freqMhz, 7, 'f', 3);

    // Debug: log first few conversions to verify format
    static int debugCount = 0;
    if (debugCount < 5) {
        LOG_DEBUG("BandMapWidget", QString("formatFrequency: %1 Hz -> %2 MHz").arg(freq).arg(result));
        debugCount++;
    }

    return result;
}

void BandMapWidget::updateScrollBars() {
    // Column-first layout: Calculate scrollbar ranges for both directions
    int lineHeight = rowHeight();

    QFontMetrics fm(QFont("Monospace", 9));
    const int FOOTER_HEIGHT = fm.height() + 10;
    int availableHeight = viewport()->height() - FOOTER_HEIGHT;
    int availableWidth = viewport()->width();

    // Safety check
    if (m_columnCount < 1) m_columnCount = 1;

    // Calculate rows per column based on available height
    int rowsPerColumn = qMax(1, availableHeight / lineHeight);

    // Calculate actual number of spots in tallest column (column-first layout)
    // The first column fills up to rowsPerColumn before spilling to next column
    int spotsInTallestColumn = qMin(rowsPerColumn, m_displaySpots.size());
    int tallestColumnHeight = spotsInTallestColumn * lineHeight;

    // Vertical scrollbar: Needed if tallest column exceeds available height
    verticalScrollBar()->setPageStep(availableHeight);
    int vScrollRange = qMax(0, tallestColumnHeight - availableHeight);
    verticalScrollBar()->setRange(0, vScrollRange);

    // Horizontal scrollbar: Needed when columns extend beyond viewport width
    int totalWidth = m_columnCount * m_columnWidth;
    horizontalScrollBar()->setPageStep(availableWidth);
    int hScrollRange = qMax(0, totalWidth - availableWidth);
    horizontalScrollBar()->setRange(0, hScrollRange);

    LOG_DEBUG("BandMapWidget", QString("Scrollbar update: spots=%1, columns=%2, rowsPerCol=%3, spotInTallest=%4, tallestHeight=%5, availHeight=%6, vRange=%7, totalWidth=%8, availWidth=%9, hRange=%10")
        .arg(m_displaySpots.size()).arg(m_columnCount).arg(rowsPerColumn).arg(spotsInTallestColumn)
        .arg(tallestColumnHeight).arg(availableHeight).arg(vScrollRange)
        .arg(totalWidth).arg(availableWidth).arg(hScrollRange));
}

void BandMapWidget::scrollContentsBy(int dx, int dy) {
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    viewport()->update();
}

void BandMapWidget::applyTheme() {
    // Repaint to update colors
    viewport()->update();
}

QString BandMapWidget::getBandFromFrequency(freq_t freq) const {
    // Convert Hz to MHz
    double freqMHz = freq / 1000000.0;

    // Amateur radio band edges (in MHz)
    if (freqMHz >= 1.8 && freqMHz <= 2.0) return "160m";
    if (freqMHz >= 3.5 && freqMHz <= 4.0) return "80m";
    if (freqMHz >= 5.3 && freqMHz <= 5.4) return "60m";
    if (freqMHz >= 7.0 && freqMHz <= 7.3) return "40m";
    if (freqMHz >= 10.1 && freqMHz <= 10.15) return "30m";
    if (freqMHz >= 14.0 && freqMHz <= 14.35) return "20m";
    if (freqMHz >= 18.068 && freqMHz <= 18.168) return "17m";
    if (freqMHz >= 21.0 && freqMHz <= 21.45) return "15m";
    if (freqMHz >= 24.89 && freqMHz <= 24.99) return "12m";
    if (freqMHz >= 28.0 && freqMHz <= 29.7) return "10m";
    if (freqMHz >= 50.0 && freqMHz <= 54.0) return "6m";
    if (freqMHz >= 144.0 && freqMHz <= 148.0) return "2m";
    if (freqMHz >= 420.0 && freqMHz <= 450.0) return "70cm";

    return "";  // Not a ham band
}

void BandMapWidget::loadSpotsFromDatabase() {
    SpotRepository repo;
    m_allSpots = repo.loadAllSpots();

    LOG_INFO("BandMapWidget", QString("Loaded %1 spots from database").arg(m_allSpots.size()));

    // Rebuild display with loaded spots
    rebuildDisplayList();
}

void BandMapWidget::saveSpotsToDatabase() {
    // Remove expired spots before saving
    removeExpiredSpots();

    SpotRepository repo;
    if (!repo.saveAllSpots(m_allSpots)) {
        LOG_WARN("BandMapWidget", QString("Failed to save spots: %1").arg(repo.lastError()));
        return;
    }

    LOG_INFO("BandMapWidget", QString("Saved %1 spots to database").arg(m_allSpots.size()));
}

void BandMapWidget::updateSpotStatus(const QString& callsign, bool isWorked, bool isMultiplier) {
    for (Spot& spot : m_allSpots) {
        if (spot.callsign == callsign) {
            spot.isWorked = isWorked;
            spot.isMultiplier = isMultiplier;
            rebuildDisplayList();
            return;
        }
    }
}

void BandMapWidget::onRefreshTimer() {
    // Remove expired spots from master list
    removeExpiredSpots();

    // Rebuild display (updates aging colors and filters)
    rebuildDisplayList();
}

void BandMapWidget::removeExpiredSpots() {
    AppSettings& settings = AppSettings::instance();
    int expirySeconds = settings.getSpotExpirySeconds();
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-expirySeconds);

    int removedCount = 0;
    for (int i = m_allSpots.size() - 1; i >= 0; --i) {
        if (m_allSpots[i].timestamp < cutoffTime) {
            m_allSpots.removeAt(i);
            removedCount++;
        }
    }

    if (removedCount > 0) {
        LOG_DEBUG("BandMapWidget", QString("Removed %1 expired spots").arg(removedCount));
    }
}

void BandMapWidget::rebuildDisplayList() {
    m_displaySpots.clear();

    // Get expiry threshold
    AppSettings& settings = AppSettings::instance();
    int expirySeconds = settings.getSpotExpirySeconds();
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-expirySeconds);

    // Filter spots for display
    for (const Spot& spot : m_allSpots) {
        // Skip expired spots
        if (spot.timestamp < cutoffTime) {
            continue;
        }

        // Apply LOTW filter
        if (m_showOnlyLotwUsers && !spot.isLotwUser) {
            continue;
        }

        // Apply band filter
        if (!m_showAllBands && m_currentFrequency > 0) {
            QString currentBand = getBandFromFrequency(m_currentFrequency);
            QString spotBand = getBandFromFrequency(spot.frequency);
            if (!currentBand.isEmpty() && !spotBand.isEmpty() && currentBand != spotBand) {
                continue;
            }
        }

        // Add to display list
        m_displaySpots.append(spot);
    }

    sortSpots();
    calculateColumnLayout();
    updateScrollBars();
    viewport()->update();
}

BandMapWidget::SpotAge BandMapWidget::getSpotAge(const Spot& spot) const {
    AppSettings& settings = AppSettings::instance();
    int expirySeconds = settings.getSpotExpirySeconds();          // 600
    int newThreshold = settings.getNewSpotThresholdSeconds();     // 60
    int agingThreshold = settings.getAgingSpotThresholdSeconds(); // 120

    qint64 ageSeconds = spot.timestamp.secsTo(QDateTime::currentDateTime());

    if (ageSeconds < 0) ageSeconds = 0;  // Future timestamp protection

    if (ageSeconds < newThreshold) {
        return SpotAge::New;        // < 60 seconds
    } else if (ageSeconds >= expirySeconds) {
        return SpotAge::Expired;    // >= 600 seconds
    } else if (ageSeconds >= (expirySeconds - agingThreshold)) {
        return SpotAge::Aging;      // >= 480 seconds (last 2 minutes)
    } else {
        return SpotAge::Normal;     // 60-480 seconds
    }
}

QColor BandMapWidget::getSpotTextColor(const Spot& spot) const {
    ThemeManager& theme = ThemeManager::instance();

    // Priority: multiplier > worked > age-based
    if (spot.isMultiplier) {
        return theme.color(ColorRole::MultiplierText);
    } else if (spot.isWorked) {
        return theme.color(ColorRole::WorkedStationText);
    }

    // Age-based coloring for unworked stations
    SpotAge age = getSpotAge(spot);
    switch (age) {
        case SpotAge::New:
            return theme.color(ColorRole::NewSpotText);
        case SpotAge::Aging:
            return theme.color(ColorRole::AgingSpotText);
        default:
            return Qt::black;  // Normal
    }
}

QColor BandMapWidget::getSpotBackgroundColor(const Spot& spot) const {
    ThemeManager& theme = ThemeManager::instance();

    SpotAge age = getSpotAge(spot);

    if (age == SpotAge::New) {
        return theme.color(ColorRole::NewSpotBackground);
    } else if (age == SpotAge::Aging) {
        return theme.color(ColorRole::AgingSpotBackground);
    }

    return Qt::white;  // Normal background
}

} // namespace TR4QT

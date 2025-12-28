#include "NeedsDisplayWidget.h"
#include "../../utils/AppSettings.h"
#include <QFont>

namespace TR4QT {

NeedsDisplayWidget::NeedsDisplayWidget(QWidget* parent)
    : QWidget(parent)
    , m_fontSize(14)  // Larger font for better visibility
{
    // Ensure widget fills its background (prevents transparent/black rendering)
    setAutoFillBackground(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(10, 5, 10, 5);

    // Create labels
    m_qsoNeedsHeaderLabel = new QLabel(this);
    m_qsoNeedsBandsLabel = new QLabel(this);
    m_multNeedsHeaderLabel = new QLabel(this);
    m_multNeedsBandsLabel = new QLabel(this);

    // Set fonts (monospaced for alignment)
    QFont monoFont("Monospace", m_fontSize);
    m_qsoNeedsHeaderLabel->setFont(monoFont);
    m_qsoNeedsBandsLabel->setFont(monoFont);
    m_multNeedsHeaderLabel->setFont(monoFont);
    m_multNeedsBandsLabel->setFont(monoFont);

    // Enable HTML rendering for color coding
    m_qsoNeedsBandsLabel->setTextFormat(Qt::RichText);
    m_multNeedsBandsLabel->setTextFormat(Qt::RichText);

    // Set alignment
    m_qsoNeedsHeaderLabel->setAlignment(Qt::AlignLeft);
    m_qsoNeedsBandsLabel->setAlignment(Qt::AlignLeft);
    m_multNeedsHeaderLabel->setAlignment(Qt::AlignLeft);
    m_multNeedsBandsLabel->setAlignment(Qt::AlignLeft);

    // Add to layout
    layout->addWidget(m_qsoNeedsHeaderLabel);
    layout->addWidget(m_qsoNeedsBandsLabel);
    layout->addWidget(m_multNeedsHeaderLabel);
    layout->addWidget(m_multNeedsBandsLabel);
    layout->addStretch();

    // Initially clear
    clear();

    // Box-style border (left and bottom lines for emphasis)
    setStyleSheet("NeedsDisplayWidget { "
                  "border-left: 2px solid #404040; "
                  "border-bottom: 2px solid #404040; "
                  "border-top: 1px solid #808080; "
                  "border-right: 1px solid #808080; "
                  "background-color: #f5f5f5; "
                  "padding: 5px; "
                  "}");
}

void NeedsDisplayWidget::updateForCallsign(const QString& callsign,
                                          ContestBase* contest,
                                          const QList<BandType>& workedBands,
                                          const QList<BandType>& workedMultBands)
{
    if (callsign.isEmpty() || !contest) {
        clear();
        return;
    }

    // Get valid bands for this contest
    // Standard HF bands (160m through 10m)
    QList<BandType> allBands = {
        BandType::Band160M,
        BandType::Band80M,
        BandType::Band40M,
        BandType::Band20M,
        BandType::Band15M,
        BandType::Band10M
    };

    // Add VHF bands if enabled in settings
    AppSettings& settings = AppSettings::instance();
    if (settings.getVHFBandsEnabled()) {
        allBands.append(BandType::Band6M);
        allBands.append(BandType::Band2M);
    }

    // Update headers
    m_qsoNeedsHeaderLabel->setText(QString("QSO needs for %1:").arg(callsign));

    // Generate colored band list for QSOs
    QString qsoBandsHtml = generateBandListHtml(allBands, workedBands);
    m_qsoNeedsBandsLabel->setText(qsoBandsHtml);

    // Only show multiplier needs if contest uses multipliers
    if (contest->usesMultipliers()) {
        m_multNeedsHeaderLabel->setText(QString("Mult needs for %1:").arg(callsign));
        QString multBandsHtml = generateBandListHtml(allBands, workedMultBands);
        m_multNeedsBandsLabel->setText(multBandsHtml);
        m_multNeedsHeaderLabel->show();
        m_multNeedsBandsLabel->show();
    } else {
        // Hide multiplier row for contests that don't use multipliers
        m_multNeedsHeaderLabel->hide();
        m_multNeedsBandsLabel->hide();
    }

    // Show the widget
    show();
}

void NeedsDisplayWidget::clear()
{
    m_qsoNeedsHeaderLabel->setText("QSO needs:");
    m_qsoNeedsBandsLabel->setText("");
    m_multNeedsHeaderLabel->setText("Mult needs:");
    m_multNeedsBandsLabel->setText("");

    // Make sure mult labels are visible (may have been hidden for non-mult contests)
    m_multNeedsHeaderLabel->show();
    m_multNeedsBandsLabel->show();
}

void NeedsDisplayWidget::setFontSize(int pointSize)
{
    m_fontSize = pointSize;
    QFont monoFont("Monospace", m_fontSize);
    m_qsoNeedsHeaderLabel->setFont(monoFont);
    m_qsoNeedsBandsLabel->setFont(monoFont);
    m_multNeedsHeaderLabel->setFont(monoFont);
    m_multNeedsBandsLabel->setFont(monoFont);
}

QString NeedsDisplayWidget::generateBandListHtml(const QList<BandType>& allBands,
                                                 const QList<BandType>& workedBands)
{
    // Get color settings
    AppSettings& settings = AppSettings::instance();
    QString workedColor = settings.getNeedsDisplayWorkedColor();
    QString neededColor = settings.getNeedsDisplayNeededColor();

    QString html;

    for (BandType band : allBands) {
        QString bandName = getBandShortName(band);

        // Check if this band has been worked
        bool worked = workedBands.contains(band);

        if (worked) {
            // Already worked - use configured worked color
            html += QString("<span style='color: %1;'>%2</span> ")
                   .arg(workedColor, bandName);
        } else {
            // Still needed - highlight with configured needed color
            html += QString("<span style='color: #000000; background-color: %1; "
                          "padding: 2px 4px; font-weight: bold;'>%2</span> ")
                   .arg(neededColor, bandName);
        }
    }

    return html.trimmed();
}

QString NeedsDisplayWidget::getBandShortName(BandType band)
{
    switch (band) {
        case BandType::Band160M: return "160";
        case BandType::Band80M:  return "80";
        case BandType::Band40M:  return "40";
        case BandType::Band20M:  return "20";
        case BandType::Band15M:  return "15";
        case BandType::Band10M:  return "10";
        case BandType::Band6M:   return "6";
        case BandType::Band2M:   return "2";
        case BandType::Band70CM: return "70cm";
        default: return "??";
    }
}

} // namespace TR4QT

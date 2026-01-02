#include "InitialExchangeManager.h"
#include "../data/ExchangeMemoryRepository.h"
#include "../utils/CountryFile.h"
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"
#include "../contests/RSTValidator.h"
#include <QRegularExpression>
#include <QFile>

namespace TR4QT {

InitialExchangeManager::InitialExchangeManager()
    : m_cache(100)  // LRU cache for 100 most recent predictions
    , m_memoryRepo(new ExchangeMemoryRepository())
    , m_countryFile(new CountryFile())
{
    // Load country file from settings
    QString countryFilePath = AppSettings::instance().getCountryFilePath();
    if (QFile::exists(countryFilePath)) {
        if (!m_countryFile->loadFromFile(countryFilePath)) {
            LOG_WARN("InitialExchangeManager",
                     QString("Failed to load country file: %1").arg(countryFilePath));
        }
    }
}

InitialExchangeManager::~InitialExchangeManager() {
    delete m_memoryRepo;
    delete m_countryFile;
}

InitialExchangeManager& InitialExchangeManager::instance() {
    static InitialExchangeManager instance;
    return instance;
}

QString InitialExchangeManager::predictExchange(const QString& callsign,
                                               ContestBase* contest,
                                               ModeType mode) {
    if (!contest || callsign.length() < 2) {
        return QString();
    }

    // Check cache first
    QString cacheKey = QString("%1_%2").arg(callsign).arg(contest->getContestId());
    if (m_cache.contains(cacheKey)) {
        return *m_cache.object(cacheKey);
    }

    QString prediction;

    // Strategy 1: Exchange memory (exact match ONLY)
    QString memoryExchange = lookupMemory(callsign, contest->getContestId());
    if (!memoryExchange.isEmpty()) {
        LOG_DEBUG("InitialExchangeManager",
                  QString("Memory hit (exact): %1 → %2")
                  .arg(callsign, memoryExchange));
        prediction = memoryExchange;
    }

    // Strategy 2: CTY.DAT lookup (zone only)
    if (prediction.isEmpty()) {
        prediction = lookupCTY(callsign, contest);
        if (!prediction.isEmpty()) {
            LOG_DEBUG("InitialExchangeManager",
                      QString("CTY hit: %1 → %2")
                      .arg(callsign, prediction));
        }
    }

    // Strategy 3: Contest defaults (RST) - DISABLED
    // Don't auto-fill RST for contests with optional/user-entered RST.
    // This prevents pre-filling "599" in CQWPX when user just needs to enter serial.
    // User workflow:
    //   - Enter just serial: "3" → parsed as serial, RST auto-filled in parseReceivedExchange()
    //   - Enter RST + serial: "579 3" → parsed as RST + serial
    // Leaving exchange field empty allows user to decide what to enter.
    //
    // if (prediction.isEmpty()) {
    //     prediction = getDefaults(contest, mode);
    //     LOG_DEBUG("InitialExchangeManager",
    //               QString("Using defaults: %1 → %2")
    //               .arg(callsign, prediction));
    // }

    // Cache the prediction
    if (!prediction.isEmpty()) {
        m_cache.insert(cacheKey, new QString(prediction));
    }

    return prediction;
}

QString InitialExchangeManager::lookupMemory(const QString& callsign,
                                            const QString& contestType) {
    ExchangeMemoryEntry entry = m_memoryRepo->findExact(callsign, contestType);
    return entry.exchange;
}

QString InitialExchangeManager::lookupCTY(const QString& callsign,
                                         ContestBase* contest) {
    // Get contest's expected fields to know what to populate
    QList<ExchangeField> fields = contest->getReceivedExchangeFields();

    // Lookup country information
    CountryData countryData = m_countryFile->lookup(callsign);

    if (!countryData.isValid()) {
        return QString();
    }

    QMap<QString, QString> exchangeFields;

    // Populate fields based on what contest expects
    for (const ExchangeField& field : fields) {
        // Zone field
        if (field.name == "Zone" || field.name == "CQ Zone") {
            exchangeFields["Zone"] = QString::number(countryData.cqZone);
        }
        else if (field.name == "ITU Zone") {
            exchangeFields["ITU Zone"] = QString::number(countryData.ituZone);
        }
        // Section field (for US/VE calls)
        else if (field.name == "Section" || field.name == "QTH") {
            // Check if US or VE - countryData.name is the country name
            if (countryData.name == "United States" ||
                countryData.name == "Canada") {
                // Could populate state from CTY.DAT if available
                // For now, leave empty - user must enter
            }
        }
        // RST field (will be added by getDefaults)
        // Other fields cannot be auto-populated from CTY.DAT
    }

    // Build exchange string from fields
    return buildExchangeString(exchangeFields, contest);
}

QString InitialExchangeManager::getDefaults(ContestBase* contest, ModeType mode) {
    QList<ExchangeField> fields = contest->getReceivedExchangeFields();

    QMap<QString, QString> exchangeFields;

    // Add RST if contest expects it
    for (const ExchangeField& field : fields) {
        if (field.name == "RST" && field.autoFill) {
            exchangeFields["RST"] = RSTValidator::getDefault(mode);
            break;  // Only one RST field
        }
    }

    return buildExchangeString(exchangeFields, contest);
}

QString InitialExchangeManager::extractPrefix(const QString& callsign) const {
    if (callsign.length() < 2) {
        return callsign;
    }

    // Extract prefix: W1, K6, G3, etc.
    // Pattern: Letters followed by digit
    QRegularExpression re("^([A-Z]+\\d+)");
    QRegularExpressionMatch match = re.match(callsign.toUpper());

    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Fallback: first 2 characters
    return callsign.left(2).toUpper();
}

QString InitialExchangeManager::buildExchangeString(const QMap<QString, QString>& fields,
                                                    ContestBase* contest) const {
    if (fields.isEmpty()) {
        return QString();
    }

    // Get expected field order from contest
    QList<ExchangeField> expectedFields = contest->getReceivedExchangeFields();

    QStringList parts;
    for (const ExchangeField& expectedField : expectedFields) {
        if (fields.contains(expectedField.name)) {
            parts.append(fields[expectedField.name]);
        }
    }

    return parts.join(" ");
}

} // namespace TR4QT

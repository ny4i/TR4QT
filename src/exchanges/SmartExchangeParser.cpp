#include "SmartExchangeParser.h"
#include "../contests/ContestBase.h"
#include "../contests/ARRLSweepstakesContest.h"
#include "../contests/WinterFieldDayContest.h"
#include "../utils/ArrlSectionHelper.h"
#include <QRegularExpression>

namespace TR4QT {

QMap<QString, QString> SmartExchangeParser::parse(
    const QString& exchange,
    const QList<ExchangeField>& expectedFields,
    ContestBase* contest)
{
    QMap<QString, QString> result;

    if (exchange.trimmed().isEmpty() || expectedFields.isEmpty()) {
        return result;  // Empty result for invalid input
    }

    // Tokenize the exchange
    QList<Token> tokens = tokenize(exchange);

    if (tokens.isEmpty()) {
        return result;
    }

    // Try smart matching
    result = matchTokensToFields(tokens, expectedFields, contest);

    return result;
}

QList<SmartExchangeParser::Token> SmartExchangeParser::tokenize(const QString& exchange) {
    QList<Token> tokens;

    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    for (int i = 0; i < parts.size(); ++i) {
        QString part = parts[i].trimmed();
        if (!part.isEmpty()) {
            TokenType type = classifyToken(part);
            tokens.append(Token(part, type, i));
        }
    }

    return tokens;
}

SmartExchangeParser::TokenType SmartExchangeParser::classifyToken(const QString& token) {
    if (token.isEmpty()) {
        return TokenType::Unknown;
    }

    // Check if it's pure numeric
    bool isNumeric;
    token.toInt(&isNumeric);
    if (isNumeric) {
        // Could be RST, serial, check, or zone
        // Don't classify as RST here - let the context-aware matching decide
        // If contest expects RST, it will be matched; otherwise treated as serial
        return TokenType::Numeric;
    }

    // Check if it's pure alpha
    static QRegularExpression alphaRegex("^[A-Za-z]+$");
    if (alphaRegex.match(token).hasMatch()) {
        return TokenType::Alpha;
    }

    // Check if it's mixed alphanumeric
    static QRegularExpression mixedRegex("^[A-Za-z0-9]+$");
    if (mixedRegex.match(token).hasMatch()) {
        return TokenType::Mixed;
    }

    return TokenType::Unknown;
}

QMap<QString, QString> SmartExchangeParser::matchTokensToFields(
    const QList<Token>& tokens,
    const QList<ExchangeField>& expectedFields,
    ContestBase* contest)
{
    QMap<QString, QString> result;
    QList<Token> unmatchedTokens = tokens;  // Copy for processing

    // Build a map of field name → expected field for quick lookup
    QMap<QString, ExchangeField> fieldMap;
    for (const ExchangeField& field : expectedFields) {
        fieldMap[field.name] = field;
    }

    // Strategy: Match tokens to fields by type detection
    // Priority order:
    // 1. RST (if expected) - match RST-like tokens first
    // 2. Precedence (if Sweepstakes) - single letter Q/A/B/M/U/S
    // 3. Section - match against known section codes
    // 4. Class (if WFD) - mixed alphanumeric like "1O", "2I"
    // 5. Check (if Sweepstakes) - 2-digit number
    // 6. Serial - remaining numeric
    // 7. Zone - numeric (last resort)

    // Pass 1: Match high-confidence tokens

    for (int i = unmatchedTokens.size() - 1; i >= 0; --i) {
        const Token& token = unmatchedTokens[i];
        bool matched = false;

        // Check for RST (only if contest expects it)
        if (fieldMap.contains("RST") && !result.contains("RST")) {
            if (token.type == TokenType::Numeric && looksLikeRST(token.value)) {
                result["RST"] = token.value;
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for Precedence (ARRL SS specific)
        if (fieldMap.contains("Precedence") && !result.contains("Precedence")) {
            if (looksLikePrecedence(token.value)) {
                result["Precedence"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for Section (use contest validation if available)
        if (fieldMap.contains("Section") && !result.contains("Section")) {
            if (looksLikeSection(token.value, contest)) {
                result["Section"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for State (NAQP and other contests using state/province multipliers)
        // Process from end to beginning, so LAST state token wins for ambiguity
        if (fieldMap.contains("State") && !result.contains("State")) {
            if (looksLikeState(token.value)) {
                result["State"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for Class (WFD specific)
        if (fieldMap.contains("Class") && !result.contains("Class")) {
            if (looksLikeClass(token.value)) {
                result["Class"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for Power (ARRL DX)
        if (fieldMap.contains("Power") && !result.contains("Power")) {
            if (looksLikePower(token.value)) {
                result["Power"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for State/Power composite field (ARRL DX)
        // First try state, then try power
        if (fieldMap.contains("State/Power") && !result.contains("State/Power")) {
            if (looksLikeState(token.value)) {
                result["State/Power"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            } else if (looksLikePower(token.value)) {
                result["State/Power"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for State/Serial composite field (ARRL RTTY Roundup)
        if (fieldMap.contains("State/Serial") && !result.contains("State/Serial")) {
            if (looksLikeState(token.value)) {
                result["State/Serial"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
            // Serial will be handled in Pass 2
        }

        // Check for County (QSO Parties)
        if (fieldMap.contains("County") && !result.contains("County")) {
            if (looksLikeCounty(token.value, contest)) {
                result["County"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }
    }

    // Pass 2: Match numeric tokens (Check, Serial, Zone, CQ Zone, ITU Zone)

    for (int i = unmatchedTokens.size() - 1; i >= 0; --i) {
        const Token& token = unmatchedTokens[i];

        if (token.type != TokenType::Numeric) {
            continue;
        }

        bool matched = false;

        // Check for Check (2-digit year in ARRL SS)
        if (fieldMap.contains("Check") && !result.contains("Check")) {
            if (looksLikeCheck(token.value)) {
                result["Check"] = token.value;
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for Serial
        if (fieldMap.contains("Serial") && !result.contains("Serial")) {
            if (looksLikeSerial(token.value)) {
                result["Serial"] = token.value;
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for Zone (generic)
        if (fieldMap.contains("Zone") && !result.contains("Zone")) {
            result["Zone"] = token.value;
            unmatchedTokens.removeAt(i);
            matched = true;
            continue;
        }

        // Check for CQ Zone (explicit, range 1-40)
        if (fieldMap.contains("CQZone") && !result.contains("CQZone")) {
            if (looksLikeCQZone(token.value)) {
                result["CQZone"] = token.value;
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for ITU Zone (explicit, range 1-90)
        if (fieldMap.contains("ITUZone") && !result.contains("ITUZone")) {
            if (looksLikeITUZone(token.value)) {
                result["ITUZone"] = token.value;
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }

        // Check for State/Serial (numeric case - serial number from DX stations)
        if (fieldMap.contains("State/Serial") && !result.contains("State/Serial")) {
            if (looksLikeSerial(token.value)) {
                result["State/Serial"] = token.value;
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }
    }

    // Pass 3: Handle multi-word "Name" field (NAQP)
    // If we have a "Name" field and unmatched tokens, combine them all as the name
    if (fieldMap.contains("Name") && !result.contains("Name") && !unmatchedTokens.isEmpty()) {
        QStringList nameParts;
        for (const Token& t : unmatchedTokens) {
            nameParts.append(t.value);
        }
        result["Name"] = nameParts.join(" ");
        unmatchedTokens.clear();
    }

    // Pass 4: Fill in remaining unmatched fields with unmatched tokens (fallback)
    // This handles ambiguous cases by position

    int tokenIndex = 0;
    for (const ExchangeField& field : expectedFields) {
        if (!result.contains(field.name) && tokenIndex < unmatchedTokens.size()) {
            result[field.name] = unmatchedTokens[tokenIndex].value;
            tokenIndex++;
        }
    }

    return result;
}

// ===== Field Type Detection Helpers =====

bool SmartExchangeParser::looksLikeRST(const QString& token) {
    // RST reports: Readability (1-5), Signal (1-9), Tone (1-9 for CW)
    // Each digit must be valid per RST spec

    if (token.length() == 2) {
        // SSB: RS format (e.g., 59, 45)
        bool ok;
        int r = token.left(1).toInt(&ok);
        if (!ok) return false;
        int s = token.mid(1, 1).toInt(&ok);
        if (!ok) return false;
        // Readability 1-5, Signal 1-9 (0 is NOT valid!)
        return r >= 1 && r <= 5 && s >= 1 && s <= 9;
    } else if (token.length() == 3) {
        // CW: RST format (e.g., 599, 579)
        bool ok;
        int r = token.left(1).toInt(&ok);
        if (!ok) return false;
        int s = token.mid(1, 1).toInt(&ok);
        if (!ok) return false;
        int t = token.mid(2, 1).toInt(&ok);
        if (!ok) return false;
        // Readability 1-5, Signal 1-9, Tone 1-9 (0 is NOT valid!)
        return r >= 1 && r <= 5 && s >= 1 && s <= 9 && t >= 1 && t <= 9;
    }

    return false;
}

bool SmartExchangeParser::looksLikePrecedence(const QString& token) {
    if (token.length() != 1) {
        return false;
    }

    QString upper = token.toUpper();
    return (upper == "Q" || upper == "A" || upper == "B" ||
            upper == "M" || upper == "U" || upper == "S");
}

bool SmartExchangeParser::looksLikeCheck(const QString& token) {
    if (token.length() != 2) {
        return false;
    }

    bool ok;
    int val = token.toInt(&ok);
    return (ok && val >= 0 && val <= 99);
}

bool SmartExchangeParser::looksLikeSerial(const QString& token) {
    bool ok;
    int val = token.toInt(&ok);
    if (!ok) return false;

    // Serial numbers are typically 1-9999
    // Exclude RST-like numbers and 2-digit checks
    if (looksLikeRST(token)) {
        return false;
    }

    return (val >= 1 && val <= 9999);
}

bool SmartExchangeParser::looksLikeSection(const QString& token, ContestBase* contest) {
    Q_UNUSED(contest);  // Contest parameter not needed - using centralized helper

    if (token.length() < 2 || token.length() > 4) {
        return false;  // Sections are typically 2-4 characters
    }

    // Check if it's pure alpha (sections are letter codes)
    static QRegularExpression alphaRegex("^[A-Za-z]+$");
    if (!alphaRegex.match(token).hasMatch()) {
        return false;
    }

    // Use centralized ARRL section validation
    return Arrl::isValidSection(token);
}

bool SmartExchangeParser::looksLikeClass(const QString& token) {
    QString upper = token.toUpper();

    // Class format: [1-99] + [Category Letter]
    // Winter Field Day categories: I (indoor), O (outdoor), H (home), M (mobile)
    // ARRL Field Day categories: A, B, C, D, E, F
    //
    // Valid examples: 1O, 2I, 10H, 22M, 99I, 1A, 2B, 20F
    // Note: "HOME" is NOT valid - must be number + letter

    // Must be 2-3 characters (digit(s) + letter)
    if (token.length() < 2 || token.length() > 3) {
        return false;
    }

    // Extract number part and letter part
    QString numberPart;
    QString letterPart;

    // Parse from left: digits first, then letters
    int i = 0;
    while (i < upper.length() && upper[i].isDigit()) {
        numberPart += upper[i];
        i++;
    }

    while (i < upper.length() && upper[i].isLetter()) {
        letterPart += upper[i];
        i++;
    }

    // Should have consumed entire string
    if (i != upper.length()) {
        return false;
    }

    // Validate number part (1-20 transmitters)
    if (numberPart.isEmpty()) {
        return false;
    }

    bool ok;
    int transmitters = numberPart.toInt(&ok);
    // WFD allows unlimited transmitters (using 1-99 for validation)
    // ARRL FD limits to 1-20 (enforced by contest-specific validator)
    if (!ok || transmitters < 1 || transmitters > 99) {
        return false;
    }

    // Validate letter part (category)
    if (letterPart.length() != 1) {
        return false;
    }

    QChar category = letterPart[0];

    // WFD categories: I, O, H, M
    // ARRL FD categories: A, B, C, D, E, F
    static const QString validCategories = "IOHM ABCDEF";

    return validCategories.contains(category);
}

bool SmartExchangeParser::looksLikeState(const QString& token) {
    // State/province codes are 2-3 characters (US states are 2, some Canadian provinces are 2-3)
    if (token.length() < 2 || token.length() > 3) {
        return false;
    }

    // Must be pure alpha (no digits)
    static QRegularExpression alphaRegex("^[A-Za-z]+$");
    if (!alphaRegex.match(token).hasMatch()) {
        return false;
    }

    // Use centralized US states + Canadian provinces list
    // This is different from isValidSection() which validates ARRL sections like EMA, WMA, etc.
    // States are: AL, AK, AZ... (50 US states)
    // Provinces are: AB, BC, MB... (Canadian provinces including Ontario subdivisions)
    static QStringList validStates = Arrl::getStatesAndProvinces();
    return validStates.contains(token.toUpper());
}

bool SmartExchangeParser::looksLikePower(const QString& token) {
    // Power values: numeric watts (5, 100, 500, 1500) or with K suffix (1K, 1.5K)
    QString upper = token.toUpper();

    // Check for K suffix (kilowatts)
    if (upper.endsWith("K")) {
        QString numPart = upper.left(upper.length() - 1);
        bool ok;
        double kw = numPart.toDouble(&ok);
        // Valid range: 0.001K to 2K (1W to 2000W)
        return ok && kw >= 0.001 && kw <= 2.0;
    }

    // Check for W suffix (watts)
    if (upper.endsWith("W")) {
        QString numPart = upper.left(upper.length() - 1);
        bool ok;
        int watts = numPart.toInt(&ok);
        // Valid range: 1W to 2000W
        return ok && watts >= 1 && watts <= 2000;
    }

    // Plain numeric (watts)
    bool ok;
    int watts = token.toInt(&ok);
    if (!ok) return false;

    // Typical power range: 1-2000 watts
    // Exclude RST-like values (59, 599) and zone-like values (1-40)
    // Power is typically 5, 10, 50, 100, 200, 500, 1000, 1500
    if (looksLikeRST(token)) {
        return false;  // RST takes priority
    }

    return watts >= 1 && watts <= 2000;
}

bool SmartExchangeParser::looksLikeCQZone(const QString& token) {
    bool ok;
    int zone = token.toInt(&ok);
    if (!ok) return false;

    // CQ Zones are 1-40
    return zone >= 1 && zone <= 40;
}

bool SmartExchangeParser::looksLikeITUZone(const QString& token) {
    bool ok;
    int zone = token.toInt(&ok);
    if (!ok) return false;

    // ITU Zones are 1-90
    return zone >= 1 && zone <= 90;
}

bool SmartExchangeParser::looksLikeCounty(const QString& token, ContestBase* contest) {
    // County codes are typically 3-5 letters (e.g., ALC, DAD, HIL for Florida)
    if (token.length() < 2 || token.length() > 5) {
        return false;
    }

    // Must be pure alpha
    static QRegularExpression alphaRegex("^[A-Za-z]+$");
    if (!alphaRegex.match(token).hasMatch()) {
        return false;
    }

    // If we have a contest, delegate to contest-specific validation
    if (contest) {
        // Use contest's validateReceivedExchange to check if it's a valid county
        // This is a heuristic - the contest knows its own county codes
        QString testExchange = QString("599 %1").arg(token);
        QString errorMsg;
        // If validation passes with this as the exchange, it's likely a county
        // This is imperfect but better than nothing
    }

    // General heuristic: 3-letter uppercase codes that aren't states/sections
    QString upper = token.toUpper();
    if (token.length() == 3 && !looksLikeState(token) && !Arrl::isValidSection(token)) {
        return true;  // Could be a county
    }

    return false;
}

} // namespace TR4QT

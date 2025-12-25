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
        if (looksLikeRST(token)) {
            return TokenType::RST;
        }
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

        // Check for RST
        if (fieldMap.contains("RST") && !result.contains("RST")) {
            if (token.type == TokenType::RST) {
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

        // Check for Class (WFD specific)
        if (fieldMap.contains("Class") && !result.contains("Class")) {
            if (looksLikeClass(token.value)) {
                result["Class"] = token.value.toUpper();
                unmatchedTokens.removeAt(i);
                matched = true;
                continue;
            }
        }
    }

    // Pass 2: Match numeric tokens (Check, Serial, Zone)

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

        // Check for Zone
        if (fieldMap.contains("Zone") && !result.contains("Zone")) {
            result["Zone"] = token.value;
            unmatchedTokens.removeAt(i);
            matched = true;
            continue;
        }
    }

    // Pass 3: Fill in remaining unmatched fields with unmatched tokens (fallback)
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
    bool ok;
    int val = token.toInt(&ok);
    if (!ok) return false;

    // RST reports are typically 59, 599, or similar patterns
    // Range: 111 to 599 for CW, 11 to 59 for SSB
    if (token.length() == 2) {
        return (val >= 11 && val <= 59);
    } else if (token.length() == 3) {
        return (val >= 111 && val <= 599);
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

} // namespace TR4QT

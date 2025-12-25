#ifndef SMARTEXCHANGEPARSER_H
#define SMARTEXCHANGEPARSER_H

#include <QString>
#include <QStringList>
#include <QMap>

namespace TR4QT {

// Forward declarations
struct ExchangeField;
class ContestBase;

/**
 * Smart Exchange Parser
 *
 * Analyzes exchange input and automatically matches fields to expected
 * values, allowing fields to be entered in any order. Uses pattern
 * recognition and contest-specific validation to identify field types.
 *
 * Examples:
 * - ARRL SS: "1 M" → Serial=1, Precedence=M (missing check/section)
 * - ARRL SS: "M 95 WMA 123" → Serial=123, Prec=M, Check=95, Section=WMA
 * - WFD: "WCF 1A" or "1A WCF" → Class=1A, Section=WCF
 *
 * Limitations:
 * - Requires fields to be space-delimited
 * - May have ambiguity with pure numeric fields (serial vs. check vs. zone)
 * - Falls back to positional parsing if smart matching fails
 */
class SmartExchangeParser {
public:
    /**
     * Parse exchange with automatic field recognition and reordering
     *
     * @param exchange Raw exchange string from user input
     * @param expectedFields List of expected fields from contest
     * @param contest Contest instance for validation (can be nullptr)
     * @return Parsed exchange map (field name → value), empty if parsing fails
     */
    static QMap<QString, QString> parse(
        const QString& exchange,
        const QList<ExchangeField>& expectedFields,
        ContestBase* contest = nullptr
    );

private:
    enum class TokenType {
        Numeric,         // Pure number (e.g., "5", "123", "95")
        Alpha,           // Pure letters (e.g., "M", "WMA", "CT")
        Mixed,           // Alphanumeric (e.g., "1A", "2O", "3I")
        RST,             // Looks like RST signal report (599, 59, 579)
        Unknown
    };

    struct Token {
        QString value;
        TokenType type;
        int position;  // Original position in input

        Token() : type(TokenType::Unknown), position(0) {}
        Token(const QString& v, TokenType t, int p)
            : value(v), type(t), position(p) {}
    };

    // Tokenization and classification
    static QList<Token> tokenize(const QString& exchange);
    static TokenType classifyToken(const QString& token);

    // Matching logic
    static QMap<QString, QString> matchTokensToFields(
        const QList<Token>& tokens,
        const QList<ExchangeField>& expectedFields,
        ContestBase* contest
    );

    // Field type detection helpers
    static bool looksLikeRST(const QString& token);
    static bool looksLikePrecedence(const QString& token);
    static bool looksLikeCheck(const QString& token);
    static bool looksLikeSerial(const QString& token);
    static bool looksLikeSection(const QString& token, ContestBase* contest);
    static bool looksLikeClass(const QString& token);
};

} // namespace TR4QT

#endif // SMARTEXCHANGEPARSER_H

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

public:
    // State detection (for NAQP and other contests using state/province multipliers)
    static bool looksLikeState(const QString& token);

    // Power detection (for ARRL DX - numeric power values in watts)
    static bool looksLikePower(const QString& token);

    // Power normalization - converts K/KW to watts (e.g., "1K" -> "1000", "1.5KW" -> "1500")
    // Also expands CW cut numbers (e.g., "NN" -> "99", "5NN" -> "599")
    static QString normalizePower(const QString& token);

    // Expand CW cut numbers to digits: T=0, A=1, U=2, V=3, E=5, B=7, D=8, N=9
    static QString expandCutNumbers(const QString& token);

    // CQ Zone detection (for CQ WW, IARU - range 1-40)
    static bool looksLikeCQZone(const QString& token);

    // ITU Zone detection (for IARU HF - range 1-90)
    static bool looksLikeITUZone(const QString& token);

    // County detection (for QSO Parties - 3-letter codes)
    // Note: Requires contest-specific validation - this is a general heuristic
    static bool looksLikeCounty(const QString& token, ContestBase* contest);
};

} // namespace TR4QT

#endif // SMARTEXCHANGEPARSER_H

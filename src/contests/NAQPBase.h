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

#ifndef NAQPBASE_H
#define NAQPBASE_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * Base class for all North American QSO Party contests
 * Provides shared logic for exchange, scoring, and multipliers
 * Mode-specific subclasses handle dates and mode restrictions
 */
class NAQPBase : public ContestBase {
protected:
    explicit NAQPBase(const StationInfo& myStation)
        : ContestBase(myStation) {}

public:
    ~NAQPBase() override = default;

    // ===== Exchange Configuration =====

    QList<ExchangeField> getReceivedExchangeFields() const override {
        QList<ExchangeField> fields;

        // Name (no RST in NAQP!)
        ExchangeField name;
        name.name = "Name";
        name.hint = "First name";
        name.autoFill = false;
        name.maxLength = 20;
        fields.append(name);

        // State/Province
        ExchangeField state;
        state.name = "State";
        state.hint = "State/Province";
        state.autoFill = false;
        state.maxLength = 4;
        fields.append(state);

        return fields;
    }

    QList<ExchangeField> getSentExchangeFields() const override {
        QList<ExchangeField> fields;

        // Name
        ExchangeField name;
        name.name = "Name";
        name.hint = "Your first name";
        name.autoFill = true;
        name.maxLength = 20;
        fields.append(name);

        // State/Province
        ExchangeField state;
        state.name = "State";
        state.hint = "Your State/Province";
        state.autoFill = true;
        state.maxLength = 4;
        fields.append(state);

        return fields;
    }

    QList<TableColumn> getTableColumns() const override {
        return {
            TableColumn("Name", "Name", 100, TableColumn::Alignment::Left),
            TableColumn("State", "QTH", 60, TableColumn::Alignment::Left)
        };
    }

    QString formatSentExchange(int serialNumber, const QString& rst) const override {
        Q_UNUSED(serialNumber);
        Q_UNUSED(rst);
        return "{NAME} {STATE}";
    }

    /**
     * Configuration fields for contest creation dialog
     * NAQP needs: Name and State/Province
     */
    QList<ContestConfigField> getConfigFields() const override {
        return {
            ContestConfigField("NAME", "Contest Name:", "First name (e.g., TOM)",
                              "Station/firstName", 20, true),
            ContestConfigField("STATE", "State/Province:", "e.g., FL, ON",
                              "Station/state", 3, true)
        };
    }

    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

    // ===== Scoring =====

    int calculateQSOPoints(const QSO& qso, const StationInfo& myStation) const override {
        Q_UNUSED(qso);
        Q_UNUSED(myStation);
        return 1;  // 1 point per QSO
    }

    int calculateTotalScore(int totalQSOPoints,
                          const QMap<MultiplierType, int>& multiplierCounts) const override {
        // NAQP scoring: QSOs × sum of all multipliers across all bands
        int totalMults = 0;
        for (int count : multiplierCounts.values()) {
            totalMults += count;
        }
        return totalQSOPoints * totalMults;
    }

    // ===== Multipliers =====

    QList<MultiplierDefinition> getMultiplierTypes() const override {
        return {
            {MultiplierType::State, MultiplierScope::PerBand, "States/Provinces"}
        };
    }

    QString getMultiplierValue(const QSO& qso,
                             MultiplierType multType,
                             const QStringList& alreadyWorkedValues) const override {
        Q_UNUSED(multType);
        QString state = qso.state.toUpper();
        if (!state.isEmpty() && !alreadyWorkedValues.contains(state)) {
            return state;
        }
        return "";
    }

    // ===== Rules =====

    bool usesSerialNumbers() const override { return false; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;
    }
};

} // namespace TR4QT

#endif // NAQPBASE_H

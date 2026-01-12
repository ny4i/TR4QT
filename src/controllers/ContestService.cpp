/**
 * ContestService - Business logic for contest operations
 */

#include "ContestService.h"
#include "../contests/ContestBase.h"
#include "../data/ContestRepository.h"
#include "../data/QSORepository.h"
#include "../ui/models/QSOTableModel.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

ContestService::ContestService(const Config& config)
    : m_config(config)
    , m_contestRepository(new ContestRepository())
    , m_qsoRepository(new QSORepository())
{
}

ContestService::~ContestService() {
    delete m_contestRepository;
    delete m_qsoRepository;
}

void ContestService::updateConfig(const Config& config) {
    m_config = config;
}

ContestService::UpdateExchangeResult ContestService::updateContestExchange(const QString& newExchange) {
    UpdateExchangeResult result;

    // Validate inputs
    if (!m_config.activeContest) {
        result.errorMessage = "No active contest";
        LOG_WARN("ContestService", result.errorMessage);
        return result;
    }

    if (m_config.currentContestDbId < 0) {
        result.errorMessage = "Invalid contest database ID";
        LOG_WARN("ContestService", result.errorMessage);
        return result;
    }

    LOG_INFO("ContestService", QString("Updating contest exchange to: \"%1\"").arg(newExchange));

    // Step 1: Update contest record in database
    if (!m_contestRepository->updateExchange(m_config.currentContestDbId, newExchange)) {
        result.errorMessage = QString("Failed to update contest exchange: %1")
            .arg(m_contestRepository->lastError());
        LOG_ERROR("ContestService", result.errorMessage);
        return result;
    }

    // Step 2: Update active contest instance
    m_config.activeContest->setExchangeSent(newExchange);

    // Step 3: Update all QSO records with new exchange
    QList<QSO> qsos = m_qsoRepository->findByContest(m_config.currentContestDbId);

    LOG_INFO("ContestService", QString("Updating %1 QSO records with new exchange")
        .arg(qsos.size()));

    int updatedCount = 0;
    for (QSO& qso : qsos) {
        // Recalculate exchange using contest's formatSentExchange
        QString newExchangeSent = m_config.activeContest->formatSentExchange(
            qso.serialNumber, qso.rstSent
        );
        qso.exchangeSent = newExchangeSent;

        // Update in database
        if (!m_qsoRepository->updateQSO(qso)) {
            result.errorMessage = QString("Failed to update QSO %1: %2")
                .arg(qso.callsign).arg(m_qsoRepository->lastError());
            LOG_ERROR("ContestService", result.errorMessage);
            return result;
        }

        // Update in table model (if provided)
        if (m_config.qsoTableModel) {
            // Find row index for this QSO
            int rowCount = m_config.qsoTableModel->count();
            for (int row = 0; row < rowCount; ++row) {
                QSO modelQso = m_config.qsoTableModel->getQSO(row);
                if (modelQso.guid == qso.guid) {
                    m_config.qsoTableModel->updateQSO(row, qso);
                    break;
                }
            }
        }

        updatedCount++;
    }

    result.success = true;
    result.qsosUpdated = updatedCount;
    result.statusMessage = QString("Exchange updated successfully. %1 QSO(s) updated.")
        .arg(updatedCount);

    LOG_INFO("ContestService", result.statusMessage);

    return result;
}

} // namespace TR4QT

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

#include "CWMessageManager.h"
#include "../utils/AppSettings.h"
#include "../cw/CWTemplateEngine.h"
#include "../logging/LogMacros.h"
#include "../contests/RSTValidator.h"

namespace TR4QT {

CWMessageManager::CWMessageManager(const Config& config)
    : m_config(config)
    , m_lastCWMessage()
{
}

CWMessageManager::Result CWMessageManager::sendFunctionKey(int fKey, bool ctrlPressed, bool altPressed, const Input& input) {
    Result result;

    // Determine which message to send based on modifier keys and operating mode
    bool isCQMode = (input.operatingMode == OperatingMode::CQ);
    QString messageTemplate;

    if (ctrlPressed) {
        // Ctrl+F1-F12
        messageTemplate = AppSettings::instance().getCtrlFMessage(fKey, isCQMode);
    } else if (altPressed) {
        // Alt+F1-F12
        messageTemplate = AppSettings::instance().getAltFMessage(fKey, isCQMode);
    } else {
        // Regular F1-F12
        messageTemplate = isCQMode
            ? AppSettings::instance().getCQMessage(fKey)
            : AppSettings::instance().getSPMessage(fKey);
    }

    // Format key name for logging and status
    QString keyName = getKeyName(fKey, ctrlPressed, altPressed);
    QString modeStr = isCQMode ? "CQ" : "S&P";

    // Check if message is defined
    if (messageTemplate.isEmpty()) {
        result.success = false;
        result.statusMessage = QString("%1: No message defined").arg(keyName);
        LOG_INFO("CWMessageManager", QString("%1 (%2 mode): No message defined").arg(keyName).arg(modeStr));
        return result;
    }

    LOG_INFO("CWMessageManager", QString("%1 (%2 mode): Sending template: %3").arg(keyName).arg(modeStr).arg(messageTemplate));

    // Send the CW message
    return sendCWMessage(messageTemplate, input);
}

CWMessageManager::Result CWMessageManager::sendCWMessage(const QString& messageTemplate, const Input& input) {
    Result result;

    // Validate preconditions (radio connected, CW mode)
    if (!validatePreconditions(input, result.errorMessage)) {
        result.success = false;
        result.statusMessage = result.errorMessage;
        return result;
    }

    // Build substitution context
    CWTemplateEngine::Context ctx;
    ctx.myCall = AppSettings::instance().getMyCallsign();
    ctx.hisCall = input.callsign.trimmed().toUpper();
    ctx.qsoNumber = input.qsoNumber;
    ctx.mode = input.radioState.modeA;
    ctx.band = input.radioState.bandA;

    // Get sent exchange from contest (for S&P_EXCHANGE substitution)
    if (m_config.contest) {
        ctx.contestName = m_config.contest->getContestName();

        // Use the contest's formatSentExchange method to get properly formatted exchange
        QString rst = RSTValidator::getDefault(ctx.mode);
        ctx.sentExchange = m_config.contest->formatSentExchange(ctx.qsoNumber, rst);
    }

    // Substitute template variables
    QString cwText = CWTemplateEngine::substitute(messageTemplate, ctx);

    // Send via radio
    int wpm = AppSettings::instance().getMorseWPM();
    m_config.radio->setCWSpeed(wpm);
    m_config.radio->sendCW(cwText);

    // Save for repeat (= key) - per radio
    m_lastCWMessage[m_activeRadioIndex] = cwText;

    // Success
    result.success = true;
    result.cwTextSent = cwText;
    result.statusMessage = QString("Sending CW: %1").arg(cwText);

    LOG_INFO("CWMessageManager", QString("Sent CW: %1 (from template: %2)")
             .arg(cwText).arg(messageTemplate));

    return result;
}

QString CWMessageManager::getLastCWMessage() const {
    return m_lastCWMessage[m_activeRadioIndex];
}

CWMessageManager::Result CWMessageManager::repeatLastCWMessage(const Input& input) {
    Result result;

    // Check if there's a message to repeat for this radio
    const QString& lastMsg = m_lastCWMessage[m_activeRadioIndex];
    if (lastMsg.isEmpty()) {
        result.success = false;
        result.statusMessage = QString("No CW message to repeat for Radio %1").arg(m_activeRadioIndex + 1);
        LOG_INFO("CWMessageManager", QString("= key pressed but no previous CW message for Radio %1").arg(m_activeRadioIndex + 1));
        return result;
    }

    // Validate preconditions (radio connected, CW mode)
    if (!validatePreconditions(input, result.errorMessage)) {
        result.success = false;
        result.statusMessage = result.errorMessage;
        return result;
    }

    // Send the last message (no template substitution needed, already substituted)
    int wpm = AppSettings::instance().getMorseWPM();
    m_config.radio->setCWSpeed(wpm);
    m_config.radio->sendCW(lastMsg);

    // Success
    result.success = true;
    result.cwTextSent = lastMsg;
    result.statusMessage = QString("Repeating CW: %1").arg(lastMsg);

    LOG_INFO("CWMessageManager", QString("Repeated CW (Radio %1): %2").arg(m_activeRadioIndex + 1).arg(lastMsg));

    return result;
}

bool CWMessageManager::validatePreconditions(const Input& input, QString& errorMessage) const {
    // Check radio connection
    if (!m_config.radio) {
        errorMessage = "CW requires radio connection";
        LOG_WARN("CWMessageManager", "Cannot send CW: radio not connected");
        return false;
    }

    // Check for CW mode
    bool isCWMode = (input.radioState.modeA == ModeType::CW ||
                     input.radioState.modeA == ModeType::CWR);
    if (!isCWMode) {
        errorMessage = "CW requires CW mode";
        LOG_WARN("CWMessageManager", "Cannot send CW: not in CW mode");
        return false;
    }

    return true;
}

QString CWMessageManager::getKeyName(int fKey, bool ctrlPressed, bool altPressed) const {
    QString keyName = QString("F%1").arg(fKey);
    if (ctrlPressed) {
        keyName = "Ctrl+" + keyName;
    }
    if (altPressed) {
        keyName = "Alt+" + keyName;
    }
    return keyName;
}

} // namespace TR4QT

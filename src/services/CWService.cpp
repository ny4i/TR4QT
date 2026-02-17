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

#include "CWService.h"
#include "../keyers/KeyerController.h"
#include "../keyers/IambicKeyer.h"
#include "../utils/AppSettings.h"
#include "../cw/CWTemplateEngine.h"
#include "../cw/CWSender.h"
#include "../cw/CWSenderFactory.h"
#include "../cw/DtrRtsCWSender.h"
#include "../contests/ContestBase.h"
#include "../contests/RSTValidator.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

CWService::CWService(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_lastCWMessage()
{
    // Create keyer controller (owns worker thread for hardware I/O)
    m_keyerController = new KeyerController(this);

    // Create iambic keyer engine (main thread, generates key-down/key-up from paddle events)
    m_iambicKeyer = new IambicKeyer(this);

    // Initialize from settings
    auto& settings = AppSettings::instance();
    m_iambicKeyer->setWpm(settings.getMorseWPM());
    m_iambicKeyer->setMode(settings.getKeyerIambicMode() == 0
                           ? IambicMode::IambicA : IambicMode::IambicB);

    // Wire paddle state from keyer controller to iambic keyer
    connect(m_keyerController, &KeyerController::paddleStateChanged,
            m_iambicKeyer, &IambicKeyer::updatePaddleState);

    // Create CW sender based on current output mode setting
    m_outputMode = static_cast<OutputMode>(settings.getCWKeyingSource());
    createSender();

    // Wire iambic keyer to appropriate output
    wireKeyerToOutput();

    LOG_DEBUG("CWService", QString("CW service initialized (mode=%1)")
              .arg(static_cast<int>(m_outputMode)));
}

CWService::~CWService()
{
    destroySender();
}

// --- CW Output Mode ---

void CWService::setCWOutputMode(OutputMode mode)
{
    if (m_outputMode == mode) return;

    LOG_INFO("CWService", QString("Switching CW output mode: %1 -> %2")
             .arg(static_cast<int>(m_outputMode)).arg(static_cast<int>(mode)));

    // Disconnect keyer from old output
    QObject::disconnect(m_iambicKeyer, &IambicKeyer::keyDown, nullptr, nullptr);
    QObject::disconnect(m_iambicKeyer, &IambicKeyer::keyUp, nullptr, nullptr);

    m_outputMode = mode;

    // Recreate sender for new mode
    destroySender();
    createSender();

    // Rewire keyer to new output
    wireKeyerToOutput();
}

// --- CW Messaging (absorbed from CWMessageManager) ---

CWService::Result CWService::sendFunctionKey(int fKey, bool ctrlPressed, bool altPressed, const Input& input)
{
    Result result;

    // Determine which message to send based on modifier keys and operating mode
    bool isCQMode = (input.operatingMode == OperatingMode::CQ);
    QString messageTemplate;

    if (ctrlPressed) {
        messageTemplate = AppSettings::instance().getCtrlFMessage(fKey, isCQMode);
    } else if (altPressed) {
        messageTemplate = AppSettings::instance().getAltFMessage(fKey, isCQMode);
    } else {
        messageTemplate = isCQMode
            ? AppSettings::instance().getCQMessage(fKey)
            : AppSettings::instance().getSPMessage(fKey);
    }

    // Format key name for logging and status
    QString keyName = getKeyName(fKey, ctrlPressed, altPressed);
    QString modeStr = isCQMode ? "CQ" : "S&P";

    if (messageTemplate.isEmpty()) {
        result.success = false;
        result.statusMessage = QString("%1: No message defined").arg(keyName);
        LOG_INFO("CWService", QString("%1 (%2 mode): No message defined").arg(keyName).arg(modeStr));
        return result;
    }

    LOG_INFO("CWService", QString("%1 (%2 mode): Sending template: %3").arg(keyName).arg(modeStr).arg(messageTemplate));

    return sendCWMessage(messageTemplate, input);
}

CWService::Result CWService::sendCWMessage(const QString& messageTemplate, const Input& input)
{
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

    // Get sent exchange from contest (for exchange substitution)
    if (m_contest) {
        ctx.contestName = m_contest->getContestName();
        QString rst = RSTValidator::getDefault(ctx.mode);
        ctx.sentExchange = m_contest->formatSentExchange(ctx.qsoNumber, rst);
    }

    // Substitute template variables
    QString cwText = CWTemplateEngine::substitute(messageTemplate, ctx);

    // Send via active output
    int wpm = AppSettings::instance().getMorseWPM();
    sendCWText(cwText, wpm);

    // Save for repeat (= key) - per radio
    m_lastCWMessage[m_activeRadioIndex] = cwText;

    result.success = true;
    result.cwTextSent = cwText;
    result.statusMessage = QString("Sending CW: %1").arg(cwText);

    LOG_INFO("CWService", QString("Sent CW: %1 (from template: %2)")
             .arg(cwText).arg(messageTemplate));

    return result;
}

QString CWService::getLastCWMessage() const
{
    return m_lastCWMessage[m_activeRadioIndex];
}

CWService::Result CWService::repeatLastCWMessage(const Input& input)
{
    Result result;

    const QString& lastMsg = m_lastCWMessage[m_activeRadioIndex];
    if (lastMsg.isEmpty()) {
        result.success = false;
        result.statusMessage = QString("No CW message to repeat for Radio %1").arg(m_activeRadioIndex + 1);
        LOG_INFO("CWService", QString("= key pressed but no previous CW message for Radio %1").arg(m_activeRadioIndex + 1));
        return result;
    }

    if (!validatePreconditions(input, result.errorMessage)) {
        result.success = false;
        result.statusMessage = result.errorMessage;
        return result;
    }

    int wpm = AppSettings::instance().getMorseWPM();
    sendCWText(lastMsg, wpm);

    result.success = true;
    result.cwTextSent = lastMsg;
    result.statusMessage = QString("Repeating CW: %1").arg(lastMsg);

    LOG_INFO("CWService", QString("Repeated CW (Radio %1): %2").arg(m_activeRadioIndex + 1).arg(lastMsg));

    return result;
}

// --- Auto-send workflows ---

CWService::Result CWService::autoSendQSLMessage(const Input& input, bool autoSendEnabled)
{
    Result result;

    if (!isCWMode(input.radioState.modeA) || !autoSendEnabled || !m_config.radio) {
        result.success = false;
        return result;
    }

    QString qslMessage = AppSettings::instance().getQSLCWMessage();
    if (qslMessage.isEmpty()) {
        result.success = false;
        return result;
    }

    result = sendCWMessage(qslMessage, input);
    if (result.success) {
        LOG_DEBUG("CWService", QString("Auto-sent QSL message: %1").arg(qslMessage));
    }
    return result;
}

CWService::Result CWService::autoSendExchange(const Input& input, bool autoSendEnabled)
{
    Result result;

    if (!isCWMode(input.radioState.modeA) || !autoSendEnabled || !m_config.radio) {
        result.success = false;
        return result;
    }

    QString messageTemplate;
    if (input.operatingMode == OperatingMode::SP) {
        messageTemplate = AppSettings::instance().getSPCWExchange();
        LOG_DEBUG("CWService", QString("Auto-sent S&P exchange: %1").arg(messageTemplate));
    } else {
        messageTemplate = AppSettings::instance().getCQCWExchange();
        LOG_DEBUG("CWService", QString("Auto-sent CQ exchange: %1").arg(messageTemplate));
    }

    if (messageTemplate.isEmpty()) {
        result.success = false;
        return result;
    }

    return sendCWMessage(messageTemplate, input);
}

// --- CW speed sync ---

void CWService::syncCWSpeedFromRadio(const RadioState& state)
{
    if ((state.modeA == ModeType::CW || state.modeA == ModeType::CWR) && state.cwSpeed > 0) {
        if (state.cwSpeed != m_lastSyncedCWSpeed) {
            AppSettings::instance().setMorseWPM(state.cwSpeed);
            m_lastSyncedCWSpeed = state.cwSpeed;

            // Update sender WPM if we have one
            if (m_sender) {
                m_sender->setWpm(state.cwSpeed);
            }

            LOG_DEBUG("CWService", QString("Synced CW speed from radio: %1 WPM").arg(state.cwSpeed));
            emit cwSpeedSynced(state.cwSpeed);
        }
    }
}

// --- Runtime state ---

void CWService::setContest(ContestBase* contest)
{
    m_contest = contest;
}

void CWService::setActiveRadioIndex(int index)
{
    m_activeRadioIndex = (index >= 0 && index < 2) ? index : 0;
}

// --- Private helpers ---

bool CWService::validatePreconditions(const Input& input, QString& errorMessage) const
{
    if (!m_config.radio) {
        errorMessage = "CW requires radio connection";
        LOG_WARN("CWService", "Cannot send CW: radio not connected");
        return false;
    }

    if (!isCWMode(input.radioState.modeA)) {
        errorMessage = "CW requires CW mode";
        LOG_WARN("CWService", "Cannot send CW: not in CW mode");
        return false;
    }

    return true;
}

QString CWService::getKeyName(int fKey, bool ctrlPressed, bool altPressed) const
{
    QString keyName = QString("F%1").arg(fKey);
    if (ctrlPressed) {
        keyName = "Ctrl+" + keyName;
    }
    if (altPressed) {
        keyName = "Alt+" + keyName;
    }
    return keyName;
}

void CWService::createSender()
{
    auto& settings = AppSettings::instance();

    switch (m_outputMode) {
    case OutputMode::CAT:
        // Hamlib: radio generates morse from text via KY command
        m_sender = CWSenderFactory::create(CWSenderFactory::Backend::Hamlib,
                                           m_config.radio, nullptr, this);
        break;

    case OutputMode::WinKeyer:
        // External keyer: WinKeyer hardware generates morse from text
        m_sender = CWSenderFactory::create(CWSenderFactory::Backend::KeyerDevice,
                                           nullptr, m_keyerController, this);
        break;

    case OutputMode::DtrRts: {
        // DTR/RTS: software morse encoder toggles serial line on a separate port
        DtrRtsCWSender::Config dtrConfig;
        dtrConfig.portName = settings.getDtrRtsPortName();
        dtrConfig.pin = static_cast<DtrRtsCWSender::Pin>(settings.getDtrRtsPin());
        m_sender = CWSenderFactory::createDtrRts(dtrConfig, this);
        break;
    }
    }

    if (m_sender) {
        m_sender->setWpm(settings.getMorseWPM());
        LOG_DEBUG("CWService", QString("Created CW sender: %1").arg(m_sender->backendName()));
    } else {
        LOG_WARN("CWService", "Failed to create CW sender");
    }
}

void CWService::destroySender()
{
    if (m_sender) {
        m_sender->stop();
        delete m_sender;
        m_sender = nullptr;
    }
}

void CWService::wireKeyerToOutput()
{
    // Disconnect any previous keyer output connections
    // (paddle events → key output)
    QObject::disconnect(m_iambicKeyer, &IambicKeyer::keyDown, nullptr, nullptr);
    QObject::disconnect(m_iambicKeyer, &IambicKeyer::keyUp, nullptr, nullptr);

    switch (m_outputMode) {
    case OutputMode::CAT:
        // IambicKeyer → RadioController PTT toggle (existing behavior)
        if (m_config.radio) {
            connect(m_iambicKeyer, &IambicKeyer::keyDown,
                    m_config.radio, &RadioController::sendKeyDown);
            connect(m_iambicKeyer, &IambicKeyer::keyUp,
                    m_config.radio, &RadioController::sendKeyUp);
        }
        break;

    case OutputMode::WinKeyer:
        // WinKeyer handles paddle directly in hardware - no software wiring needed.
        // IambicKeyer is only used for HaliKey MIDI paddle → we still route to radio.
        if (m_config.radio) {
            connect(m_iambicKeyer, &IambicKeyer::keyDown,
                    m_config.radio, &RadioController::sendKeyDown);
            connect(m_iambicKeyer, &IambicKeyer::keyUp,
                    m_config.radio, &RadioController::sendKeyUp);
        }
        break;

    case OutputMode::DtrRts: {
        // IambicKeyer → DtrRtsCWSender DTR/RTS toggle
        auto* dtrSender = qobject_cast<DtrRtsCWSender*>(m_sender);
        if (dtrSender) {
            connect(m_iambicKeyer, &IambicKeyer::keyDown,
                    dtrSender, &DtrRtsCWSender::keyDown);
            connect(m_iambicKeyer, &IambicKeyer::keyUp,
                    dtrSender, &DtrRtsCWSender::keyUp);
        }
        break;
    }
    }

    LOG_DEBUG("CWService", QString("Keyer wired to output mode %1").arg(static_cast<int>(m_outputMode)));
}

void CWService::sendCWText(const QString& text, int wpm)
{
    if (m_sender) {
        // Use the CWSender abstraction
        m_sender->setWpm(wpm);
        m_sender->send(text);
    } else {
        // Fallback: direct radio control (backward compatibility)
        m_config.radio->setCWSpeed(wpm);
        m_config.radio->sendCW(text);
    }
}

} // namespace TR4QT

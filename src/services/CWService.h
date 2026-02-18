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

#ifndef CWSERVICE_H
#define CWSERVICE_H

#include <QObject>
#include <QString>
#include "../core/Types.h"
#include "../cw/CWOutputProfile.h"
#include "../radio/RadioController.h"

namespace TR4QT {

class ContestBase;
class KeyerController;
class IambicKeyer;
class CWSender;

/**
 * CWService - Consolidates all CW/keyer functionality
 *
 * Owns keyer hardware (KeyerController + IambicKeyer), absorbs CWMessageManager
 * logic (template substitution, function key handling, repeat), and handles
 * CW speed control and auto-send workflows.
 *
 * Follows the AmplifierService pattern: QObject, receives RadioController* (not owned),
 * emits signals for UI updates.
 *
 * Responsibilities:
 * - Keyer lifecycle (KeyerController + IambicKeyer creation, wiring, destruction)
 * - CW message sending via function keys (F1-F12, Ctrl+, Alt+)
 * - CW template substitution ({MYCALL}, {HISCALL}, {RST}, {NR}, etc.)
 * - CW speed sync from radio state
 * - Auto-send QSL and exchange messages
 * - Last-message tracking for = key repeat (per radio)
 *
 * Does NOT handle:
 * - UI dialogs (SendMorseDialog, CWMessageEditorDialog, KeyerSetupDialog)
 *   — caller passes parent widget to those dialogs
 * - Status bar updates — emits statusMessage() signal
 * - AppSettings persistence — reads from AppSettings, doesn't modify
 */
class CWService : public QObject {
    Q_OBJECT

public:
    /**
     * Configuration for CWService
     * RadioController is borrowed (not owned)
     */
    struct Config {
        RadioController* radio = nullptr;
    };

    /**
     * Input parameters for sending a CW message
     * Caller builds this from UI widget state
     */
    struct Input {
        QString callsign;            // Current callsign in entry field
        int qsoNumber = 0;          // Current serial number
        RadioState radioState;       // Current radio state (mode, band, etc.)
        OperatingMode operatingMode; // CQ vs S&P mode
    };

    /**
     * Result from CW message operations
     */
    struct Result {
        bool success = false;
        QString statusMessage;       // For UI status bar
        QString cwTextSent;          // Actual CW text sent (after substitution)
        QString errorMessage;        // Error if success = false
    };

    explicit CWService(const Config& config, QObject* parent = nullptr);
    ~CWService() override;

    // --- CW Messaging (absorbed from CWMessageManager) ---

    /**
     * Handle function key press (F1-F12, Ctrl+F1-F12, Alt+F1-F12)
     */
    Result sendFunctionKey(int fKey, bool ctrlPressed, bool altPressed, const Input& input);

    /**
     * Send CW message with template substitution
     */
    Result sendCWMessage(const QString& messageTemplate, const Input& input);

    /**
     * Get last CW message sent (for = key repeat, per active radio)
     */
    QString getLastCWMessage() const;

    /**
     * Send the last CW message again (= key repeat)
     */
    Result repeatLastCWMessage(const Input& input);

    // --- Auto-send workflows (extracted from MainWindow) ---

    /**
     * Auto-send QSL message after logging a QSO
     * @param input CW input context
     * @param autoSendEnabled Whether auto-send is enabled (menu checkbox)
     * @return Result (success=false if not in CW mode or auto-send disabled)
     */
    Result autoSendQSLMessage(const Input& input, bool autoSendEnabled);

    /**
     * Auto-send exchange after entering callsign
     * @param input CW input context
     * @param autoSendEnabled Whether auto-send is enabled (menu checkbox)
     * @return Result (success=false if not in CW mode or auto-send disabled)
     */
    Result autoSendExchange(const Input& input, bool autoSendEnabled);

    // --- CW speed adjustment ---

    /**
     * Adjust WPM by delta (positive = increase, negative = decrease)
     * Reads current speed from radio state, clamps to radio limits,
     * sends new speed to radio. Emits statusMessage with result.
     * @param radioState Current radio state (for current CW speed)
     * @return true if adjustment was made, false if preconditions failed
     */
    bool adjustWPM(const RadioState& radioState, int delta);

    // --- CW speed sync (extracted from MainWindow::onRadioStateUpdated) ---

    /**
     * Sync CW speed from radio state to AppSettings
     * Call from onRadioStateUpdated
     */
    void syncCWSpeedFromRadio(const RadioState& state);

    // --- CW Output Mode ---

    /**
     * CW output modes matching AppSettings CW/outputMode values.
     * 0=CAT (Hamlib KY command), 1=WinKeyer, 2=DTR/RTS
     */
    enum class OutputMode { CAT = 0, WinKeyer = 1, DtrRts = 2 };

    /**
     * Activate a CW output profile. Creates/destroys CWSender as needed.
     * This is the primary method for switching CW output hardware.
     * Called when station profile changes or user switches active radio.
     */
    void activateCWProfile(const CWOutputProfile& profile);

    /**
     * Configure paddle input device (global, not per-radio).
     * Connects/disconnects HaliKey via KeyerController based on config.
     * Called at startup and when user changes paddle settings.
     */
    void configurePaddleInput(const PaddleInputConfig& config);

    /**
     * Switch CW output mode (legacy). Creates/destroys CWSender as needed.
     * Prefer activateCWProfile() for new code.
     */
    void setCWOutputMode(OutputMode mode);
    OutputMode cwOutputMode() const { return m_outputMode; }

    /**
     * Get the active CWSender (for SendMorseDialog or other direct users).
     * May be nullptr if no sender is configured.
     */
    CWSender* sender() const { return m_sender; }

    // --- Keyer accessors (for KeyerSetupDialog) ---

    KeyerController* keyerController() const { return m_keyerController; }
    IambicKeyer* iambicKeyer() const { return m_iambicKeyer; }

    // --- Runtime state ---

    void setContest(ContestBase* contest);
    void setActiveRadioIndex(int index);

signals:
    /**
     * Emitted for status bar messages
     */
    void statusMessage(const QString& message);

    /**
     * Emitted when CW speed is synced from radio
     */
    void cwSpeedSynced(int wpm);

private:
    /**
     * Validate preconditions for sending CW (radio connected, CW mode)
     */
    bool validatePreconditions(const Input& input, QString& errorMessage) const;

    /**
     * Format key name for logging (e.g., "F1", "Ctrl+F5", "Alt+F12")
     */
    QString getKeyName(int fKey, bool ctrlPressed, bool altPressed) const;

    /**
     * Map CWSenderFactory::Backend to OutputMode for backward compat.
     */
    static OutputMode backendToOutputMode(CWSenderFactory::Backend backend);

    /**
     * Create the appropriate CWSender based on active profile or output mode.
     */
    void createSender();

    /**
     * Create CWSender from a CWOutputProfile.
     */
    void createSenderFromProfile(const CWOutputProfile& profile);

    /**
     * Destroy current sender and disconnect keyer wiring.
     */
    void destroySender();

    /**
     * Wire IambicKeyer signals to the appropriate output based on mode.
     */
    void wireKeyerToOutput();

    /**
     * Send CW text through the active sender (or fall back to RadioController).
     */
    void sendCWText(const QString& text, int wpm);

    Config m_config;
    ContestBase* m_contest = nullptr;  // Borrowed, not owned
    QString m_lastCWMessage[2];        // Per radio (for = key repeat)
    int m_activeRadioIndex = 0;
    int m_lastSyncedCWSpeed = 0;       // Track last synced speed to avoid redundant updates
    OutputMode m_outputMode = OutputMode::CAT;  // Default: CAT (Hamlib KY command)
    CWOutputProfile m_activeProfile;           // Active CW output profile (if set via activateCWProfile)

    // Owned keyer hardware
    KeyerController* m_keyerController = nullptr;
    IambicKeyer* m_iambicKeyer = nullptr;

    // Owned CW sender (created based on output mode)
    CWSender* m_sender = nullptr;
};

} // namespace TR4QT

#endif // CWSERVICE_H

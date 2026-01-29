#ifndef CWMESSAGEMANAGER_H
#define CWMESSAGEMANAGER_H

#include <QString>
#include <QWidget>
#include "../core/Types.h"
#include "../radio/RadioController.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

/**
 * CWMessageManager - Handles CW message sending and template substitution
 *
 * Extracted from MainWindow to isolate CW messaging logic.
 * Responsible for:
 * - Handling function key presses (F1-F12, Ctrl+F1-F12, Alt+F1-F12)
 * - Sending CW messages via RadioController
 * - Template variable substitution ({MYCALL}, {HISCALL}, {RST}, {NR}, etc.)
 * - Tracking last message sent (for = key repeat)
 * - Validating preconditions (radio connected, CW mode)
 *
 * Does NOT handle:
 * - UI dialogs (CWMessageEditorDialog, SendMorseDialog) - caller's responsibility
 * - Status bar updates - returns status messages for caller to display
 * - AppSettings persistence - reads from AppSettings, doesn't modify
 */
class CWMessageManager {
public:
    // Configuration passed to constructor
    struct Config {
        RadioController* radio = nullptr;  // Radio for sending CW
        ContestBase* contest = nullptr;    // Active contest (can be nullptr)
    };

    // Input parameters for sending a CW message
    struct Input {
        QString callsign;            // Current callsign in entry field
        int qsoNumber;               // Current serial number
        RadioState radioState;       // Current radio state (for mode validation, exchange context)
        OperatingMode operatingMode; // CQ vs S&P mode
    };

    // Result returned from sendFunctionKey() and sendCWMessage()
    struct Result {
        bool success = false;        // true if CW sent successfully
        QString statusMessage;       // Status message for UI (always populated)
        QString cwTextSent;          // Actual CW text sent (after substitution)
        QString errorMessage;        // Error message if success = false
    };

    /**
     * Constructor
     * @param config Configuration with radio and contest pointers
     */
    explicit CWMessageManager(const Config& config);

    /**
     * Handle function key press (F1-F12, Ctrl+F1-F12, Alt+F1-F12)
     *
     * @param fKey Function key number (1-12)
     * @param ctrlPressed true if Ctrl modifier pressed
     * @param altPressed true if Alt modifier pressed
     * @param input Input parameters (callsign, QSO number, radio state, mode)
     * @return Result with success status, CW text sent, status message, or error
     */
    Result sendFunctionKey(int fKey, bool ctrlPressed, bool altPressed, const Input& input);

    /**
     * Send CW message with template substitution
     *
     * @param messageTemplate Template string with TR4W special characters
     * @param input Input parameters for template substitution
     * @return Result with success status, CW text sent, status message, or error
     */
    Result sendCWMessage(const QString& messageTemplate, const Input& input);

    /**
     * Get last CW message sent (for = key repeat)
     * @return Last CW text sent (empty if none)
     */
    QString getLastCWMessage() const;

    /**
     * Send the last CW message again (= key repeat)
     *
     * @param input Input parameters (only radio state used for validation)
     * @return Result with success status, CW text sent, status message, or error
     */
    Result repeatLastCWMessage(const Input& input);

private:
    /**
     * Validate preconditions for sending CW
     * @param input Input parameters (radio state for mode check)
     * @param errorMessage Output error message if validation fails
     * @return true if all preconditions met, false otherwise
     */
    bool validatePreconditions(const Input& input, QString& errorMessage) const;

    /**
     * Get formatted key name for logging (e.g., "F1", "Ctrl+F5", "Alt+F12")
     * @param fKey Function key number (1-12)
     * @param ctrlPressed true if Ctrl modifier pressed
     * @param altPressed true if Alt modifier pressed
     * @return Formatted key name
     */
    QString getKeyName(int fKey, bool ctrlPressed, bool altPressed) const;

    Config m_config;
    QString m_lastCWMessage[2];  // Track last message sent per radio (for = key repeat)
    int m_activeRadioIndex{0};   // Which radio is active (0 or 1)

public:
    /**
     * Set active radio index for per-radio message tracking
     * @param index Radio index (0 or 1)
     */
    void setActiveRadioIndex(int index) { m_activeRadioIndex = (index >= 0 && index < 2) ? index : 0; }
};

} // namespace TR4QT

#endif // CWMESSAGEMANAGER_H

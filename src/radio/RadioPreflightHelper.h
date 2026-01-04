#ifndef RADIOPREFLIGHTHELPER_H
#define RADIOPREFLIGHTHELPER_H

#include <QString>
#include <hamlib/rig.h>

namespace TR4QT {

/**
 * @brief Helper class for radio pre-flight connectivity and verification checks
 *
 * Provides two levels of pre-flight testing:
 * 1. General network connectivity (TCP socket connection test)
 * 2. Radio-specific verification (send commands and verify responses)
 *
 * This keeps radio verification logic out of the RadioController "god class"
 * and makes it easy to add verification for new radio models.
 */
class RadioPreflightHelper
{
public:
    /**
     * @brief General pre-flight check: verify radio is reachable via TCP
     * @param host Radio hostname or IP address
     * @param port Radio port number
     * @param timeoutMs Timeout in milliseconds (default 2000ms)
     * @return true if radio is reachable, false otherwise
     *
     * Tests basic TCP connectivity without radio-specific commands.
     * Useful for all network-connected radios.
     */
    static bool generalPreflight(const QString& host, quint16 port, int timeoutMs = 2000);

    /**
     * @brief Radio-specific pre-flight check: verify radio model responds correctly
     * @param radioModel Hamlib radio model ID (e.g., RIG_MODEL_K4)
     * @param host Radio hostname or IP address
     * @param port Radio port number
     * @param timeoutMs Timeout in milliseconds (default 2000ms)
     * @return true if radio verification succeeded, false otherwise
     *
     * Performs radio-specific verification by sending commands and checking responses.
     * Falls back to general pre-flight if no specific verification is implemented
     * for the given radio model.
     *
     * Currently supported:
     * - RIG_MODEL_K4: Sends "ID;" and expects "ID017;"
     */
    static bool radioSpecificPreflight(rig_model_t radioModel, const QString& host, quint16 port, int timeoutMs = 2000);

private:
    /**
     * @brief Verify Elecraft K4 by sending ID command
     * @param host K4 hostname or IP address
     * @param port K4 port number (typically 9200)
     * @param timeoutMs Timeout in milliseconds
     * @return true if K4 responds with "ID017;", false otherwise
     *
     * Sends "ID;" command and verifies response is "ID017;" (K4's ID).
     */
    static bool verifyK4(const QString& host, quint16 port, int timeoutMs);
};

} // namespace TR4QT

#endif // RADIOPREFLIGHTHELPER_H

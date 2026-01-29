/**
 * IRadioCommandHandler - Interface for handling radio commands
 *
 * This interface abstracts radio command handling, allowing different implementations:
 * - MainWindow: GUI mode with full RadioManager
 * - Headless: Returns "not available" or does nothing
 *
 * Used by WebServerContext to handle radio commands without depending on QtWidgets.
 */

#ifndef IRADIOCOMMANDHANDLER_H
#define IRADIOCOMMANDHANDLER_H

#include "../core/Types.h"
#include "../radio/RadioInterface.h"  // For RadioState

namespace TR4QT {

/**
 * Interface for handling radio commands from web API
 */
class IRadioCommandHandler {
public:
    virtual ~IRadioCommandHandler() = default;

    /**
     * Check if radio control is available
     * @return true if radio commands can be executed
     */
    virtual bool isRadioAvailable() const = 0;

    /**
     * Get current radio state
     * @return Current radio state (or default if not available)
     */
    virtual RadioState getCurrentRadioState() const = 0;

    /**
     * Set radio frequency
     * @param frequency Frequency in Hz
     * @return true if command executed successfully
     */
    virtual bool setFrequency(freq_t frequency) = 0;

    /**
     * Set radio band
     * @param band Band to set
     * @return true if command executed successfully
     */
    virtual bool setBand(BandType band) = 0;

    /**
     * Set radio mode
     * @param mode Mode to set
     * @return true if command executed successfully
     */
    virtual bool setMode(ModeType mode) = 0;
};

} // namespace TR4QT

#endif // IRADIOCOMMANDHANDLER_H

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

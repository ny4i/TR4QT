#ifndef CWSENDERFACTORY_H
#define CWSENDERFACTORY_H

#include <memory>
#include <QString>

namespace TR4QT {

class CWSender;
class RadioController;

/**
 * Factory for creating CW sender instances
 *
 * Supports different backends:
 * - "hamlib": Uses Hamlib rig_send_morse (default)
 * - "winkeyer": Uses Winkeyer hardware (future)
 * - "simulated": For testing without hardware (future)
 */
class CWSenderFactory {
public:
    enum class Backend {
        Hamlib,     // Default: Use Hamlib via RadioController
        Winkeyer,   // Future: Use Winkeyer serial device
        Simulated   // Future: For testing
    };

    /**
     * Create a CW sender with the specified backend
     *
     * @param backend The backend to use
     * @param radio The radio controller (required for Hamlib backend)
     * @param parent QObject parent for memory management
     * @return Pointer to the created CW sender, or nullptr on failure
     */
    static CWSender* create(Backend backend,
                           RadioController* radio = nullptr,
                           QObject* parent = nullptr);

    /**
     * Create a CW sender by backend name
     *
     * @param backendName "hamlib", "winkeyer", or "simulated"
     * @param radio The radio controller (required for Hamlib backend)
     * @param parent QObject parent for memory management
     * @return Pointer to the created CW sender, or nullptr on failure
     */
    static CWSender* create(const QString& backendName,
                           RadioController* radio = nullptr,
                           QObject* parent = nullptr);

    /**
     * Get the default backend
     */
    static Backend defaultBackend() { return Backend::Hamlib; }

    /**
     * Convert backend enum to string
     */
    static QString backendToString(Backend backend);

    /**
     * Convert string to backend enum
     */
    static Backend stringToBackend(const QString& name);
};

} // namespace TR4QT

#endif // CWSENDERFACTORY_H

#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QString>

namespace TR4QT {

/**
 * CredentialStore - Secure password storage using OS-native credential stores
 *
 * Uses qtkeychain to store passwords in:
 * - macOS: Keychain
 * - Windows: Credential Manager
 * - Linux: Secret Service (libsecret)
 *
 * All methods are synchronous (QEventLoop). Only call from the main thread
 * during settings save/load operations.
 *
 * Graceful fallback: if the credential store is unavailable (locked keychain,
 * no Secret Service, permission denied), callers should fall back to QSettings.
 */
class CredentialStore {
public:
    static CredentialStore& instance();

    /**
     * Save a password to the secure store.
     * @param storageKey Logical key (e.g., "IcomRadio" or "IcomRadio/Default")
     * @param user Username (use empty string for password-only auth)
     * @param password Password to store
     * @return 0 on success, non-zero on failure
     */
    int savePassword(const QString& storageKey, const QString& user, const QString& password);

    /**
     * Retrieve a password from the secure store.
     * @param storageKey Logical key
     * @param user Username (use empty string for password-only auth)
     * @return The password, or empty string if not found or on error
     */
    QString getPassword(const QString& storageKey, const QString& user) const;

    /**
     * Delete a password from the secure store.
     * @param storageKey Logical key
     * @param user Username (use empty string for password-only auth)
     */
    void deletePassword(const QString& storageKey, const QString& user);

private:
    CredentialStore() = default;
    ~CredentialStore() = default;
    CredentialStore(const CredentialStore&) = delete;
    CredentialStore& operator=(const CredentialStore&) = delete;

    /**
     * Build the effective username for keychain storage.
     * Uses "__default__" sentinel for empty usernames (Icom password-only auth).
     * On Windows, prepends the storage key to work around qtkeychain issue #105.
     */
    static QString effectiveUser(const QString& storageKey, const QString& user);

    /// Prefix applied to all storage keys
    static constexpr const char* KEY_PREFIX = "TR4QT:";

    /// Sentinel value for empty usernames
    static constexpr const char* DEFAULT_USER_SENTINEL = "__default__";

    /// Timeout for keychain operations (ms)
    static constexpr int KEYCHAIN_TIMEOUT_MS = 10000;
};

/// Well-known storage key constants for credential storage.
/// Use these instead of hardcoded strings to prevent typos and ensure consistency.
namespace CredentialKeys {
    /// Legacy single-radio configuration key
    constexpr const char* ICOM_RADIO = "IcomRadio";

    /// Build a profile-specific storage key: "IcomRadio/{profileName}"
    inline QString icomRadioProfile(const QString& profileName) {
        return QString("IcomRadio/%1").arg(profileName);
    }
}

} // namespace TR4QT

#endif // CREDENTIALSTORE_H

#include "CredentialStore.h"
#include "../logging/LogMacros.h"

#include <qtkeychain/keychain.h>
#include <QEventLoop>
#include <QTimer>

namespace TR4QT {

CredentialStore& CredentialStore::instance() {
    static CredentialStore inst;
    return inst;
}

QString CredentialStore::effectiveUser(const QString& storageKey, const QString& user) {
    QString effUser = user.isEmpty() ? QString(DEFAULT_USER_SENTINEL) : user;
#ifdef Q_OS_WIN
    // Windows Credential Manager uses "user@service" internally.
    // qtkeychain issue #105 (https://github.com/frankosterfeld/qtkeychain/issues/105):
    // different storage keys with the same username collide because Credential Manager
    // deduplicates on user+service. Prepend the storage key to disambiguate.
    effUser = storageKey + QLatin1Char('/') + effUser;
#else
    Q_UNUSED(storageKey);
#endif
    return effUser;
}

int CredentialStore::savePassword(const QString& storageKey, const QString& user, const QString& password) {
    const QString fullKey = QString(KEY_PREFIX) + storageKey;
    const QString effUser = effectiveUser(storageKey, user);

    QKeychain::WritePasswordJob job(fullKey);
    job.setAutoDelete(false);
    job.setKey(effUser);
    job.setTextData(password);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&job, &QKeychain::WritePasswordJob::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    job.start();
    timeout.start(KEYCHAIN_TIMEOUT_MS);
    loop.exec();

    if (!timeout.isActive()) {
        LOG_WARN("CredentialStore",
                 QString("Timeout saving password for '%1' user='%2'").arg(storageKey, effUser));
        return -1;
    }
    timeout.stop();

    if (job.error() != QKeychain::NoError) {
        LOG_WARN("CredentialStore",
                 QString("Failed to save password for '%1' user='%2': %3")
                     .arg(storageKey, effUser, job.errorString()));
        return static_cast<int>(job.error());
    }

    LOG_DEBUG("CredentialStore",
              QString("Password saved for '%1' user='%2'").arg(storageKey, effUser));
    return 0;
}

QString CredentialStore::getPassword(const QString& storageKey, const QString& user) const {
    const QString fullKey = QString(KEY_PREFIX) + storageKey;
    const QString effUser = effectiveUser(storageKey, user);

    QKeychain::ReadPasswordJob job(fullKey);
    job.setAutoDelete(false);
    job.setKey(effUser);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&job, &QKeychain::ReadPasswordJob::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    job.start();
    timeout.start(KEYCHAIN_TIMEOUT_MS);
    loop.exec();

    if (!timeout.isActive()) {
        LOG_WARN("CredentialStore",
                 QString("Timeout reading password for '%1' user='%2'").arg(storageKey, effUser));
        return QString();
    }
    timeout.stop();

    if (job.error() != QKeychain::NoError) {
        // EntryNotFound is expected for first-run / unmigrated credentials — not a warning
        if (job.error() != QKeychain::EntryNotFound) {
            LOG_WARN("CredentialStore",
                     QString("Failed to read password for '%1' user='%2': %3")
                         .arg(storageKey, effUser, job.errorString()));
        }
        return QString();
    }

    LOG_DEBUG("CredentialStore",
              QString("Password read for '%1' user='%2'").arg(storageKey, effUser));
    return job.textData();
}

void CredentialStore::deletePassword(const QString& storageKey, const QString& user) {
    const QString fullKey = QString(KEY_PREFIX) + storageKey;
    const QString effUser = effectiveUser(storageKey, user);

    QKeychain::DeletePasswordJob job(fullKey);
    job.setAutoDelete(false);
    job.setKey(effUser);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&job, &QKeychain::DeletePasswordJob::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    job.start();
    timeout.start(KEYCHAIN_TIMEOUT_MS);
    loop.exec();

    if (!timeout.isActive()) {
        LOG_WARN("CredentialStore",
                 QString("Timeout deleting password for '%1' user='%2'").arg(storageKey, effUser));
        return;
    }
    timeout.stop();

    if (job.error() != QKeychain::NoError && job.error() != QKeychain::EntryNotFound) {
        LOG_WARN("CredentialStore",
                 QString("Failed to delete password for '%1' user='%2': %3")
                     .arg(storageKey, effUser, job.errorString()));
    } else {
        LOG_DEBUG("CredentialStore",
                  QString("Password deleted for '%1' user='%2'").arg(storageKey, effUser));
    }
}

} // namespace TR4QT

#include <QTest>
#include <QCoreApplication>
#include "../src/utils/CredentialStore.h"

using namespace TR4QT;

/**
 * Integration tests for CredentialStore
 *
 * These tests exercise the real OS credential store (Windows Credential Manager,
 * macOS Keychain, or Linux Secret Service). They use a unique test-only storage
 * key to avoid colliding with production credentials.
 *
 * Key scenario: Windows Credential Manager is case-insensitive on target names,
 * but qtkeychain treats them as case-sensitive. A username case change (ny4i → NY4I)
 * can create a shadow entry that returns stale data. The delete-then-recreate
 * pattern in saveRadioProfiles() handles this; these tests verify the underlying
 * CredentialStore behavior that motivated that fix.
 */
class TestCredentialStore : public QObject {
    Q_OBJECT

private:
    static constexpr const char* TEST_KEY = "TR4QT_Test_CredStore";

private slots:
    void cleanup();

    // Basic round-trip
    void testSaveAndRetrieve();

    // Username case change: delete old, save new, verify new password is returned
    void testUsernameCaseChange_DeleteThenRecreate();

    // Overwrite same username (no case change) works
    void testPasswordUpdate_SameUsername();

    // Delete nonexistent credential doesn't crash
    void testDeleteNonexistent();
};

void TestCredentialStore::cleanup()
{
    // Clean up any test credentials after each test
    CredentialStore::instance().deletePassword(TEST_KEY, "testuser");
    CredentialStore::instance().deletePassword(TEST_KEY, "TESTUSER");
    CredentialStore::instance().deletePassword(TEST_KEY, "TestUser");
}

void TestCredentialStore::testSaveAndRetrieve()
{
    auto& store = CredentialStore::instance();

    int rc = store.savePassword(TEST_KEY, "testuser", "secret123");
    QCOMPARE(rc, 0);

    QString retrieved = store.getPassword(TEST_KEY, "testuser");
    QCOMPARE(retrieved, QString("secret123"));
}

void TestCredentialStore::testUsernameCaseChange_DeleteThenRecreate()
{
    // This is the exact scenario that caused the bug:
    // 1. Save password with lowercase username
    // 2. User changes username to uppercase in UI
    // 3. Delete old credential with old username
    // 4. Save new credential with new username and new password
    // 5. Read back should return the NEW password

    auto& store = CredentialStore::instance();

    // Step 1: Save with lowercase
    int rc = store.savePassword(TEST_KEY, "testuser", "old_password");
    QCOMPARE(rc, 0);
    QCOMPARE(store.getPassword(TEST_KEY, "testuser"), QString("old_password"));

    // Step 2-3: Delete old credential (the delete-then-recreate pattern)
    store.deletePassword(TEST_KEY, "testuser");

    // Step 4: Save with uppercase username and new password
    rc = store.savePassword(TEST_KEY, "TESTUSER", "new_password");
    QCOMPARE(rc, 0);

    // Step 5: Read back with new username must return new password
    QString retrieved = store.getPassword(TEST_KEY, "TESTUSER");
    QCOMPARE(retrieved, QString("new_password"));
}

void TestCredentialStore::testPasswordUpdate_SameUsername()
{
    auto& store = CredentialStore::instance();

    int rc = store.savePassword(TEST_KEY, "testuser", "password_v1");
    QCOMPARE(rc, 0);

    rc = store.savePassword(TEST_KEY, "testuser", "password_v2");
    QCOMPARE(rc, 0);

    QString retrieved = store.getPassword(TEST_KEY, "testuser");
    QCOMPARE(retrieved, QString("password_v2"));
}

void TestCredentialStore::testDeleteNonexistent()
{
    // Should not crash or throw
    CredentialStore::instance().deletePassword(TEST_KEY, "nobody");
}

QTEST_MAIN(TestCredentialStore)
#include "test_credential_store.moc"

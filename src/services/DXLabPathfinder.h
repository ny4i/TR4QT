#ifndef DXLABPATHFINDER_H
#define DXLABPATHFINDER_H

#include <QObject>
#include <QString>

#ifdef Q_OS_WIN

#include <windows.h>
#include <ddeml.h>

/**
 * @brief Impersonates DXLab's PathFinder application via DDE.
 *
 * Registers as "Pathfinder" on the DDE service bus, sends a "002start"
 * handshake to SpotCollector, and emits callsignReceived() whenever
 * SpotCollector sends a getqslinfo command (e.g., spot double-click).
 *
 * Usage:
 *   auto *pf = new DXLabPathfinder(this);
 *   connect(pf, &DXLabPathfinder::callsignReceived, this, &MyClass::onCallsign);
 *   pf->start();
 */
class DXLabPathfinder : public QObject
{
    Q_OBJECT

public:
    explicit DXLabPathfinder(QObject *parent = nullptr);
    ~DXLabPathfinder();

    bool start();
    void stop();
    bool isRunning() const { return m_running; }
    QString lastError() const { return m_lastError; }

    static QString parseCallsign(const QString &command);

signals:
    void callsignReceived(const QString &callsign);
    void connected();
    void disconnected();
    void error(const QString &message);

private:
    static HDDEDATA CALLBACK ddeCallback(UINT uType, UINT uFmt, HCONV hconv,
                                          HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                          ULONG_PTR dwData1, ULONG_PTR dwData2);

    QString hszToString(HSZ hsz) const;
    QString getExecuteData(HDDEDATA hdata) const;
    bool informSpotCollector();

    DWORD m_idInst = 0;
    HSZ m_hszService = 0;
    bool m_running = false;
    QString m_lastError;

    static DXLabPathfinder *s_instance;
};

#else
// Stub for non-Windows platforms
class DXLabPathfinder : public QObject
{
    Q_OBJECT
public:
    explicit DXLabPathfinder(QObject *parent = nullptr) : QObject(parent) {}
    ~DXLabPathfinder() {}
    bool start() { m_lastError = QStringLiteral("DDE not available on this platform"); emit error(m_lastError); return false; }
    void stop() {}
    bool isRunning() const { return false; }
    QString lastError() const { return m_lastError; }
    static QString parseCallsign(const QString &command);
signals:
    void callsignReceived(const QString &callsign);
    void connected();
    void disconnected();
    void error(const QString &message);
private:
    QString m_lastError;
};
#endif

#endif // DXLABPATHFINDER_H

#include "DXLabPathfinder.h"

#ifdef Q_OS_WIN

#include <QRegularExpression>
#include "../logging/LogMacros.h"

// Server ID for PathFinder (QSLInfoServer) in DXLab's numbering
static constexpr int QSLINFO_SERVER_ID = 2;

// Static instance pointer for routing C callback to Qt object
DXLabPathfinder *DXLabPathfinder::s_instance = nullptr;

DXLabPathfinder::DXLabPathfinder(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

DXLabPathfinder::~DXLabPathfinder()
{
    stop();
    if (s_instance == this)
        s_instance = nullptr;
}

bool DXLabPathfinder::start()
{
    if (m_running)
        return true;

    // Check if real PathFinder is already running using a client-only DDE instance
    {
        DWORD checkInst = 0;
        UINT chk = DdeInitializeA(&checkInst, nullptr, APPCMD_CLIENTONLY, 0);
        if (chk == DMLERR_NO_ERROR) {
            HSZ hszSvc = DdeCreateStringHandleA(checkInst, "Pathfinder", CP_WINANSI);
            HSZ hszTop = DdeCreateStringHandleA(checkInst, "DDEServer", CP_WINANSI);
            if (hszSvc && hszTop) {
                HCONV hTest = DdeConnect(checkInst, hszSvc, hszTop, nullptr);
                if (hTest) {
                    DdeDisconnect(hTest);
                    if (hszSvc) DdeFreeStringHandle(checkInst, hszSvc);
                    if (hszTop) DdeFreeStringHandle(checkInst, hszTop);
                    DdeUninitialize(checkInst);
                    m_lastError = QStringLiteral("PathFinder is already running. Cannot register as Pathfinder.");
                    emit error(m_lastError);
                    return false;
                }
            }
            if (hszSvc) DdeFreeStringHandle(checkInst, hszSvc);
            if (hszTop) DdeFreeStringHandle(checkInst, hszTop);
            DdeUninitialize(checkInst);
        }
    }

    // Initialize DDEML as server
    UINT result = DdeInitializeA(&m_idInst, ddeCallback, APPCLASS_STANDARD, 0);
    if (result != DMLERR_NO_ERROR) {
        m_lastError = QStringLiteral("DdeInitialize failed with code %1").arg(result);
        emit error(m_lastError);
        return false;
    }

    m_hszService = DdeCreateStringHandleA(m_idInst, "Pathfinder", CP_WINANSI);
    if (!m_hszService) {
        m_lastError = QStringLiteral("Failed to create service string handle");
        emit error(m_lastError);
        DdeUninitialize(m_idInst);
        m_idInst = 0;
        return false;
    }

    HDDEDATA regResult = DdeNameService(m_idInst, m_hszService, 0, DNS_REGISTER);
    if (!regResult) {
        m_lastError = QStringLiteral("Failed to register Pathfinder service");
        emit error(m_lastError);
        DdeFreeStringHandle(m_idInst, m_hszService);
        DdeUninitialize(m_idInst);
        m_hszService = 0;
        m_idInst = 0;
        return false;
    }

    m_running = true;
    LOG_INFO("DXLabPathfinder", "Registered as Pathfinder");

    // Tell SpotCollector we're available
    if (informSpotCollector()) {
        emit connected();
    }

    return true;
}

void DXLabPathfinder::stop()
{
    if (!m_running)
        return;

    if (m_hszService) {
        DdeNameService(m_idInst, m_hszService, 0, DNS_UNREGISTER);
        DdeFreeStringHandle(m_idInst, m_hszService);
        m_hszService = 0;
    }

    if (m_idInst) {
        DdeUninitialize(m_idInst);
        m_idInst = 0;
    }

    m_running = false;
    LOG_INFO("DXLabPathfinder", "Stopped");
    emit disconnected();
}

bool DXLabPathfinder::informSpotCollector()
{
    // Connect to SpotCollector|DDEClient and send "002start"
    HSZ hszService = DdeCreateStringHandleA(m_idInst, "SpotCollector", CP_WINANSI);
    HSZ hszTopic = DdeCreateStringHandleA(m_idInst, "DDEClient", CP_WINANSI);

    if (!hszService || !hszTopic) {
        if (hszService) DdeFreeStringHandle(m_idInst, hszService);
        if (hszTopic) DdeFreeStringHandle(m_idInst, hszTopic);
        LOG_WARN("DXLabPathfinder", "Failed to create SpotCollector string handles");
        return false;
    }

    HCONV hconv = DdeConnect(m_idInst, hszService, hszTopic, nullptr);
    if (!hconv) {
        LOG_DEBUG("DXLabPathfinder", "SpotCollector not running or DDEClient not available");
        DdeFreeStringHandle(m_idInst, hszService);
        DdeFreeStringHandle(m_idInst, hszTopic);
        return false;
    }

    // Send "002start"
    QByteArray cmd = QStringLiteral("%1start").arg(QSLINFO_SERVER_ID, 3, 10, QLatin1Char('0')).toLatin1();
    cmd.append('\0');

    HDDEDATA hData = DdeCreateDataHandle(m_idInst,
                                          reinterpret_cast<LPBYTE>(cmd.data()),
                                          cmd.size(), 0, 0, CF_TEXT, 0);

    bool success = false;
    if (hData) {
        HDDEDATA hResult = DdeClientTransaction(
            reinterpret_cast<LPBYTE>(hData), 0xFFFFFFFF,
            hconv, 0, CF_TEXT, XTYP_EXECUTE, 5000, nullptr);
        success = (hResult != 0);
    }

    DdeDisconnect(hconv);
    DdeFreeStringHandle(m_idInst, hszService);
    DdeFreeStringHandle(m_idInst, hszTopic);

    LOG_INFO("DXLabPathfinder", QString("Inform SpotCollector: %1").arg(success ? "OK" : "FAILED"));
    return success;
}

HDDEDATA CALLBACK DXLabPathfinder::ddeCallback(UINT uType, UINT uFmt, HCONV hconv,
                                                 HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                                 ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    Q_UNUSED(uFmt); Q_UNUSED(hconv); Q_UNUSED(dwData1); Q_UNUSED(dwData2);

    if (!s_instance)
        return reinterpret_cast<HDDEDATA>(DDE_FNOTPROCESSED);

    switch (uType) {
    case XTYP_CONNECT:
        return reinterpret_cast<HDDEDATA>(TRUE);

    case XTYP_CONNECT_CONFIRM:
        return reinterpret_cast<HDDEDATA>(DDE_FNOTPROCESSED);

    case XTYP_WILDCONNECT:
        return reinterpret_cast<HDDEDATA>(TRUE);

    case XTYP_EXECUTE: {
        QString command = s_instance->getExecuteData(hdata);
        QString callsign = parseCallsign(command);
        if (!callsign.isEmpty()) {
            LOG_DEBUG("DXLabPathfinder", QString("Callsign: %1").arg(callsign));
            emit s_instance->callsignReceived(callsign);
        }
        return reinterpret_cast<HDDEDATA>(DDE_FACK);
    }

    case XTYP_REQUEST:
    case XTYP_ADVREQ: {
        // Return empty data
        QByteArray empty(1, '\0');
        HDDEDATA hResult = DdeCreateDataHandle(
            s_instance->m_idInst,
            reinterpret_cast<LPBYTE>(empty.data()),
            empty.size(), 0, hsz2, uFmt, 0);
        return hResult ? hResult : reinterpret_cast<HDDEDATA>(DDE_FNOTPROCESSED);
    }

    case XTYP_ADVSTART:
        return reinterpret_cast<HDDEDATA>(DDE_FACK);

    default:
        return reinterpret_cast<HDDEDATA>(DDE_FNOTPROCESSED);
    }
}

QString DXLabPathfinder::hszToString(HSZ hsz) const
{
    if (!hsz)
        return {};
    char buf[256];
    DdeQueryStringA(m_idInst, hsz, buf, sizeof(buf), CP_WINANSI);
    return QString::fromLatin1(buf);
}

QString DXLabPathfinder::getExecuteData(HDDEDATA hdata) const
{
    if (!hdata)
        return {};

    DWORD size = DdeGetData(hdata, nullptr, 0, 0);
    if (size == 0)
        return {};

    QByteArray buf(static_cast<int>(size), '\0');
    DdeGetData(hdata, reinterpret_cast<LPBYTE>(buf.data()), size, 0);

    // Remove null terminator if present
    if (buf.endsWith('\0'))
        buf.chop(1);

    return QString::fromLatin1(buf);
}

#endif // Q_OS_WIN

// parseCallsign is platform-independent (pure string parsing)
#include <QRegularExpression>

QString DXLabPathfinder::parseCallsign(const QString &command)
{
    // Command format: "002getqslinfo<callsign:4>AK7G"
    // Extract the callsign field value using the length specifier
    static QRegularExpression re(QStringLiteral("<callsign:(\\d+)>(.+)"),
                                  QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(command);
    if (match.hasMatch()) {
        int len = match.captured(1).toInt();
        return match.captured(2).left(len).trimmed();
    }
    return {};
}

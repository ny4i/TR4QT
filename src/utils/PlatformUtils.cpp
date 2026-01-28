#include "PlatformUtils.h"
#include <QHostInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_MACOS
#include <SystemConfiguration/SystemConfiguration.h>
#endif

namespace TR4QT {
namespace PlatformUtils {

QString getNetBiosName()
{
#ifdef Q_OS_WIN
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(buffer, &size)) {
        return QString::fromWCharArray(buffer);
    }
#endif

#ifdef Q_OS_MACOS
    // Query the SMB NetBIOS name from SystemConfiguration
    SCDynamicStoreRef store = SCDynamicStoreCreate(nullptr, CFSTR("TR4QT"), nullptr, nullptr);
    if (store) {
        CFDictionaryRef smbConfig = static_cast<CFDictionaryRef>(
            SCDynamicStoreCopyValue(store, CFSTR("com.apple.smb")));
        if (smbConfig) {
            CFStringRef netBiosName = static_cast<CFStringRef>(
                CFDictionaryGetValue(smbConfig, CFSTR("NetBIOSName")));
            if (netBiosName) {
                char buf[256];
                if (CFStringGetCString(netBiosName, buf, sizeof(buf), kCFStringEncodingUTF8)) {
                    CFRelease(smbConfig);
                    CFRelease(store);
                    return QString::fromUtf8(buf);
                }
            }
            CFRelease(smbConfig);
        }
        CFRelease(store);
    }
#endif

    // Fallback for all platforms: hostname without domain suffix
    QString hostname = QHostInfo::localHostName();
    int dotIndex = hostname.indexOf('.');
    if (dotIndex > 0) {
        hostname = hostname.left(dotIndex);
    }
    return hostname;
}

}  // namespace PlatformUtils
}  // namespace TR4QT

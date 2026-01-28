#pragma once

#include <QString>

namespace TR4QT {
namespace PlatformUtils {

// Returns the computer's NetBIOS name (or equivalent).
// Windows: Win32 GetComputerName API (true NetBIOS name)
// macOS: SCDynamicStore SMB/NetBIOSName, falls back to local hostname
// Linux: hostname without domain suffix
QString getNetBiosName();

}  // namespace PlatformUtils
}  // namespace TR4QT

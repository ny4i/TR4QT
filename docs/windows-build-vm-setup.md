# Windows Build VM Setup

For local Windows builds/testing when GitHub CI isn't enough.

## VM Requirements

- **OS**: Windows 11 Pro (for RDP) or Home (SSH only)
- **CPU**: 4+ vCPUs
- **RAM**: 8GB minimum (16GB better for parallel builds)
- **Disk**: 60GB+ (Windows ~25GB, Qt ~10GB, MinGW ~5GB, projects)

## Installation Steps

### 1. Create Windows VM

- Use Windows 11 ISO
- Enable VirtIO drivers for better performance
- Install guest tools

### 2. Enable OpenSSH Server

```powershell
# Run as Administrator
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
Start-Service sshd
Set-Service -Name sshd -StartupType 'Automatic'
```

### 3. Install Build Tools

**Qt 6.10.1:**
```
# Download from: https://www.qt.io/download-qt-installer
# Install to: C:\Qt
# Select: Qt 6.10.1 > MinGW 13.x 64-bit
# Select: Qt 6.10.1 > Additional Libraries > WebSockets, HttpServer, SerialPort, ShaderTools
```

**MinGW 13.x** (comes with Qt installer, just ensure it's selected)

**CMake:**
```
winget install Kitware.CMake
```

**Hamlib:**
```
# Download from: https://github.com/Hamlib/Hamlib/releases
# Extract hamlib-w64-4.6.5.zip to C:\hamlib
```

### 4. Set Environment Variables

Add to System PATH:
```
C:\Qt\6.10.1\mingw_64\bin
C:\Qt\Tools\mingw1310_64\bin
C:\hamlib\bin
```

Set environment variables:
```
QT_ROOT_DIR=C:\Qt\6.10.1\mingw_64
HAMLIB_ROOT=C:\hamlib
```

### 5. SSH Config (on Mac)

Add to `~/.ssh/config`:
```
Host windows-build
    HostName 192.168.x.x
    User youruser
```

### 6. Test Build

From Mac:
```bash
ssh windows-build "cd /d C:\projects\TR4QT && cmake -B build -G \"MinGW Makefiles\" && cmake --build build"
```

## Optional: Convert to Self-Hosted Runner

If you want automated CI on this VM later:

1. Go to: https://github.com/ny4i/TR4QT/settings/actions/runners/new
2. Select Windows x64
3. Follow the installation steps
4. Configure service to run at startup

## Notes

- Windows SSH uses PowerShell by default; use `cmd /c` for cmd.exe commands
- VirtIO drivers significantly improve disk I/O
- Consider snapshots before major Windows updates

# Raspberry Pi 5 ARM64 Self-Hosted Runner Setup

For building ARM64/aarch64 Linux AppImages on native hardware.

## Requirements

- Raspberry Pi 5 (8GB model recommended, 4GB works with swap)
- 64-bit Raspberry Pi OS or Ubuntu 24.04
- 32GB+ SD card or NVMe (NVMe preferred for build speed)
- Network access to GitHub

## 1. Install Build Dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    wget \
    pkg-config \
    libgl1-mesa-dev \
    libxkbcommon-dev \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-keysyms1 \
    libxcb-shape0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libxcb-xfixes0-dev \
    libxcb-randr0-dev \
    libusb-1.0-0-dev \
    libsecret-1-dev \
    libfuse2 \
    imagemagick \
    file \
    python3-pip \
    pipx
```

## 2. Install Qt 6.10.1 via aqtinstall

```bash
pipx install aqtinstall
pipx ensurepath
source ~/.bashrc

sudo mkdir -p /opt/Qt
sudo chown $USER:$USER /opt/Qt

~/.local/bin/aqt install-qt linux desktop 6.10.1 linux_gcc_64 -m qtwebsockets qthttpserver qtserialport qtshadertools --outputdir /opt/Qt
```

**Note:** aqtinstall may not have ARM64 Qt binaries. If not available, build Qt from source or use system Qt:

```bash
# Alternative: Use system Qt (may be older version)
sudo apt-get install -y qt6-base-dev qt6-websockets-dev qt6-httpserver-dev qt6-serialport-dev libqt6sql6-sqlite
```

## 3. Set Qt Environment

```bash
echo 'export QT_ROOT_DIR=/opt/Qt/6.10.1/gcc_64' >> ~/.bashrc
echo 'export PATH=$QT_ROOT_DIR/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=$QT_ROOT_DIR/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

## 4. Build Hamlib from Source

```bash
cd /tmp
curl -L -o hamlib.tar.gz "https://github.com/Hamlib/Hamlib/releases/download/4.6.5/hamlib-4.6.5.tar.gz"
tar -xzf hamlib.tar.gz
cd hamlib-4.6.5
./configure --prefix=/opt/hamlib --disable-static
make -j4
sudo make install

echo 'export HAMLIB_ROOT=/opt/hamlib' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/opt/hamlib/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

## 5. Install GitHub Actions Runner

```bash
mkdir -p ~/actions-runner && cd ~/actions-runner

# Download ARM64 runner
curl -o actions-runner-linux-arm64.tar.gz -L https://github.com/actions/runner/releases/download/v2.321.0/actions-runner-linux-arm64-2.321.0.tar.gz
tar xzf actions-runner-linux-arm64.tar.gz
```

## 6. Register Runner with GitHub

1. Go to: https://github.com/ny4i/TR4QT/settings/actions/runners/new
2. Select **Linux** and **ARM64**
3. Copy the token

```bash
./config.sh --url https://github.com/ny4i/TR4QT --token YOUR_TOKEN_HERE

# When prompted for labels, use: self-hosted,linux,ARM64
```

## 7. Install as Service

```bash
sudo ./svc.sh install
sudo ./svc.sh start
sudo ./svc.sh status
```

## 8. Update Workflow (build.yml)

Add ARM64 job:

```yaml
build-linux-arm64:
  name: Linux (ARM64)
  runs-on: [self-hosted, linux, ARM64]
  continue-on-error: true  # Optional until stable

  env:
    QT_ROOT_DIR: /opt/Qt/6.10.1/gcc_64
    HAMLIB_ROOT: /opt/hamlib

  steps:
  - name: Checkout
    uses: actions/checkout@v4

  - name: Configure CMake
    run: |
      cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${{ env.QT_ROOT_DIR }}" \
        -DHAMLIB_ROOT="${{ env.HAMLIB_ROOT }}" \
        -DCMAKE_INSTALL_PREFIX=/usr

  # ... rest of build steps same as x86_64
```

## Performance Notes

- **8GB Pi 5**: Can use `make -j4` comfortably
- **4GB Pi 5**: Use `make -j2` and add swap:
  ```bash
  sudo fallocate -l 8G /swapfile
  sudo chmod 600 /swapfile
  sudo mkswap /swapfile
  sudo swapon /swapfile
  echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
  ```
- **Expected build time**: 20-40 minutes (vs ~5 min on x86_64)
- **NVMe storage**: Highly recommended over SD card for faster I/O

## Troubleshooting

### Qt ARM64 binaries not available via aqtinstall

Build Qt from source or use Ubuntu/Debian packaged Qt:
```bash
sudo apt-get install qt6-base-dev qt6-websockets-dev ...
export QT_ROOT_DIR=/usr/lib/aarch64-linux-gnu/qt6
```

### Out of memory during build

Reduce parallel jobs:
```bash
cmake --build build -j2
```

### Runner not picking up jobs

Check labels match workflow `runs-on`:
```bash
cd ~/actions-runner
cat .runner | grep labels
```

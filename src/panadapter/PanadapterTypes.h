/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PANADAPTERTYPES_H
#define PANADAPTERTYPES_H

#include <QVector>
#include <QString>

namespace TR4QT {

/**
 * @brief Parsed panadapter packet from K4 radio
 */
struct PanadapterPacket {
    char panId{'A'};              // 'A', 'B' (main), 'Y', 'Z' (mini)
    qint64 centerFreqHz{0};       // Center frequency in Hz
    int sampleRateHz{48000};      // Sample rate (typically 48000)
    float noiseFloor{-130.0f};    // Noise floor in dB
    int sequenceNumber{0};        // Packet sequence (0-255)
    QVector<float> samples;       // 2048 dB values
    bool isValid{false};          // Packet passed validation

    static const int SAMPLE_COUNT = 2048;
    static const int HEADER_SIZE = 64;
    static const int PAYLOAD_SIZE = 4096;
    static const int FOOTER_SIZE = 2;
    static const int PACKET_SIZE = HEADER_SIZE + PAYLOAD_SIZE + FOOTER_SIZE;  // 4162
};

/**
 * @brief Color palette options for waterfall display
 */
enum class WaterfallPalette {
    HeatMap,      // Blue → Cyan → Green → Yellow → Red
    Grayscale,    // Black → White
    Sepia,        // Brown tones (like K4LanExample)
    BlueGreen,    // Blue → Green
    FireIce       // Blue → White → Red
};

/**
 * @brief Panadapter display settings
 */
struct PanadapterSettings {
    int averaging{3};                           // Sample averaging (1-10)
    WaterfallPalette palette{WaterfallPalette::HeatMap};
    float refLevel{0.0f};                       // Reference level adjustment in dB
    int waterfallHeight{400};                   // Waterfall height in pixels
    int spectrumHeight{200};                    // Spectrum height in pixels
    bool showSpectrum{true};                    // Show spectrum line graph
    bool showWaterfall{true};                   // Show waterfall
    bool peakHold{false};                       // Hold spectrum peaks
    int frameRate{30};                          // Target frame rate
};

} // namespace TR4QT

#endif // PANADAPTERTYPES_H

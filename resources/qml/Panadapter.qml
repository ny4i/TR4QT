/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

import QtQuick
import TR4QT.Panadapter 1.0

Rectangle {
    id: root
    color: "#1a1a1a"

    // Waterfall refresh timer - throttle image updates (fallback only, not used with QRhi)
    Timer {
        id: waterfallRefreshTimer
        interval: 66  // ~15fps
        repeat: false
        onTriggered: {
            // Force image reload by changing the source URL (fallback path)
            if (typeof waterfallImage !== "undefined") {
                waterfallImage.source = "";
                waterfallImage.source = "image://waterfall/frame" + panadapter.waterfallFrame;
            }
        }
    }

    // Frequency formatting helper
    function formatFrequency(freqHz) {
        var mhz = freqHz / 1000000.0;
        return mhz.toFixed(3) + " MHz";
    }

    // Main content column
    Column {
        anchors.fill: parent
        spacing: 0

        // Spectrum display - GPU-accelerated gradient fill with glow effect
        Rectangle {
            id: spectrumArea
            width: parent.width
            height: 150
            color: "#0a0a0a"

            // GPU-rendered spectrum using QQuickRhiItem
            SpectrumRhiItem {
                id: spectrumRhiItem
                anchors.fill: parent

                refLevel: panadapter ? panadapter.refLevel : 0
                noiseFloor: panadapter ? panadapter.noiseFloor : -130

                Connections {
                    target: panadapter
                    function onSamplesChanged() {
                        if (!panadapter.paused) {
                            spectrumRhiItem.updateSpectrum(panadapter.samples);
                        }
                    }
                }
            }

            // Center frequency marker (overlay on spectrum)
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: "#ff0000"
                opacity: 0.7
            }

            // DX Spot labels overlay
            Repeater {
                model: panadapter ? panadapter.visibleSpots : []

                Item {
                    id: spotItem
                    property var spotData: modelData
                    property real xPos: spotData.xRatio * spectrumArea.width

                    x: xPos - spotLabel.width / 2
                    y: 5  // Offset from top

                    // Vertical line from label to bottom of spectrum
                    Rectangle {
                        x: spotLabel.width / 2
                        y: spotLabel.height
                        width: 1
                        height: spectrumArea.height - spotItem.y - spotLabel.height
                        color: spotData.isMultiplier ? "#ff00ff" : (spotData.isWorked ? "#666666" : "#00ffff")
                        opacity: 0.7
                    }

                    // Callsign label
                    Rectangle {
                        id: spotLabel
                        width: spotText.width + 6
                        height: spotText.height + 2
                        color: spotData.isMultiplier ? "#400040" : (spotData.isWorked ? "#333333" : "#004040")
                        border.color: spotData.isMultiplier ? "#ff00ff" : (spotData.isWorked ? "#666666" : "#00ffff")
                        border.width: 1
                        radius: 2

                        Text {
                            id: spotText
                            anchors.centerIn: parent
                            text: spotData.callsign
                            color: spotData.isMultiplier ? "#ff00ff" : (spotData.isWorked ? "#888888" : "#00ffff")
                            font.pixelSize: 9
                            font.bold: spotData.isMultiplier
                        }

                        // Click to tune
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: function(mouse) {
                                var vfo = (mouse.button === Qt.RightButton) ? 1 : 0;
                                panadapter.onFrequencyClicked(spotData.frequency, vfo);
                            }
                        }
                    }
                }
            }

            // dB scale on right
            Column {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.rightMargin: 5
                width: 40

                Repeater {
                    model: 5
                    Text {
                        width: parent.width
                        height: spectrumArea.height / 5
                        color: "#888888"
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignRight
                        text: {
                            var noiseFloor = panadapter ? panadapter.noiseFloor : -130;
                            var refLevel = panadapter ? panadapter.refLevel : 0;
                            var minDb = Math.min(Math.max(noiseFloor, -150), -100) - 8 + refLevel;
                            var maxDb = -20 + refLevel;
                            var db = maxDb - (index * (maxDb - minDb) / 4);
                            return db.toFixed(0) + " dB";
                        }
                    }
                }
            }
        }

        // Waterfall display - uses GPU-accelerated QRhi rendering
        Rectangle {
            id: waterfallArea
            width: parent.width
            height: parent.height - spectrumArea.height - freqScale.height
            color: "#00001e"  // Dark blue background (matches QRhi clear color)
            clip: true

            // GPU-accelerated waterfall using QQuickRhiItem
            WaterfallRhiItem {
                id: waterfallRhiItem
                anchors.fill: parent

                // Bind settings from panadapter provider
                // refLevel is for spectrum (not used by waterfall shader)
                refLevel: panadapter ? panadapter.refLevel : 0
                // waterfallRefLevel is independent control for waterfall brightness
                waterfallRefLevel: panadapter ? panadapter.waterfallRefLevel : 0
                waterfallRange: panadapter ? panadapter.waterfallRange : 80

                // Feed new sample rows to the GPU renderer
                Connections {
                    target: panadapter
                    function onSamplesChanged() {
                        if (!panadapter.paused) {
                            waterfallRhiItem.addRow(panadapter.samples);
                        }
                    }
                }
            }

            // Center frequency marker (overlay on top of image)
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: "#0088ff"
                opacity: 0.7
            }

            // Click handler for tuning
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true

                onClicked: function(mouse) {
                    var xRatio = mouse.x / width;
                    var freq = panadapter.frequencyAtPosition(xRatio);
                    var vfo = (mouse.button === Qt.RightButton) ? 1 : 0;
                    panadapter.onFrequencyClicked(freq, vfo);
                }

                onPositionChanged: function(mouse) {
                    var xRatio = mouse.x / width;
                    var freq = panadapter.frequencyAtPosition(xRatio);
                    var sampleIndex = Math.floor(xRatio * 2048);
                    var samples = panadapter.samples;
                    var db = (samples && sampleIndex < samples.length) ? samples[sampleIndex] : -130;
                    panadapter.onCursorMoved(freq, db);
                    cursorFreqText.text = formatFrequency(freq);
                    cursorDbText.text = db.toFixed(1) + " dB";
                }

                onExited: {
                    cursorFreqText.text = "";
                    cursorDbText.text = "";
                }
            }

            // Cursor info overlay
            Row {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 5
                spacing: 10

                Text {
                    id: cursorFreqText
                    color: "#ffff00"
                    font.pixelSize: 12
                    font.bold: true
                }
                Text {
                    id: cursorDbText
                    color: "#00ff00"
                    font.pixelSize: 12
                    font.bold: true
                }
            }
        }

        // Frequency scale
        Rectangle {
            id: freqScale
            width: parent.width
            height: 25
            color: "#1a1a1a"

            Row {
                anchors.fill: parent
                anchors.leftMargin: 5
                anchors.rightMargin: 5

                Repeater {
                    model: 5
                    Text {
                        width: parent.width / 5
                        height: parent.height
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        horizontalAlignment: index === 2 ? Text.AlignHCenter : (index < 2 ? Text.AlignLeft : Text.AlignRight)
                        verticalAlignment: Text.AlignVCenter
                        text: {
                            var centerFreq = panadapter ? panadapter.centerFrequency : 7200000;
                            var sampleRate = panadapter ? panadapter.sampleRate : 48000;
                            var halfSpan = sampleRate / 2;
                            var freqOffset = (index - 2) * halfSpan / 2;
                            var freq = centerFreq + freqOffset;
                            return formatFrequency(freq);
                        }
                    }
                }
            }
        }
    }

    // Center frequency display
    Text {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 5
        color: "#ffff00"
        font.pixelSize: 14
        font.bold: true
        text: formatFrequency(panadapter ? panadapter.centerFrequency : 7200000)
    }

    // Pan ID indicator
    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 5
        color: "#00ffff"
        font.pixelSize: 14
        font.bold: true
        text: "Pan " + (panadapter ? panadapter.panId : "A")
    }

    // Paused indicator
    Text {
        anchors.centerIn: parent
        color: "#ff0000"
        font.pixelSize: 24
        font.bold: true
        visible: panadapter ? panadapter.paused : false
        text: "PAUSED"
    }
}

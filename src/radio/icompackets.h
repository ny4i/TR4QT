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

#ifndef ICOMPACKETS_H
#define ICOMPACKETS_H

#include <QtGlobal>

// Timing constants
#define AREYOUTHERE_PERIOD 500
#define PING_PERIOD 500
#define IDLE_PERIOD 100
#define TOKEN_RENEWAL 60000
#define RETRANSMIT_PERIOD 100
#define WATCHDOG_PERIOD 500
#define BUFSIZE 500
#define MAX_MISSING 50
#define GUIDLEN 16

// Fixed packet sizes
#define CONTROL_SIZE 0x10
#define PING_SIZE 0x15
#define OPENCLOSE_SIZE 0x16
#define TOKEN_SIZE 0x40
#define STATUS_SIZE 0x50
#define LOGIN_RESPONSE_SIZE 0x60
#define LOGIN_SIZE 0x80
#define CONNINFO_SIZE 0x90
#define CAPABILITIES_SIZE 0x42
#define RADIO_CAP_SIZE 0x66
#define CIV_SIZE 0x15

#pragma pack(push, 1)

// 0x10 length control packet (connect/disconnect/idle)
struct control_packet {
    quint32 len;
    quint16 type;
    quint16 seq;
    quint32 sentid;
    quint32 rcvdid;
};

// 0x15 length ping packet
struct ping_packet {
    quint32 len;        // 0x00
    quint16 type;       // 0x04
    quint16 seq;        // 0x06
    quint32 sentid;     // 0x08
    quint32 rcvdid;     // 0x0c
    quint8  reply;      // 0x10
    quint32 time;       // 0x11 (uptime of device)
};

// 0x15 length CI-V data header packet
struct data_packet {
    quint32 len;        // 0x00
    quint16 type;       // 0x04
    quint16 seq;        // 0x06
    quint32 sentid;     // 0x08
    quint32 rcvdid;     // 0x0c
    quint8  reply;      // 0x10
    quint16 datalen;    // 0x11
    quint16 sendseq;    // 0x13
    // Followed by actual CI-V data
};

// 0x16 length open/close packet (for CI-V stream)
struct openclose_packet {
    quint32 len;        // 0x00
    quint16 type;       // 0x04
    quint16 seq;        // 0x06
    quint32 sentid;     // 0x08
    quint32 rcvdid;     // 0x0c
    quint16 data;       // 0x10
    char unused;        // 0x12
    quint16 sendseq;    // 0x13
    char magic;         // 0x15
};

// 0x40 length token packet
struct token_packet {
    quint32 len;                // 0x00
    quint16 type;               // 0x04
    quint16 seq;                // 0x06
    quint32 sentid;             // 0x08
    quint32 rcvdid;             // 0x0c
    quint32 payloadsize;        // 0x10
    quint8 requestreply;        // 0x14
    quint8 requesttype;         // 0x15
    quint16 innerseq;           // 0x16
    char unusedb[2];            // 0x18
    quint16 tokrequest;         // 0x1a
    quint32 token;              // 0x1c
    union {
        struct {
            quint16 authstartid;    // 0x20
            char unusedg2[2];       // 0x22
            quint16 resetcap;       // 0x24
            char unusedg1;          // 0x26
            quint16 commoncap;      // 0x27
            char unusedh;           // 0x29
            quint8 macaddress[6];   // 0x2a
        };
        quint8 guid[GUIDLEN];       // 0x20
    };
    quint32 response;           // 0x30
    char unusede[12];           // 0x34
};

// 0x50 length status packet
struct status_packet {
    quint32 len;                // 0x00
    quint16 type;               // 0x04
    quint16 seq;                // 0x06
    quint32 sentid;             // 0x08
    quint32 rcvdid;             // 0x0c
    quint32 payloadsize;        // 0x10
    quint8 requestreply;        // 0x14
    quint8 requesttype;         // 0x15
    quint16 innerseq;           // 0x16
    char unusedb[2];            // 0x18
    quint16 tokrequest;         // 0x1a
    quint32 token;              // 0x1c
    union {
        struct {
            quint16 authstartid;    // 0x20
            char unusedd[5];        // 0x22
            quint16 commoncap;      // 0x27
            char unusede;           // 0x29
            quint8 macaddress[6];   // 0x2a
        };
        quint8 guid[GUIDLEN];       // 0x20
    };
    quint32 error;              // 0x30
    char unusedg[12];           // 0x34
    char disc;                  // 0x40
    char unusedh;               // 0x41
    quint16 civport;            // 0x42 // Sent bigendian
    quint16 unusedi;            // 0x44
    quint16 audioport;          // 0x46
    char unusedj[7];            // 0x48
};

// 0x60 length login response packet
struct login_response_packet {
    quint32 len;                // 0x00
    quint16 type;               // 0x04
    quint16 seq;                // 0x06
    quint32 sentid;             // 0x08
    quint32 rcvdid;             // 0x0c
    quint32 payloadsize;        // 0x10
    quint8 requestreply;        // 0x14
    quint8 requesttype;         // 0x15
    quint16 innerseq;           // 0x16
    char unusedb[2];            // 0x18
    quint16 tokrequest;         // 0x1a
    quint32 token;              // 0x1c
    quint16 authstartid;        // 0x20
    char unusedd[14];           // 0x22
    quint32 error;              // 0x30
    char unusede[12];           // 0x34
    char connection[16];        // 0x40
    char unusedf[16];           // 0x50
};

// 0x80 length login packet
struct login_packet {
    quint32 len;                // 0x00
    quint16 type;               // 0x04
    quint16 seq;                // 0x06
    quint32 sentid;             // 0x08
    quint32 rcvdid;             // 0x0c
    quint32 payloadsize;        // 0x10
    quint8 requestreply;        // 0x14
    quint8 requesttype;         // 0x15
    quint16 innerseq;           // 0x16
    char unusedb[2];            // 0x18
    quint16 tokrequest;         // 0x1a
    quint32 token;              // 0x1c
    char unusedc[32];           // 0x20
    char username[16];          // 0x40
    char password[16];          // 0x50
    char name[16];              // 0x60
    char unusedf[16];           // 0x70
};

// 0x90 length conninfo/stream request packet
struct conninfo_packet {
    quint32 len;              // 0x00
    quint16 type;             // 0x04
    quint16 seq;              // 0x06
    quint32 sentid;           // 0x08
    quint32 rcvdid;           // 0x0c
    quint32 payloadsize;      // 0x10
    quint8 requestreply;      // 0x14
    quint8 requesttype;       // 0x15
    quint16 innerseq;         // 0x16
    char unusedb[2];          // 0x18
    quint16 tokrequest;       // 0x1a
    quint32 token;            // 0x1c
    union {
        struct {
            quint16 authstartid;    // 0x20
            char unusedg[5];        // 0x22
            quint16 commoncap;      // 0x27
            char unusedh;           // 0x29
            quint8 macaddress[6];   // 0x2a
        };
        quint8 guid[GUIDLEN];       // 0x20
    };
    char unusedab[16];        // 0x30
    char name[32];            // 0x40
    union {
        struct { // Receive
            quint32 busy;            // 0x60
            char computer[16];       // 0x64
            char unusedi[16];        // 0x74
            quint32 ipaddress;       // 0x84
            char unusedj[8];         // 0x88
        };
        struct { // Send
            char username[16];    // 0x60
            char rxenable;        // 0x70
            char txenable;        // 0x71
            char rxcodec;         // 0x72
            char txcodec;         // 0x73
            quint32 rxsample;     // 0x74
            quint32 txsample;     // 0x78
            quint32 civport;      // 0x7c
            quint32 audioport;    // 0x80
            quint32 txbuffer;     // 0x84
            quint8 convert;       // 0x88
            char unusedl[7];      // 0x89
        };
    };
};

// 0x66 length radio capabilities
struct radio_cap_packet {
    union {
        struct {
            quint8 unusede[7];          // 0x00
            quint16 commoncap;          // 0x07
            quint8 unused;              // 0x09
            quint8 macaddress[6];       // 0x0a
        };
        quint8 guid[GUIDLEN];           // 0x00
    };
    char name[32];            // 0x10
    char audio[32];           // 0x30
    quint16 conntype;         // 0x50
    char civ;                 // 0x52
    quint16 rxsample;         // 0x53
    quint16 txsample;         // 0x55
    quint8 enablea;           // 0x57
    quint8 enableb;           // 0x58
    quint8 enablec;           // 0x59
    quint32 baudrate;         // 0x5a
    quint16 capf;             // 0x5e
    char unusedi;             // 0x60
    quint16 capg;             // 0x61
    char unusedj[3];          // 0x63
};

// 0x42 length capabilities packet header
struct capabilities_packet {
    quint32 len;              // 0x00
    quint16 type;             // 0x04
    quint16 seq;              // 0x06
    quint32 sentid;           // 0x08
    quint32 rcvdid;           // 0x0c
    quint32 payloadsize;      // 0x10
    quint8 requestreply;      // 0x14
    quint8 requesttype;       // 0x15
    quint16 innerseq;         // 0x16
    char unusedb[2];          // 0x18
    quint16 tokrequest;       // 0x1a
    quint32 token;            // 0x1c
    char unusedd[32];         // 0x20
    quint16 numradios;        // 0x40
    // Followed by numradios * radio_cap_packet structures
};

#pragma pack(pop)

/// <summary>
/// Password encoding function used by Icom
/// Encodes username or password using Icom's substitution cipher
/// </summary>
/// <param name="in">Input string (username or password)</param>
/// <param name="out">Output encoded byte array</param>
static inline void passcode(const QString& in, QByteArray& out)
{
    const quint8 sequence[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x47,0x5d,0x4c,0x42,0x66,0x20,0x23,0x46,0x4e,0x57,0x45,0x3d,0x67,0x76,0x60,0x41,
        0x62,0x39,0x59,0x2d,0x68,0x7e,0x7c,0x65,0x7d,0x49,0x29,0x72,0x73,0x78,0x21,0x6e,
        0x5a,0x5e,0x4a,0x3e,0x71,0x2c,0x2a,0x54,0x3c,0x3a,0x63,0x4f,0x43,0x75,0x27,0x79,
        0x5b,0x35,0x70,0x48,0x6b,0x56,0x6f,0x34,0x32,0x6c,0x30,0x61,0x6d,0x7b,0x2f,0x4b,
        0x64,0x38,0x2b,0x2e,0x50,0x40,0x3f,0x55,0x33,0x37,0x25,0x77,0x24,0x26,0x74,0x6a,
        0x28,0x53,0x4d,0x69,0x22,0x5c,0x44,0x31,0x36,0x58,0x3b,0x7a,0x51,0x5f,0x52,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };

    QByteArray ba = in.toLocal8Bit();
    const uchar* ascii = (const uchar*)ba.constData();
    for (int i = 0; i < in.length() && i < 16; i++)
    {
        int p = ascii[i] + i;
        if (p > 126)
        {
            p = 32 + p % 127;
        }
        out.append(sequence[p]);
    }
}

#endif // ICOMPACKETS_H

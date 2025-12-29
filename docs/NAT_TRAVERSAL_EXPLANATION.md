# NAT Traversal for P2P TCP Networking

**Quick Answer:** NAT traversal IS an issue for P2P systems even with TCP, because peer-to-peer requires **bidirectional connectivity** while NAT only allows **outbound** connections by default.

---

## The NAT Problem for P2P

### Client-Server (No NAT Issues)

```
Home Network              Internet              Server
┌─────────────┐          │                    ┌────────┐
│             │          │                    │        │
│ Client A    │          │                    │ Server │
│ 192.168.1.5 │──────────┼───────────────────>│ Public │
│             │ Outbound │                    │ IP     │
│             │ TCP:443  │                    │ 1.2.3.4│
└─────────────┘          │                    └────────┘
      ▲                  │
      │                  │
   NAT Router            │
   Allows outbound       │
   connections           │
```

**Works because:**
- Client initiates outbound connection (NAT allows this)
- Server has public IP and accepts incoming connections
- **Direction: Client → Server (one-way initiation)**

---

### Peer-to-Peer (NAT Is a Problem!)

```
Home Network A           Internet           Home Network B
┌─────────────┐                            ┌─────────────┐
│             │                            │             │
│ Peer A      │                            │ Peer B      │
│ 192.168.1.5 │────────────X───────────────│ 192.168.1.7 │
│             │   Can't connect!           │             │
│             │                            │             │
└─────────────┘                            └─────────────┘
      ▲                                          ▲
      │                                          │
   NAT Router A                              NAT Router B
   Blocks incoming                           Blocks incoming
   connections                               connections
```

**Problem:**
- Peer A wants to connect to Peer B
- Peer B is behind NAT at 192.168.1.7 (private IP)
- NAT Router B blocks incoming connections to 192.168.1.7
- **Neither peer can accept connections from the other!**

---

## Why NAT Blocks Incoming Connections

### How NAT Works

1. **Outbound connection** (Client → Internet):
   ```
   Client 192.168.1.5:50000 → NAT translates → Public 1.2.3.4:60000 → Internet
   NAT creates mapping: 1.2.3.4:60000 ↔ 192.168.1.5:50000
   ```

2. **Return traffic** (Internet → Client):
   ```
   Internet → 1.2.3.4:60000 → NAT uses mapping → 192.168.1.5:50000
   ```

3. **Unsolicited incoming** (Internet → NAT):
   ```
   Internet → 1.2.3.4:7300 → NAT has NO MAPPING → ❌ DROPPED
   ```

**NAT only allows incoming packets if there's an existing mapping created by an outbound connection.**

---

## NAT Types

### 1. Full Cone NAT (Easiest)
```
Internal 192.168.1.5:7300 → External 1.2.3.4:60000

After mapping created:
ANY external host can send to 1.2.3.4:60000 → reaches 192.168.1.5:7300
```

**P2P Solution:** STUN to discover public IP/port, share with peers

### 2. Restricted Cone NAT
```
Internal 192.168.1.5:7300 → External 1.2.3.4:60000

Only hosts that 192.168.1.5 has sent to can reply to 1.2.3.4:60000
```

**P2P Solution:** Connection reversal or TURN relay

### 3. Port-Restricted Cone NAT
```
Same as Restricted Cone, but must match exact source IP:port
```

**P2P Solution:** TCP hole punching or TURN

### 4. Symmetric NAT (Hardest)
```
Different external port for each destination:
  192.168.1.5:7300 → PeerA = 1.2.3.4:60000
  192.168.1.5:7300 → PeerB = 1.2.3.4:60001  (different port!)
```

**P2P Solution:** TURN relay required

---

## P2P NAT Traversal Techniques

### Technique 1: Direct Connection (Public IPs)

**Works when:** At least one peer has a public IP or port forwarding configured

```
┌──────────────────────────────────────────────────────────┐
│ Contest Station Setup                                    │
│                                                          │
│ Station A (Public IP)         Station B (Private IP)    │
│ ┌────────┐                    ┌────────┐                │
│ │  1.2.3.4│                   │  Router│                │
│ │  :7300  │◄───────────────────┤  NAT   │               │
│ └────────┘    B connects      └────┬───┘                │
│                                    │                     │
│                               ┌────▼────┐                │
│                               │192.168  │                │
│                               │  .1.5   │                │
│                               └─────────┘                │
└──────────────────────────────────────────────────────────┘

B initiates: TCP connect to 1.2.3.4:7300 → Works! (outbound)
A initiates: TCP connect to B's public IP → Works if port forwarded!
```

**Configuration:**
```json
{
  "peers": [
    {"id": "A", "host": "1.2.3.4", "port": 7300},           // Public IP
    {"id": "B", "host": "contest.dyndns.org", "port": 7301} // Dynamic DNS
  ]
}
```

**Setup required:**
- Station with public IP: None
- Station behind NAT: Configure port forwarding (NAT router admin page)

---

### Technique 2: UPnP/NAT-PMP (Automatic Port Forwarding)

**How it works:** Application automatically configures NAT router

```cpp
// Using Qt UPnP library (or libnatpmp)
#include <QUPnPDeviceDiscovery>
#include <QUPnPPortMapper>

void setupUPnP(quint16 port) {
    QUPnPPortMapper mapper;

    // Request port forwarding
    mapper.mapPort(port,                    // Internal port
                   port,                    // External port (same)
                   QUPnPPortMapper::TCP,
                   "TR4QT P2P",             // Description
                   3600);                   // Lease time (1 hour)

    // Discover external IP
    QString externalIP = mapper.externalIP();
    qInfo() << "External IP:" << externalIP << ":" << port;
}
```

**Pros:**
- ✅ Automatic (no user configuration)
- ✅ Works with most home routers

**Cons:**
- ❌ Not all routers support UPnP
- ❌ Some networks disable UPnP (security)
- ❌ Doesn't work with carrier-grade NAT (CGNAT)

---

### Technique 3: STUN (Discover Public Endpoint)

**STUN Server:** Public server that tells you your public IP:port

```
Peer A (behind NAT)         STUN Server (public)
┌─────────────┐             ┌────────────┐
│             │             │            │
│ Send packet │────────────>│ STUN       │
│ to STUN     │             │ 3.4.5.6:19302
│             │             │            │
│             │<────────────┤ Reply:     │
│             │   "Your IP:port is      │
│             │    1.2.3.4:60000"       │
└─────────────┘             └────────────┘

Now Peer A knows: "I'm reachable at 1.2.3.4:60000"
```

**Implementation:**
```cpp
#include <QUdpSocket>

void discoverPublicEndpoint() {
    QUdpSocket socket;

    // Send STUN request to public STUN server
    socket.writeDatagram(stunRequest, QHostAddress("stun.l.google.com"), 19302);

    // Wait for reply
    QByteArray reply;
    socket.readDatagram(reply.data(), reply.size());

    // Parse STUN response → get public IP:port
    QString publicIP = parseSTUN(reply);
    quint16 publicPort = parseSTUNPort(reply);

    qInfo() << "Public endpoint:" << publicIP << ":" << publicPort;

    // Share this with other peers via signaling server
}
```

**STUN Servers (Free Public):**
- `stun.l.google.com:19302`
- `stun1.l.google.com:19302`
- `stun.stunprotocol.org:3478`

**Limitation:** Only works with Full Cone and Restricted Cone NAT

---

### Technique 4: Connection Reversal

**Idea:** If A can't connect to B, but B can connect to A, let B initiate

```
Signaling Server (public)
┌────────────────┐
│                │
│  Coordinates   │
│  connections   │
└───┬────────┬───┘
    │        │
    ▼        ▼
┌───────┐ ┌───────┐
│Peer A │ │Peer B │
│(NAT)  │ │(Public│
└───────┘ └───────┘

1. A tries to connect to B → Success (outbound through NAT)
2. B tries to connect to A → Fails (NAT blocks)
3. Signaling server tells B: "You connect to A instead"
4. B initiates connection → Success (A's NAT allows because B has public IP)
```

**Implementation:**
```cpp
void PeerManager::establishConnection(const QString& peerId, const QString& host, quint16 port) {
    // Try direct connection
    auto* conn = new PeerConnection(peerId);
    conn->connectToPeer(host, port);

    // If fails after 5 seconds, request reversal
    QTimer::singleShot(5000, [=]() {
        if (!conn->isConnected()) {
            qWarning() << "Direct connection to" << peerId << "failed";
            requestConnectionReversal(peerId);  // Ask peer to connect to us
        }
    });
}
```

---

### Technique 5: TURN Relay (Last Resort)

**TURN Server:** Public relay server for when direct connection impossible

```
Peer A (NAT)         TURN Server (public)         Peer B (NAT)
┌─────────┐          ┌──────────────┐             ┌─────────┐
│         │          │              │             │         │
│ Connect │─────────>│ Allocate     │<────────────│ Connect │
│ to TURN │          │ relay        │             │ to TURN │
│         │          │              │             │         │
│ Send    │─────────>│ Forward─────>│────────────>│ Receive │
│ data    │          │ data         │             │ data    │
└─────────┘          └──────────────┘             └─────────┘
```

**Implementation:**
```cpp
void PeerManager::useTURNRelay(const QString& peerId) {
    // Connect to TURN server
    QTcpSocket* turnSocket = new QTcpSocket();
    turnSocket->connectToHost("turn.example.com", 3478);

    // Authenticate with TURN server
    sendTURNAllocateRequest(turnSocket);

    // TURN server assigns relay address
    QString relayAddress = receiveTURNRelayAddress(turnSocket);

    // Share relay address with peer (via signaling)
    shareRelayAddress(peerId, relayAddress);

    // All traffic now goes through TURN server
    // Downsides: Increased latency, bandwidth costs
}
```

**TURN Servers:**
- **Commercial:** Twilio STUN/TURN, Xirsys, Metered TURN
- **Open Source:** coturn (self-hosted)

**Costs:**
- Bandwidth intensive (all data relayed)
- Typical: $0.80/GB for relay traffic

---

## Recommended Solution for TR4QT

### Strategy: Layered Fallback

```cpp
class NATTraversalStrategy {
public:
    enum Method {
        DIRECT,             // Direct TCP (no NAT or port forwarding)
        UPNP,               // Automatic port forwarding
        CONNECTION_REVERSAL,// Peer initiates connection
        TURN_RELAY          // Relay via TURN server
    };

    Method currentMethod() const { return m_method; }

    void attemptConnection(const QString& peerId, const QString& host, quint16 port) {
        // 1. Try direct connection
        if (tryDirect(host, port)) {
            m_method = DIRECT;
            return;
        }

        // 2. Try UPnP
        if (tryUPnP() && tryDirect(host, port)) {
            m_method = UPNP;
            return;
        }

        // 3. Try connection reversal (peer connects to us)
        if (tryConnectionReversal(peerId)) {
            m_method = CONNECTION_REVERSAL;
            return;
        }

        // 4. Fall back to TURN relay
        if (tryTURNRelay(peerId, host, port)) {
            m_method = TURN_RELAY;
            return;
        }

        // Give up
        emit connectionFailed(peerId, "All NAT traversal methods failed");
    }

private:
    Method m_method;
};
```

---

## Configuration for Different Scenarios

### Scenario 1: All Stations on Same LAN
```json
{
  "network": {
    "nat_traversal": "disabled",
    "peers": [
      {"id": "A", "host": "192.168.1.101", "port": 7300},
      {"id": "B", "host": "192.168.1.102", "port": 7301}
    ]
  }
}
```
**NAT Traversal:** Not needed ✅

---

### Scenario 2: Stations on Different Networks (Port Forwarding)
```json
{
  "network": {
    "nat_traversal": "manual",
    "external_ip": "1.2.3.4",  // Set by user
    "external_port": 7300,

    "peers": [
      {"id": "A", "host": "contest1.dyndns.org", "port": 7300},
      {"id": "B", "host": "5.6.7.8", "port": 7301}
    ]
  }
}
```

**Setup Required:**
1. User configures port forwarding on router: `External 7300 → Internal 192.168.1.5:7300`
2. User sets up Dynamic DNS (if no static IP)
3. Manually enter peer addresses

**NAT Traversal:** Manual port forwarding ⚙️

---

### Scenario 3: Automatic with UPnP
```json
{
  "network": {
    "nat_traversal": "upnp",
    "upnp_enabled": true,
    "listen_port": 7300,

    "signaling_server": "signal.tr4qt.net:8443",  // Coordinate connections

    "peers": [
      {"id": "A"},  // Address discovered via signaling
      {"id": "B"}
    ]
  }
}
```

**Flow:**
1. Each peer discovers external IP via UPnP/STUN
2. Registers with signaling server: `{"id": "A", "address": "1.2.3.4:60000"}`
3. Queries signaling server for peer addresses
4. Attempts direct connections

**NAT Traversal:** Automatic UPnP + STUN ✅

---

### Scenario 4: Fully Automatic (STUN + TURN Fallback)
```json
{
  "network": {
    "nat_traversal": "auto",

    "stun_servers": [
      "stun.l.google.com:19302",
      "stun1.l.google.com:19302"
    ],

    "turn_servers": [
      {
        "host": "turn.tr4qt.net",
        "port": 3478,
        "username": "tr4qt_user",
        "password": "secret"
      }
    ],

    "signaling_server": "wss://signal.tr4qt.net",

    "peers": [
      {"id": "A"},
      {"id": "B"},
      {"id": "C"}
    ]
  }
}
```

**Flow:**
1. Discover public endpoint via STUN
2. Register with signaling server
3. Attempt direct connection to peers
4. If fails, fall back to TURN relay

**NAT Traversal:** Fully automatic (STUN + TURN) ✅✅✅

---

## Signaling Server

**Purpose:** Coordinate initial peer discovery (NOT used for relaying data)

```python
# Simple WebSocket signaling server (Python)
import asyncio
import websockets
import json

peers = {}  # {peer_id: websocket}

async def handle_client(websocket, path):
    peer_id = None
    try:
        async for message in websocket:
            data = json.loads(message)

            if data['type'] == 'register':
                peer_id = data['peer_id']
                peers[peer_id] = {
                    'ws': websocket,
                    'address': data['address'],  # "1.2.3.4:60000"
                    'port': data['port']
                }
                print(f"Peer {peer_id} registered at {data['address']}")

            elif data['type'] == 'query':
                # Client wants to know peer addresses
                peer_list = [
                    {'id': pid, 'address': info['address']}
                    for pid, info in peers.items()
                    if pid != data['peer_id']
                ]
                await websocket.send(json.dumps({
                    'type': 'peer_list',
                    'peers': peer_list
                }))

    finally:
        if peer_id:
            del peers[peer_id]

asyncio.run(websockets.serve(handle_client, "0.0.0.0", 8443))
```

**Hosting:**
- Free tier: Heroku, Render, Fly.io
- VPS: DigitalOcean ($5/month)
- Self-hosted: Any server with public IP

---

## Summary

### Why NAT Is a Problem for P2P (Even TCP):
- ❌ NAT blocks **incoming** connections by default
- ❌ P2P requires **bidirectional** connectivity
- ❌ Private IPs (192.168.x.x) are not routable on Internet

### Solutions (in order of preference):
1. ✅ **Same LAN** - No NAT issues (use private IPs)
2. ✅ **Manual port forwarding** - User configures router
3. ✅ **UPnP** - Automatic port forwarding (if supported)
4. ✅ **STUN + connection reversal** - Discover public endpoint
5. ✅ **TURN relay** - Last resort (bandwidth costs)

### For TR4QT:
**Default:** Assume LAN-based operation (no NAT)
**Advanced:** Support UPnP + STUN for Internet-based multi-op
**Future:** Add TURN relay support for fully remote stations

**Client-server architecture avoids this entirely** because:
- Server has public IP (or known port forwarding)
- Clients only make **outbound** connections (NAT allows)
- **No peer-to-peer connections needed**

This is one tradeoff to consider when choosing P2P vs client-server!

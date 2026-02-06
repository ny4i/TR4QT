import ctypes
from ctypes import wintypes
import sys
import socket
import re
import json
from datetime import datetime

# Load Windows DLL
user32 = ctypes.windll.user32

# DDE Constants
APPCLASS_STANDARD = 0x00000000

# Transaction Types
XTYP_CONNECT = 0x1060
XTYP_CONNECT_CONFIRM = 0x8070
XTYP_EXECUTE = 0x4050
XTYP_REQUEST = 0x20B0
XTYP_ADVSTART = 0x1030
XTYP_ADVSTOP = 0x8040
XTYP_ADVREQ = 0x2020
XTYP_POKE = 0x4090
XTYP_REGISTER = 0x80A0
XTYP_UNREGISTER = 0x80D0
XTYP_WILDCONNECT = 0x20E0

# Transaction type masks
XCLASS_MASK = 0xFC00
XTYP_MASK = 0x00F0

# Return values
DDE_FACK = 0x8000
DDE_FNOTPROCESSED = 0x0000
DMLERR_NO_ERROR = 0

# Type definitions
HDDEDATA = wintypes.LPVOID
HCONV = wintypes.LPVOID
HSZ = wintypes.LPVOID

# Define the DDE callback function type
PFNCALLBACK = ctypes.WINFUNCTYPE(
    HDDEDATA,
    wintypes.UINT,
    wintypes.UINT,
    HCONV,
    HSZ,
    HSZ,
    HDDEDATA,
    ctypes.c_size_t,
    ctypes.c_size_t
)

# Global instance ID
g_idInst = wintypes.DWORD(0)

# Global UDP socket
g_udp_socket = None
g_udp_host = None
g_udp_port = None

def timestamp():
    """Return current timestamp in same format as DXLab logs (UTC)."""
    from datetime import timezone
    return datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

def parse_dde_command(command):
    """Parse DDE command and extract spot data.

    Formats:
    - 002getqslinfo<callsign:5>AA1BA
    - 006setscnewentry<scfreq:8>7018.8<scmode:2>CW<sccall:5>OK2LA...
    Returns: (command_name, {callsign, freq, mode}) or (None, None) if not parseable
    """
    try:
        # Remove server prefix (first 3 digits)
        if len(command) < 3:
            return None, None

        data = command[3:]

        # Extract command name (everything before '<')
        cmd_match = re.match(r'^([^<]+)', data)
        if not cmd_match:
            return None, None

        cmd_name = cmd_match.group(1).lower()

        # Extract all fields using pattern <name:length>value
        fields = {}
        field_pattern = r'<([^:]+):(\d+)>([^<]*)'
        for match in re.finditer(field_pattern, data, re.IGNORECASE):
            field_name = match.group(1).lower()
            field_length = int(match.group(2))
            field_value = match.group(3)[:field_length].strip()
            fields[field_name] = field_value

        # Extract spot data
        spot_data = {}

        # Callsign
        for key in ['callsign', 'sccall', 'conveyspotcallsign']:
            if key in fields:
                spot_data['callsign'] = fields[key]
                break

        # Frequency
        for key in ['scfreq', 'conveyspotfreq', 'freq']:
            if key in fields:
                spot_data['freq'] = fields[key]
                break

        # Mode
        for key in ['scmode', 'conveyspotmode', 'mode']:
            if key in fields:
                spot_data['mode'] = fields[key]
                break

        return cmd_name, spot_data if spot_data else None
    except Exception as e:
        print(f"  ERROR parsing command: {e}")
        return None, None

def hsz_to_string(hsz):
    """Convert a DDE string handle (HSZ) to a Python string."""
    if not hsz:
        return ""

    buffer = ctypes.create_string_buffer(256)
    user32.DdeQueryStringA(g_idInst.value, hsz, buffer, 256, 0)
    return buffer.value.decode('latin-1', errors='replace')

def get_execute_data(hdata):
    """Extract execute command data."""
    try:
        if not hdata:
            return "[no data handle]"

        print(f"  DEBUG: hdata=0x{hdata:X}")

        # Get size without pointer first
        size_only = user32.DdeGetData(hdata, None, 0, 0)
        print(f"  DEBUG: DdeGetData says size={size_only}")

        if size_only == 0:
            return "[no data available]"

        # Allocate buffer and get data
        buffer = ctypes.create_string_buffer(size_only)
        actual_size = user32.DdeGetData(hdata, buffer, size_only, 0)

        print(f"  DEBUG: Got {actual_size} bytes")

        if actual_size == 0:
            return "[DdeGetData failed]"

        # Decode the data
        result = buffer.value.decode('latin-1', errors='replace')
        return result

    except Exception as e:
        import traceback
        traceback.print_exc()
        return f"[exception: {e}]"

def dde_callback(uType, uFmt, hconv, hsz1, hsz2, hdata, dwData1, dwData2):
    """DDE server callback function."""

    print(f"{timestamp()} > [CALLBACK] uType=0x{uType:04X}")

    # Mask the transaction type
    uType_masked = uType & (XCLASS_MASK | XTYP_MASK)

    if uType_masked == XTYP_CONNECT:
        service = hsz_to_string(hsz2)
        topic = hsz_to_string(hsz1)
        print(f"{timestamp()} > [CONNECT] Service: {service}, Topic: {topic}")
        print(f"{timestamp()} >   -> Accepting connection")
        # Accept the connection
        return 1

    elif uType_masked == XTYP_CONNECT_CONFIRM:
        service = hsz_to_string(hsz2)
        topic = hsz_to_string(hsz1)
        print(f"[CONNECT_CONFIRM] Service: {service}, Topic: {topic}")
        return DDE_FNOTPROCESSED

    elif uType_masked == XTYP_WILDCONNECT:
        service = hsz_to_string(hsz2)
        topic = hsz_to_string(hsz1)
        print(f"[WILDCONNECT] Service: {service}, Topic: {topic}")
        # Accept wildcard connections - return TRUE to allow any topic
        return 1

    elif uType_masked == XTYP_EXECUTE:
        topic = hsz_to_string(hsz1)
        data = get_execute_data(hdata)
        ts = timestamp()
        print(f"\n{'='*70}")
        print(f"{ts} > [EXECUTE COMMAND RECEIVED]")
        print(f"{ts} >   Topic: {topic}")
        print(f"{ts} >   Command: {data}")

        # Parse command and extract spot data
        cmd_name, spot_data = parse_dde_command(data)
        if cmd_name:
            print(f"{ts} >   Parsed command: {cmd_name}")
        if spot_data:
            callsign = spot_data.get('callsign', '')
            freq = spot_data.get('freq', '')
            mode = spot_data.get('mode', '')

            if callsign:
                print(f"{ts} >   Callsign: {callsign}")
            if freq:
                print(f"{ts} >   Freq: {freq}")
            if mode:
                print(f"{ts} >   Mode: {mode}")

            # Send via UDP if configured
            if g_udp_socket and g_udp_host and g_udp_port and callsign:
                try:
                    # Create JSON message
                    udp_data = {"callsign": callsign}
                    if freq:
                        udp_data["freq"] = freq
                    if mode:
                        udp_data["mode"] = mode

                    udp_msg = json.dumps(udp_data)
                    g_udp_socket.sendto(udp_msg.encode('utf-8'), (g_udp_host, g_udp_port))
                    print(f"{ts} >   -> Sent JSON to {g_udp_host}:{g_udp_port}: {udp_msg}")
                except Exception as e:
                    print(f"{ts} >   -> UDP send failed: {e}")

        print(f"{'='*70}\n")
        return DDE_FACK

    elif uType_masked == XTYP_REQUEST:
        topic = hsz_to_string(hsz1)
        item = hsz_to_string(hsz2)
        print(f"[REQUEST] Topic: {topic}, Item: {item}, Format: {uFmt}")

        # Create a valid data handle with empty string
        data_str = ""
        data_bytes = data_str.encode('latin-1') + b'\x00'
        hdata = user32.DdeCreateDataHandle(
            g_idInst.value,
            data_bytes,
            len(data_bytes),
            0,
            hsz2,  # Use the item handle from the request
            uFmt,
            0
        )
        print(f"  -> Returning data handle: 0x{hdata:X}" if hdata else "  -> Failed to create data handle")
        return hdata if hdata else DDE_FNOTPROCESSED

    elif uType_masked == XTYP_ADVSTART:
        topic = hsz_to_string(hsz1)
        item = hsz_to_string(hsz2)
        print(f"[ADVSTART] Topic: {topic}, Item: {item} - ACCEPTED")
        return DDE_FACK  # Accept the advise loop

    elif uType_masked == XTYP_ADVREQ:
        topic = hsz_to_string(hsz1)
        item = hsz_to_string(hsz2)
        print(f"[ADVREQ] Topic: {topic}, Item: {item}, Format: {uFmt}")

        # Return empty data for advise
        data_str = ""
        data_bytes = data_str.encode('latin-1') + b'\x00'
        hdata = user32.DdeCreateDataHandle(
            g_idInst.value,
            data_bytes,
            len(data_bytes),
            0,
            hsz2,  # Use the item handle from the request
            uFmt,
            0
        )
        print(f"  -> Returning advise data: 0x{hdata:X}" if hdata else "  -> Failed")
        return hdata if hdata else DDE_FNOTPROCESSED

    elif uType_masked == XTYP_POKE:
        topic = hsz_to_string(hsz1)
        item = hsz_to_string(hsz2)
        data = get_execute_data(hdata)
        ts = timestamp()
        print(f"\n{'='*70}")
        print(f"{ts} > [POKE RECEIVED]")
        print(f"{ts} >   Topic: {topic}")
        print(f"{ts} >   Item: {item}")
        print(f"{ts} >   Data: {data}")

        # If it's the SpotCall item, send via UDP
        if item.lower() == "spotcall" and data:
            callsign = data.strip()
            print(f"{ts} >   Callsign: {callsign}")

            # Send via UDP if configured
            if g_udp_socket and g_udp_host and g_udp_port:
                try:
                    g_udp_socket.sendto(callsign.encode('utf-8'), (g_udp_host, g_udp_port))
                    print(f"{ts} >   -> Sent '{callsign}' to {g_udp_host}:{g_udp_port}")
                except Exception as e:
                    print(f"{ts} >   -> UDP send failed: {e}")

        print(f"{'='*70}\n")
        return DDE_FACK

    elif uType_masked == XTYP_REGISTER or uType_masked == XTYP_UNREGISTER:
        # Ignore register/unregister notifications
        return DDE_FNOTPROCESSED

    else:
        ts = timestamp()
        topic = hsz_to_string(hsz1)
        item = hsz_to_string(hsz2)
        print(f"\n{'='*70}")
        print(f"{ts} > [OTHER] uType=0x{uType:04X} (masked: 0x{uType_masked:04X})")
        print(f"{ts} >   Topic: {topic}")
        print(f"{ts} >   Item: {item}")
        print(f"{ts} >   uFmt: {uFmt}")
        print(f"{ts} >   hconv: 0x{hconv:X}" if hconv else f"{ts} >   hconv: None")
        print(f"{ts} >   hdata: 0x{hdata:X}" if hdata else f"{ts} >   hdata: None")

        # Try to extract data if there's a handle
        if hdata:
            try:
                data = get_execute_data(hdata)
                print(f"{ts} >   Data: {data}")
            except:
                pass

        print(f"{'='*70}\n")
        return DDE_FNOTPROCESSED

def main():
    global g_idInst, g_udp_socket, g_udp_host, g_udp_port

    # Parse command line arguments
    # Usage: python dde_server.py [service_name] [udp_host] [udp_port]
    service_name = sys.argv[1] if len(sys.argv) > 1 else "PathFinder"

    if len(sys.argv) > 3:
        g_udp_host = sys.argv[2]
        g_udp_port = int(sys.argv[3])
        g_udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        print(f"UDP output enabled: {g_udp_host}:{g_udp_port}")
        print("=" * 70)

    print(f"Creating DDE Server: {service_name}")
    print("=" * 70)

    # Create callback
    callback = PFNCALLBACK(dde_callback)

    # Initialize DDE as a standard server (ANSI version)
    flags = APPCLASS_STANDARD
    result = user32.DdeInitializeA(
        ctypes.byref(g_idInst),
        callback,
        flags,
        0
    )

    if result != DMLERR_NO_ERROR:
        print(f"ERROR: DdeInitialize failed with code {result}")
        sys.exit(1)

    print(f"DDE initialized (Instance ID: {g_idInst.value})")

    # Create service name string handle (ANSI, code page 0)
    hsz_service = user32.DdeCreateStringHandleA(g_idInst.value, service_name.encode('latin-1'), 0)
    if not hsz_service:
        error = user32.DdeGetLastError(g_idInst.value)
        print(f"ERROR: Failed to create service handle. Error: {error}")
        user32.DdeUninitialize(g_idInst)
        sys.exit(1)

    print(f"Service handle created: 0x{hsz_service:X}")

    # Register the service name
    result = user32.DdeNameService(g_idInst.value, hsz_service, 0, 1)  # DNS_REGISTER = 1
    if not result:
        error = user32.DdeGetLastError(g_idInst.value)
        print(f"ERROR: Failed to register service. Error: {error}")
        user32.DdeFreeStringHandle(g_idInst.value, hsz_service)
        user32.DdeUninitialize(g_idInst)
        sys.exit(1)

    print(f"Service '{service_name}' registered successfully")
    print("Waiting for DDE connections...")
    if g_udp_socket:
        print(f"Will forward callsigns to UDP {g_udp_host}:{g_udp_port}")
    print("Press Ctrl+C to exit")
    print("=" * 70)
    print()

    # Message pump
    msg = wintypes.MSG()
    p_msg = ctypes.byref(msg)
    PM_REMOVE = 0x0001

    try:
        while True:
            if user32.PeekMessageW(p_msg, None, 0, 0, PM_REMOVE):
                user32.TranslateMessage(p_msg)
                user32.DispatchMessageW(p_msg)
            else:
                ctypes.windll.kernel32.Sleep(50)
    except KeyboardInterrupt:
        print("\nShutting down DDE server...")
    finally:
        # Unregister service
        user32.DdeNameService(g_idInst.value, hsz_service, 0, 2)  # DNS_UNREGISTER = 2
        user32.DdeFreeStringHandle(g_idInst.value, hsz_service)
        user32.DdeUninitialize(g_idInst)

        # Close UDP socket
        if g_udp_socket:
            g_udp_socket.close()

        print("Cleanup complete.")

if __name__ == "__main__":
    main()

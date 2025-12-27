#!/usr/bin/env python3
"""
Simple TCP server to test port conflict handling in TR4QT.
Listens on port 14140 to simulate a port already in use.
"""

import socket
import sys

def main():
    port = 14140
    host = '127.0.0.1'

    # Create a TCP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((host, port))
        server_socket.listen(5)
        print(f"Test server listening on {host}:{port}")
        print("Press Ctrl+C to stop")

        # Keep the server running
        while True:
            # Accept connections but don't do anything with them
            client_socket, addr = server_socket.accept()
            print(f"Connection from {addr}")
            client_socket.close()

    except OSError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nShutting down test server")
    finally:
        server_socket.close()

if __name__ == "__main__":
    main()

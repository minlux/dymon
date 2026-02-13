#!/usr/bin/env python3
"""
Dymo Protocol Mock Server

Listens on port 9100 (all interfaces) and responds to Dymo status requests.
Status requests (ESC, 0x41, x) receive 32 zero bytes as response.
All other requests are silently discarded.
"""

import socket
import threading
import sys


def handle_client(client_socket, client_address):
    """Handle a single client connection."""
    print(f"[+] New connection from {client_address}")
    
    try:
        while True:
            # Receive data from client
            data = client_socket.recv(1024)
            
            if not data:
                # Client disconnected
                break
            
            # print(f"[*] Received {len(data)} bytes from {client_address}: {data.hex()}")
            
            # Check for status request pattern: ESC (0x1B), 0x41, x
            for i in range(len(data) - 2):
                if data[i] == 0x1B and data[i+1] == 0x41:
                    print(f"[*] Status request detected, sending 32 zero bytes")
                    # Send 32 zero bytes as response
                    response = bytes(32)
                    client_socket.sendall(response)
                    break
            # else:
            #     # No status request found, silently discard
            #     print(f"[*] No status request found, discarding data")
    
    except Exception as e:
        print(f"[!] Error handling client {client_address}: {e}")
    
    finally:
        print(f"[-] Connection closed: {client_address}")
        client_socket.close()


def main():
    """Start the Dymo mock server."""
    HOST = '0.0.0.0'  # Listen on all interfaces
    PORT = 9100
    
    # Create TCP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        # Bind and listen
        server_socket.bind((HOST, PORT))
        server_socket.listen(5)
        print(f"[*] Dymo Mock Server listening on {HOST}:{PORT}")
        print(f"[*] Waiting for connections...")
        
        while True:
            # Accept client connection
            client_socket, client_address = server_socket.accept()
            
            # Handle client in a separate thread
            client_thread = threading.Thread(
                target=handle_client,
                args=(client_socket, client_address),
                daemon=True
            )
            client_thread.start()
    
    except KeyboardInterrupt:
        print("\n[*] Server shutting down...")
    
    except Exception as e:
        print(f"[!] Server error: {e}")
        return 1
    
    finally:
        server_socket.close()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

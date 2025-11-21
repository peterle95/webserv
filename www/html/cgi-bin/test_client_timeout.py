import socket
import time
import sys

HOST = '127.0.0.1'
PORT = 8091

def test_timeout():
    print(f"Connecting to {HOST}:{PORT}...")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        print("Connected.")
    except Exception as e:
        print(f"Failed to connect: {e}")
        return

    print("Sleeping for 12 seconds to test timeout...")
    try:
        # Wait to see if server closes connection
        s.settimeout(12)
        start = time.time()
        while time.time() - start < 12:
            data = s.recv(1024)
            if not data:
                print("Server closed connection.")
                break
            print(f"Received data: {data}")
    except socket.timeout:
        print("No data received (timeout on recv), connection still open.")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        s.close()
        print("Test finished.")

if __name__ == "__main__":
    test_timeout()

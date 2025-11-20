#!/usr/bin/env python3
# NEW
import time
import sys

print("Content-Type: text/plain\r\n\r\n", end='')
sys.stdout.flush()

print("Starting infinite loop simulation...", flush=True)
time.sleep(35)
print("Finished sleeping (should not be reached if timeout works)", flush=True)

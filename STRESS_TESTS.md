# Webserv Stress Tests

This document contains comprehensive stress tests that evaluators can run to thoroughly test the webserv implementation.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Basic Stress Tests](#basic-stress-tests)
- [Siege Stress Tests](#siege-stress-tests)
- [Memory Leak Tests](#memory-leak-tests)
- [Connection Tests](#connection-tests)
- [CGI Stress Tests](#cgi-stress-tests)
- [Concurrent Operation Tests](#concurrent-operation-tests)
- [Edge Case Tests](#edge-case-tests)
- [Performance Benchmarks](#performance-benchmarks)

---

## Prerequisites

### Install Required Tools
```bash
# Install siege
brew install siege

# Install other useful tools
brew install htop    # For monitoring
brew install watch   # For periodic monitoring
```

### Prepare Test Environment
```bash
# Compile the server
make clean && make

# Create test files
mkdir -p www/html/test_files
dd if=/dev/zero of=www/html/test_files/1kb.bin bs=1024 count=1
dd if=/dev/zero of=www/html/test_files/10kb.bin bs=1024 count=10
dd if=/dev/zero of=www/html/test_files/100kb.bin bs=1024 count=100
dd if=/dev/zero of=www/html/test_files/1mb.bin bs=1024 count=1024

# Create simple CGI scripts for testing
mkdir -p www/cgi-bin

cat > www/cgi-bin/simple.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain\r")
print("\r")
print("Hello from CGI")
EOF

cat > www/cgi-bin/echo.py << 'EOF'
#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/plain\r")
print("\r")
print(f"Method: {os.environ.get('REQUEST_METHOD')}")

content_length = os.environ.get('CONTENT_LENGTH', '0')
if content_length and content_length != '0':
    data = sys.stdin.read(int(content_length))
    print(f"Received: {data}")
EOF

cat > www/cgi-bin/slow.py << 'EOF'
#!/usr/bin/env python3
import time
print("Content-Type: text/plain\r")
print("\r")
time.sleep(2)
print("Slow response")
EOF

chmod +x www/cgi-bin/*.py
```

---

## Basic Stress Tests

### Test 1: Simple Load Test
**Purpose:** Verify basic stability under load

```bash
siege -b -c10 -r100 http://localhost:8080/
```

**Parameters:**
- `-b`: Benchmark mode (no delays)
- `-c10`: 10 concurrent users
- `-r100`: 100 repetitions per user

**Expected Results:**
- Transactions: 1000 hits
- Availability: 100.00%
- Successful transactions: 1000
- Failed transactions: 0
- Server should NOT crash

### Test 2: Moderate Concurrency
**Purpose:** Test with moderate concurrent connections

```bash
siege -b -c50 -t60s http://localhost:8080/
```

**Parameters:**
- `-c50`: 50 concurrent users
- `-t60s`: Run for 60 seconds

**Expected Results:**
- Availability: > 99.5%
- Response time: < 1 second
- No crashes or hangs

### Test 3: High Concurrency
**Purpose:** Test server limits

```bash
siege -b -c100 -t30s http://localhost:8080/
```

**Parameters:**
- `-c100`: 100 concurrent users
- `-t30s`: Run for 30 seconds

**Expected Results:**
- Availability: > 99.0%
- Server handles gracefully
- No memory spikes

---

## Siege Stress Tests

### Test 4: Empty Page Benchmark (Critical)
**Purpose:** Meet evaluation requirement of >99.5% availability

```bash
siege -b http://localhost:8080/
```

**Expected Results:**
- Availability: > 99.5%
- Should run indefinitely without issues
- Press Ctrl+C to stop after observing stability

### Test 5: Mixed URLs Test
**Purpose:** Test different endpoints simultaneously

Create `urls.txt`:
```
http://localhost:8080/
http://localhost:8080/index.html
http://localhost:8080/test_files/1kb.bin
http://localhost:8080/test_files/10kb.bin
http://localhost:8080/test_files/100kb.bin
```

Run test:
```bash
siege -b -c25 -t60s -f urls.txt
```

**Expected Results:**
- All URLs accessible
- Availability: > 99.5%
- Even distribution of requests

### Test 6: Long Duration Test
**Purpose:** Test stability over extended period

```bash
siege -b -c20 -t300s http://localhost:8080/
```

**Parameters:**
- `-t300s`: 5 minutes

**Monitor during test:**
```bash
# In another terminal
watch -n 1 'ps aux | grep webserv'
```

**Expected Results:**
- Consistent performance throughout
- No memory growth
- No connection leaks

### Test 7: Burst Traffic Pattern
**Purpose:** Simulate sudden traffic spikes

```bash
# Run multiple siege instances simultaneously
for i in {1..5}; do
    siege -b -c20 -r50 http://localhost:8080/ &
done
wait
```

**Expected Results:**
- All requests handled
- No crashes during spike
- Quick recovery

---

## Memory Leak Tests

### Test 8: Memory Stability Check
**Purpose:** Ensure no memory leaks

**Setup monitoring:**
```bash
# Terminal 1: Start server
./webserv conf/default.conf

# Terminal 2: Get PID and monitor
PID=$(pgrep webserv)
echo "Monitoring PID: $PID"
watch -n 1 "ps -p $PID -o pid,vsz,rss,pmem,comm"
```

**Run test:**
```bash
# Terminal 3: Run extended siege
siege -b -c50 -t600s http://localhost:8080/
```

**Expected Results:**
- VSZ (Virtual Memory) stays stable
- RSS (Physical Memory) stays stable
- No continuous growth
- Memory should plateau after initial ramp-up

### Test 9: Connection Churn Test
**Purpose:** Test client connection/disconnection handling

```bash
# Rapid connect/disconnect cycles
for i in {1..1000}; do
    curl -m 1 http://localhost:8080/ > /dev/null 2>&1 &
    if [ $((i % 50)) -eq 0 ]; then
        wait
        echo "Completed $i requests"
    fi
done
wait
```

**Check memory after:**
```bash
ps aux | grep webserv
```

**Expected Results:**
- Memory returns to baseline
- No accumulation of resources

### Test 10: Large File Transfer Memory
**Purpose:** Verify no memory issues with large transfers

```bash
siege -b -c10 -r100 http://localhost:8080/test_files/1mb.bin
```

**Monitor memory during test**

**Expected Results:**
- Memory usage proportional to concurrent connections
- No runaway growth
- Proper cleanup after transfers

---

## Connection Tests

### Test 11: Connection Limit Test
**Purpose:** Test maximum concurrent connections

```bash
# Check current ulimit
ulimit -n

# Test with many connections
siege -b -c200 -t30s http://localhost:8080/
```

**Expected Results:**
- Server handles up to system limits
- Graceful degradation if limits reached
- No crashes

### Test 12: Keep-Alive Test
**Purpose:** Verify keep-alive connections work

```bash
# Test with keep-alive
curl -v -H "Connection: keep-alive" \
  http://localhost:8080/ \
  http://localhost:8080/ \
  http://localhost:8080/
```

**Expected Results:**
- Same connection reused
- Connection: keep-alive in response headers

### Test 13: Hanging Connection Detection
**Purpose:** Ensure no hanging connections

```bash
# While siege is running
lsof -i :8080 | grep webserv | wc -l

# Run siege
siege -b -c50 -t60s http://localhost:8080/

# Check again immediately after
lsof -i :8080 | grep webserv | wc -l
```

**Expected Results:**
- Connection count drops after siege stops
- No accumulation of CLOSE_WAIT states
- Clean connection management

### Test 14: Slow Client Test
**Purpose:** Test handling of slow reading clients

```bash
# Slow client simulation
(echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"; sleep 30; cat) | nc localhost 8080
```

**Run simultaneously with normal traffic:**
```bash
siege -b -c20 -t30s http://localhost:8080/
```

**Expected Results:**
- Server doesn't block on slow client
- Other clients served normally
- Slow client eventually handled or timed out

---

## CGI Stress Tests

### Test 15: CGI Concurrency
**Purpose:** Test concurrent CGI execution

```bash
siege -b -c20 -t60s http://localhost:8080/cgi-bin/simple.py
```

**Expected Results:**
- All CGI requests complete
- No CGI process leaks
- Availability > 99%

### Test 16: CGI POST Stress
**Purpose:** Test CGI with POST data

```bash
siege -b -c10 -r100 'http://localhost:8080/cgi-bin/echo.py POST data=test123'
```

**Expected Results:**
- All POST requests processed
- Correct data echoed back
- No errors

### Test 17: Mixed GET/POST CGI
**Purpose:** Test mixed CGI operations

Create `cgi_urls.txt`:
```
http://localhost:8080/cgi-bin/simple.py
http://localhost:8080/cgi-bin/echo.py POST data=test1
http://localhost:8080/cgi-bin/echo.py POST data=test2
```

```bash
siege -b -c15 -t60s -f cgi_urls.txt
```

**Expected Results:**
- Both GET and POST work
- No interference between requests

### Test 18: CGI Error Handling
**Purpose:** Verify CGI error handling doesn't crash server

Create `www/cgi-bin/error.py`:
```python
#!/usr/bin/env python3
import sys
sys.exit(1)
```

```bash
chmod +x www/cgi-bin/error.py
siege -b -c10 -r50 http://localhost:8080/cgi-bin/error.py
```

**Expected Results:**
- Returns 500 Internal Server Error
- Server continues running
- No crashes

---

## Concurrent Operation Tests

### Test 19: Simultaneous Different Methods
**Purpose:** Test GET, POST, DELETE simultaneously

```bash
# Terminal 1
siege -b -c10 -t30s http://localhost:8080/

# Terminal 2
siege -b -c10 -t30s 'http://localhost:8080/cgi-bin/echo.py POST data=test'

# Terminal 3
for i in {1..100}; do
    curl -X DELETE http://localhost:8080/uploads/test.txt 2>/dev/null &
done
```

**Expected Results:**
- All operations proceed correctly
- No interference between methods
- Server stability maintained

### Test 20: Multi-Port Stress
**Purpose:** Test multiple server blocks under load

**Config:** Use `conf/multiple_server.conf`

```bash
./webserv conf/multiple_server.conf

# Terminal 2
siege -b -c20 -t30s http://localhost:8080/

# Terminal 3
siege -b -c20 -t30s http://localhost:8081/
```

**Expected Results:**
- Both ports handle traffic
- No cross-interference
- Combined availability > 99.5%

### Test 21: Upload Stress Test
**Purpose:** Test concurrent file uploads

```bash
# Create test file
echo "test content" > /tmp/upload_test.txt

# Concurrent uploads
for i in {1..50}; do
    curl -X POST -F "file=@/tmp/upload_test.txt" \
      http://localhost:8080/uploads/ &
done
wait
```

**Expected Results:**
- All uploads complete
- No file corruption
- Proper cleanup

---

## Edge Case Tests

### Test 22: Zero-Byte File
**Purpose:** Test edge case of empty file

```bash
touch www/html/empty.txt
curl -v http://localhost:8080/empty.txt
```

**Expected Results:**
- 200 OK
- Content-Length: 0
- No crash

### Test 23: Large Header Test
**Purpose:** Test handling of large headers

```bash
curl -v -H "X-Large-Header: $(python3 -c 'print("A"*8000)')" \
  http://localhost:8080/
```

**Expected Results:**
- Either accepts and processes
- Or returns 431 Request Header Fields Too Large
- No crash

### Test 24: Malformed Requests Barrage
**Purpose:** Test resilience against bad requests

```bash
for i in {1..100}; do
    echo -e "GARBAGE DATA\r\n\r\n" | nc localhost 8080 &
    echo -e "GET /\r\n\r\n" | nc localhost 8080 &  # Missing HTTP version
    echo -e "GET / HTTP/1.1\r\n\r\n" | nc localhost 8080 &  # Missing Host
done
wait

# Verify server still works
curl http://localhost:8080/
```

**Expected Results:**
- Server handles all malformed requests
- Returns appropriate error codes
- Still serves valid requests
- No crash

### Test 25: Partial Request Test
**Purpose:** Test incomplete request handling

```bash
# Send incomplete request and hold
(echo -ne "GET / HTTP/1.1\r\nHost: localhost\r\n"; sleep 5) | nc localhost 8080
```

**Expected Results:**
- Server waits for complete request
- Eventually times out or processes
- No hang

### Test 26: Rapid Connect/Disconnect
**Purpose:** Test connection handling

```bash
for i in {1..1000}; do
    nc localhost 8080 < /dev/null &>/dev/null &
    if [ $((i % 100)) -eq 0 ]; then
        wait
    fi
done
wait
```

**Expected Results:**
- Server handles all connections
- No resource exhaustion
- No crash

---

## Performance Benchmarks

### Test 27: Throughput Benchmark
**Purpose:** Measure maximum throughput

```bash
siege -b -c50 -t60s http://localhost:8080/test_files/100kb.bin
```

**Record:**
- Transaction rate (trans/sec)
- Throughput (MB/sec)
- Response time

### Test 28: Small File Performance
**Purpose:** Measure performance on small files

```bash
siege -b -c100 -r1000 http://localhost:8080/test_files/1kb.bin
```

**Record:**
- Total transactions
- Transaction rate
- Response time

### Test 29: Static vs CGI Performance
**Purpose:** Compare static and CGI performance

```bash
# Static
siege -b -c50 -r100 http://localhost:8080/index.html

# CGI
siege -b -c50 -r100 http://localhost:8080/cgi-bin/simple.py
```

**Compare:**
- Response times
- Transaction rates
- Availability

### Test 30: Keep-Alive vs Connection Close
**Purpose:** Measure keep-alive benefit

```bash
# With keep-alive (default)
siege -b -c10 -r1000 http://localhost:8080/

# Force close (requires siege config modification)
# Or test manually with curl
```

---

## Comprehensive Stress Test Suite

### Full Test Suite Script
Save as `run_all_tests.sh`:

```bash
#!/bin/bash

echo "=== Starting Webserv Stress Test Suite ==="
echo "Date: $(date)"
echo ""

# Check if server is running
if ! curl -s http://localhost:8080/ > /dev/null; then
    echo "ERROR: Server not running on port 8080"
    exit 1
fi

echo "1. Basic load test..."
siege -b -c10 -r100 http://localhost:8080/ 2>&1 | grep -E "(Availability|Failed)"

echo "2. High concurrency test..."
siege -b -c100 -t30s http://localhost:8080/ 2>&1 | grep -E "(Availability|Failed)"

echo "3. Empty page benchmark (30s)..."
timeout 30 siege -b http://localhost:8080/ 2>&1 | grep -E "(Availability|Failed)"

echo "4. CGI stress test..."
siege -b -c20 -r50 http://localhost:8080/cgi-bin/simple.py 2>&1 | grep -E "(Availability|Failed)"

echo "5. Large file test..."
siege -b -c10 -r20 http://localhost:8080/test_files/1mb.bin 2>&1 | grep -E "(Availability|Failed)"

echo "6. Malformed requests test..."
for i in {1..20}; do
    echo -e "GARBAGE\r\n\r\n" | nc localhost 8080 &>/dev/null &
done
wait
echo "Server still responding: $(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)"

echo ""
echo "=== Test Suite Complete ==="
```

```bash
chmod +x run_all_tests.sh
./run_all_tests.sh
```

---

## Test Results Template

```
### Stress Test Results

**Date:** YYYY-MM-DD
**Server Version:** webserv v1.0
**Hardware:** [Describe CPU, RAM]
**OS:** [macOS/Linux version]

| Test | Concurrency | Duration | Transactions | Availability | Avg Response Time | Result |
|------|------------|----------|--------------|--------------|-------------------|---------|
| Basic Load | 10 | 100 req | 1000 | 100.00% | 0.01s | PASS |
| Moderate Concurrency | 50 | 60s | 45000 | 99.98% | 0.05s | PASS |
| High Concurrency | 100 | 30s | 28000 | 99.50% | 0.10s | PASS |
| Empty Page Bench | Unlimited | 60s | 75000 | 99.99% | 0.01s | PASS |
| CGI Stress | 20 | 50 req | 1000 | 99.80% | 0.15s | PASS |
| Large Files | 10 | 20 req | 200 | 100.00% | 0.25s | PASS |
| Memory Stability | 50 | 600s | 450000 | 99.95% | 0.05s | PASS |

**Memory Usage:**
- Initial: 2.5 MB
- Peak: 8.2 MB
- Final: 3.1 MB
- Leak Detected: NO

**Connection Handling:**
- Max Concurrent: 100
- Hanging Connections: 0
- CLOSE_WAIT States: 0

**Notes:**
- All tests passed successfully
- No crashes or hangs observed
- Performance consistent throughout testing
```

---

## Troubleshooting Failed Tests

### If Availability < 99.5%
1. Check for blocking I/O operations
2. Verify select() timeout is reasonable
3. Look for deadlocks in client handling
4. Check CGI process management

### If Memory Increases
1. Check client cleanup in AcceptSocket.cpp
2. Verify response buffer cleanup
3. Check for unclosed file descriptors
4. Look for CGI pipe leaks

### If Connections Hang
1. Verify timeout mechanisms
2. Check keep-alive logic
3. Review socket closure paths
4. Check for zombie CGI processes

### If Server Crashes
1. Check error handling on all recv/send
2. Verify null pointer checks
3. Review memory allocation/deallocation
4. Check for buffer overflows

---

## Advanced Testing Tools

### Using Apache Bench (alternative)
```bash
# Install
brew install apache2

# Run test
ab -n 1000 -c 50 http://localhost:8080/
```

### Using wrk (alternative)
```bash
# Install
brew install wrk

# Run test
wrk -t4 -c100 -d30s http://localhost:8080/
```

### Custom Python Stress Test
```python
#!/usr/bin/env python3
import socket
import threading
import time

def stress_test(url, port, num_requests):
    for _ in range(num_requests):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(('localhost', port))
            s.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            data = s.recv(4096)
            s.close()
        except Exception as e:
            print(f"Error: {e}")

# Run 100 threads, 10 requests each
threads = []
for i in range(100):
    t = threading.Thread(target=stress_test, args=('/', 8080, 10))
    threads.append(t)
    t.start()

for t in threads:
    t.join()
```

---

## Final Checklist for Evaluators

- [ ] Server compiles without warnings
- [ ] Basic load test passes (availability > 99.5%)
- [ ] High concurrency handled gracefully
- [ ] No memory leaks detected
- [ ] No hanging connections
- [ ] CGI stress test passes
- [ ] Malformed requests handled properly
- [ ] Server runs indefinitely under siege
- [ ] Keep-alive connections work
- [ ] Large file transfers stable
- [ ] Multiple simultaneous methods work
- [ ] Multi-port configuration works
- [ ] No crashes in any test
- [ ] Performance meets expectations
- [ ] Error handling robust

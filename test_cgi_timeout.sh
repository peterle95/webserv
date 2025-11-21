#!/bin/bash
echo "Testing CGI timeout..."
echo "Sending POST request to infinite_loop.py (sleeps 35s, server timeout 30s)"
echo "Expected result: 504 Gateway Timeout (after 30s)"

start_time=$(date +%s)
curl -v -X POST http://127.0.0.1:8080/cgi-bin/infinite_loop.py
end_time=$(date +%s)
duration=$((end_time - start_time))

echo ""
echo "Request took $duration seconds."
if [ $duration -ge 30 ]; then
    echo "Test PASSED: Request took at least 30 seconds."
else
    echo "Test FAILED: Request took less than 30 seconds."
fi

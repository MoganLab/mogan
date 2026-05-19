#!/bin/bash
set -e

FILE="/home/da/DevTeam/chapter-4.tmu"

echo "Building stem..."
xmake b stem

echo "Starting stem with $FILE..."
START=$(date +%s%N)

# Start stem in background
xmake r stem "$FILE" &
PID=$!

# Wait for process to start
sleep 5

# Kill the process
kill $PID 2>/dev/null || true
wait $PID 2>/dev/null || true

END=$(date +%s%N)
ELAPSED=$(( (END - START) / 1000000 ))
echo "Total time: ${ELAPSED}ms"

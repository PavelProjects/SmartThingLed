#!/bin/bash

HOST=192.168.1.11
PORT=9090
CONFIG=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)/cava_config

# looks bad but i don't care
function cleanup() {
  echo c\n | netcat -u $HOST $PORT &
  PID=$(echo $!)
  sleep 0.3
  kill $PID
}

trap cleanup INT

echo "Target device: $HOST:$PORT"
echo "Using config: $CONFIG"
cleanup
cava -p $CONFIG | netcat -u $HOST $PORT

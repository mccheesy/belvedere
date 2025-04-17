#!/bin/bash

# Find the belvedere process
BELVEDERE_PID=$(pgrep belvedere)

if [ -z "$BELVEDERE_PID" ]; then
    echo "Belvedere is not running. Starting it..."
    open -a belvedere
    exit 0
fi

# Send SIGHUP signal to reload configuration
echo "Reloading belvedere configuration..."
kill -HUP $BELVEDERE_PID

# Check if the reload was successful
sleep 1
if ps -p $BELVEDERE_PID > /dev/null; then
    echo "Configuration reloaded successfully."
else
    echo "Failed to reload configuration. Belvedere may have crashed."
    echo "Starting belvedere again..."
    open -a belvedere
fi
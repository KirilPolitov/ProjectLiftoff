#!/bin/bash

# Stop immediately if something fails
set -e

# Absolute path to the folder where this script lives
DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Launcher directory: $DIR"

# Add local lib folder to the library search path
export LD_LIBRARY_PATH="$DIR/lib:${LD_LIBRARY_PATH}"

echo "LD_LIBRARY_PATH set to:"
echo "$LD_LIBRARY_PATH"
echo

# Move to the executable directory (important for assets)
cd "$DIR"

echo "Starting ProjectLiftoff..."
exec "$DIR/ProjectLiftoff"

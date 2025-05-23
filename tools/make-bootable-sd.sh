#!/bin/bash

#
#  Copyright (c) 2025 R. A.  and contributors..
#  This file is part of StarshipOS, an experimental operating system.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#        https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
#

# make-bootable-sd.sh



# make-bootable-sd.sh
# Writes StarshipOS ISO to a 128GB SanDisk SD card and verifies it
# Place this script in the StarshipOS project root alongside superclean.sh

set -euo pipefail

ISO_PATH="l4/target/starshipos.iso"
DEFAULT_DEVICE="/dev/sdX"  # Replace this with actual device if you want hardcoded
BLOCK_SIZE="4M"

echo "=== StarshipOS Bootable SD Writer ==="

# Step 1: Verify ISO exists
if [[ ! -f "$ISO_PATH" ]]; then
  echo "[ERROR] ISO file not found at: $ISO_PATH"
  exit 1
fi

# Step 2: Ask user for device
read -rp "Enter the full path to the SD device (e.g., /dev/sdX): " DEVICE
if [[ ! -b "$DEVICE" ]]; then
  echo "[ERROR] Device $DEVICE not found or is not a block device."
  exit 1
fi

echo
echo "WARNING: This will erase ALL data on $DEVICE"
read -rp "Type YES to continue: " CONFIRM
if [[ "$CONFIRM" != "YES" ]]; then
  echo "Aborted."
  exit 0
fi

# Step 3: Unmount all partitions on the device
echo "[INFO] Unmounting partitions on $DEVICE..."
sudo umount "${DEVICE}"* || true

# Step 4: Write ISO to SD card
echo "[INFO] Writing ISO to $DEVICE..."
sudo dd if="$ISO_PATH" of="$DEVICE" bs=$BLOCK_SIZE status=progress conv=fsync

# Step 5: Sync and sleep
sync
echo "[INFO] Write complete. Sleeping 3s..."
sleep 3

# Step 6: Verification
echo "[INFO] Verifying write..."
ISO_SIZE=$(stat -c%s "$ISO_PATH")
DEVICE_SIZE=$(sudo dd if="$DEVICE" bs=1 count="$ISO_SIZE" 2>/dev/null | wc -c)

if [[ "$ISO_SIZE" -eq "$DEVICE_SIZE" ]]; then
  echo "[SUCCESS] Verification passed. ISO written correctly to $DEVICE."
else
  echo "[FAILURE] Verification failed. Size mismatch between ISO and device."
  echo "Expected: $ISO_SIZE bytes, Got: $DEVICE_SIZE bytes"
  exit 1
fi

echo "[DONE] You can now safely eject the SD card."


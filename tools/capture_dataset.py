#!/usr/bin/env python3
import os
import sys
import struct
import cv2
import numpy as np
import serial

PORT = "/dev/ttyACM0"
BAUD = 115200
OUTPUT_DIR = "dataset_raw"
HEADER = b"\xAA\xBB\xCC\xDD"

os.makedirs(OUTPUT_DIR, exist_ok=True)

try:
    ser = serial.Serial(PORT, BAUD, timeout=1.0)
except Exception as e:
    print(f"Error opening serial port {PORT}: {e}")
    sys.exit(1)

print(f"Connected to {PORT}.")
print("Controls:")
print("  [SPACE] - Save current frame to dataset")
print("  [Q]     - Exit")

frame_idx = len(os.listdir(OUTPUT_DIR))

def read_exact(ser: serial.Serial, num_bytes: int) -> bytes | None:
    data = bytearray()
    while len(data) < num_bytes:
        chunk = ser.read(num_bytes - len(data))
        if not chunk:
            return None
        data.extend(chunk)
    return bytes(data)

buffer = bytearray()

while True:
    byte = ser.read(1)
    if not byte:
        continue
    buffer.extend(byte)
    if len(buffer) > 4:
        buffer.pop(0)

    # Sync to frame boundary header
    if bytes(buffer) == HEADER:
        len_bytes = read_exact(ser, 4)
        if not len_bytes:
            continue
        (frame_len,) = struct.unpack("<I", len_bytes)

        # Sanity check against corrupted payload size
        if frame_len > 200000:
            continue

        jpeg_data = read_exact(ser, frame_len)
        if not jpeg_data:
            continue

        nparr = np.frombuffer(jpeg_data, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

        if frame is not None:
            # Draw center reference grid for ROI orientation
            h, w, _ = frame.shape
            display = frame.copy()
            cv2.rectangle(display, (w // 4, h // 4), (3 * w // 4, 3 * h // 4), (0, 255, 0), 1)
            cv2.putText(
                display,
                f"Saved: {frame_idx} | SPACE: Save, Q: Quit",
                (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (0, 255, 0),
                2,
            )

            cv2.imshow("EdgeFlow-S3 Collector", display)
            key = cv2.waitKey(1) & 0xFF

            if key == ord(" "):
                filename = os.path.join(OUTPUT_DIR, f"frame_{frame_idx:04d}.jpg")
                cv2.imwrite(filename, frame)
                print(f"[SAVED] {filename}")
                frame_idx += 1
            elif key == ord("q"):
                break

cv2.destroyAllWindows()
ser.close()
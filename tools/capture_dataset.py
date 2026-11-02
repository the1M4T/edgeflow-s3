#!/usr/bin/env python3
import os
import sys
import time
import struct
import cv2
import numpy as np
import serial

PORT = "/dev/ttyACM0"
BAUD = 2000000  # Match 2 Mbps UART rate configured on ESP32-S3
OUTPUT_DIR = "dataset_raw"
HEADER = b"\xAA\xBB\xCC\xDD"

os.makedirs(OUTPUT_DIR, exist_ok=True)

try:
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    ser.reset_input_buffer()
except Exception as e:
    print(f"Error opening serial port {PORT}: {e}")
    sys.exit(1)

print(f"Connected to {PORT}.")
print("Controls:")
print("  [SPACE] - Save current frame to dataset")
print("  [Q]     - Exit")

window_title = "EdgeFlow-S3 Collector"
cv2.namedWindow(window_title, cv2.WINDOW_NORMAL)
cv2.resizeWindow(window_title, 640, 480)

frame_idx = len(os.listdir(OUTPUT_DIR))
prev_time = time.time()
fps = 0.0

def decode_suppressed(jpeg_bytes: bytes):
    devnull = os.open(os.devnull, os.O_WRONLY)
    old_stderr = os.dup(2)
    os.dup2(devnull, 2)
    os.close(devnull)
    try:
        nparr = np.frombuffer(jpeg_bytes, np.uint8)
        return cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    finally:
        os.dup2(old_stderr, 2)
        os.close(old_stderr)

buffer = bytearray()

while True:
    # Read all available bytes in bulk (avoid single-byte syscall overhead)
    waiting = ser.in_waiting
    chunk = ser.read(waiting if waiting > 0 else 4096)
    if chunk:
        buffer.extend(chunk)

    # Process all complete frames currently accumulated in memory
    while True:
        idx = buffer.find(HEADER)
        if idx == -1:
            # Keep tail in case header is split across chunks
            if len(buffer) > 3:
                buffer = buffer[-3:]
            break

        # Discard preceding junk bytes
        if idx > 0:
            buffer = buffer[idx:]
            idx = 0

        # Wait until at least header (4B) and length field (4B) are present
        if len(buffer) < 8:
            break

        (frame_len,) = struct.unpack("<I", buffer[4:8])

        # Validate plausible JPEG frame size
        if frame_len < 100 or frame_len > 250000:
            buffer = buffer[4:] # Corrupt header, skip and search next
            continue

        total_frame_size = 8 + frame_len
        if len(buffer) < total_frame_size:
            # Full frame payload has not arrived yet
            break

        # Extract payload and slice remaining buffer
        jpeg_data = bytes(buffer[8:total_frame_size])
        buffer = buffer[total_frame_size:]

        frame = decode_suppressed(jpeg_data)

        if frame is not None:
            now = time.time()
            dt = now - prev_time
            prev_time = now
            if dt > 0:
                current_fps = 1.0 / dt
                fps = 0.85 * fps + 0.15 * current_fps if fps > 0 else current_fps

            h, w, _ = frame.shape
            display = frame.copy()
            cv2.rectangle(display, (w // 4, h // 4), (3 * w // 4, 3 * h // 4), (0, 255, 0), 1)

            cv2.putText(
                display,
                f"FPS: {fps:.1f} | Saved: {frame_idx}",
                (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (0, 255, 255),
                2,
            )
            cv2.putText(
                display,
                "SPACE: Save | Q: Quit",
                (10, 50),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 255, 0),
                1,
            )

            cv2.imshow(window_title, display)
            key = cv2.waitKey(1) & 0xFF

            if key == ord(" "):
                filename = os.path.join(OUTPUT_DIR, f"frame_{frame_idx:04d}.jpg")
                cv2.imwrite(filename, frame)
                print(f"[SAVED] {filename}")
                frame_idx += 1
            elif key == ord("q"):
                cv2.destroyAllWindows()
                ser.close()
                sys.exit(0)
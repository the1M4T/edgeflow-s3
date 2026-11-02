# EdgeFlow-S3

An embedded EdgeAI vehicle counting and traffic flow analysis node built on the **ESP32-S3-N16R8** SoC. The system uses quantized neural networks (`int8` with Xtensa LX7 PIE SIMD acceleration) and centroid tracking to monitor bidirectional street traffic from an elevated vantage point.

## Hardware Architecture
* **MCU:** ESP32-S3-WROOM-1-N16R8 (Dual-core Xtensa LX7 @ 240 MHz, 16 MB Quad Flash, 8 MB Octal PSRAM)
* **Optical Sensors:** 
  * **OV2640** (2 MP, DVP interface) – baseline PoC sensor
  * **OV5640** (5 MP, 68° FOV, DVP interface) – secondary evaluation sensor

## System Pipeline (PoC)
1. **Frame Acquisition (Core 0):** DVP DMA capture in `RGB565` / `GRAYSCALE` with sensor-level Region of Interest (ROI) cropping along the road axis.
2. **Inference Engine (Core 1):** Quantized `int8` FOMO (Faster Objects, More Objects) object detection accelerated via `esp-nn` / TFLite Micro.
3. **Post-Processing & Tracking (Core 0):** Inter-frame centroid matching, vector direction validation, and virtual tripwire crossing counter.

## Project Roadmap
- [x] **Stage 1: Hardware Bring-up & Data Collector** – DVP initialization, Octal PSRAM verification, raw frame capture for scene-specific dataset building.
- [ ] **Stage 2: Dataset Curation & TinyML Training** – Training and `int8` quantization of a custom FOMO model on street-level ROI frames.
- [ ] **Stage 3: Dual-Core Edge Inference & Tracker** – FreeRTOS pipeline isolating inference on Core 1 and centroid tracking on Core 0.
- [ ] **Stage 4: Sensor & Model Benchmarking** – Latency, FPS, accuracy, and power profiling (OV2640 vs. OV5640).

## Build & Flash (ESP-IDF v5.x)
```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
#include <cstdio>
#include <cstdint>
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "camera_pins.hpp"

namespace {
constexpr const char* kTag = "DataCollector";
constexpr uint8_t kFrameHeader[4] = {0xAA, 0xBB, 0xCC, 0xDD};

esp_err_t init_camera_jpeg() {
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = edgeflow::hw::kPinD0;
    config.pin_d1       = edgeflow::hw::kPinD1;
    config.pin_d2       = edgeflow::hw::kPinD2;
    config.pin_d3       = edgeflow::hw::kPinD3;
    config.pin_d4       = edgeflow::hw::kPinD4;
    config.pin_d5       = edgeflow::hw::kPinD5;
    config.pin_d6       = edgeflow::hw::kPinD6;
    config.pin_d7       = edgeflow::hw::kPinD7;
    config.pin_xclk     = edgeflow::hw::kPinXclk;
    config.pin_pclk     = edgeflow::hw::kPinPclk;
    config.pin_vsync    = edgeflow::hw::kPinVsync;
    config.pin_href     = edgeflow::hw::kPinHref;
    config.pin_sccb_sda = edgeflow::hw::kPinSiod;
    config.pin_sccb_scl = edgeflow::hw::kPinSioc;
    config.pin_pwdn     = edgeflow::hw::kPinPwdn;
    config.pin_reset    = edgeflow::hw::kPinReset;
    
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = FRAMESIZE_VGA; // 640x480 resolution for precise framing and ROI selection
    config.jpeg_quality = 10;            // 1-63, lower values mean higher image fidelity
    config.fb_count     = 2;             // Double buffering in PSRAM
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;

    return esp_camera_init(&config);
}
} // namespace

extern "C" void app_main(void) {
    if (init_camera_jpeg() != ESP_OK) {
        ESP_LOGE(kTag, "Camera initialization failed");
        return;
    }

    sensor_t* s = esp_camera_sensor_get();
    // Slightly boost sharpness to enhance edge boundaries for vehicle detection
    s->set_sharpness(s, 1);

    while (true) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            uint32_t len = fb->len;
            // Transmit sync header and payload over serial stdout (UART / USB CDC)
            fwrite(kFrameHeader, 1, sizeof(kFrameHeader), stdout);
            fwrite(&len, 1, sizeof(len), stdout);
            fwrite(fb->buf, 1, len, stdout);
            fflush(stdout);

            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Target ~20 FPS acquisition rate
    }
}
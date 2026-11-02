#include <cstdio>
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "camera_pins.hpp"

namespace {
constexpr const char* kTag = "EdgeFlow_BringUp";

esp_err_t init_camera() {
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
    
    config.xclk_freq_hz = 20000000; // 20 MHz XCLK
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size   = FRAMESIZE_QVGA; // 320x240 bazowo pod wycięcie ROI
    config.jpeg_quality = 12;
    config.fb_count     = 2; // Podwójne buforowanie w PSRAM
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;

    return esp_camera_init(&config);
}
} // namespace

extern "C" void app_main(void) {
    ESP_LOGI(kTag, "Starting EdgeFlow-S3 Hardware Bring-up...");

    size_t psram_size = esp_psram_get_size();
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(kTag, "PSRAM Total: %zu KB | Free: %zu KB", psram_size / 1024, free_psram / 1024);

    if (init_camera() != ESP_OK) {
        ESP_LOGE(kTag, "Camera initialization failed! Check FPC ribbon orientation.");
        return;
    }

    sensor_t* s = esp_camera_sensor_get();
    ESP_LOGI(kTag, "Camera initialized successfully. Sensor PID: 0x%02x", s->id.PID);

    while (true) {
        int64_t t_start = esp_timer_get_time();
        camera_fb_t* fb = esp_camera_fb_get();
        int64_t t_end = esp_timer_get_time();

        if (!fb) {
            ESP_LOGE(kTag, "Frame buffer capture failed");
        } else {
            // Obliczamy średnią jasność klatki jako szybki test dostępu do bufora w PSRAM
            uint64_t sum = 0;
            for (size_t i = 0; i < fb->len; ++i) {
                sum += fb->buf[i];
            }
            uint8_t avg_lum = static_cast<uint8_t>(sum / fb->len);

            ESP_LOGI(kTag, "Frame %zux%zu (%zu B) | Capture: %lld us | Avg Luminance: %u/255",
                     fb->width, fb->height, fb->len, (t_end - t_start), avg_lum);

            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
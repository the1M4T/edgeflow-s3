#pragma once

namespace edgeflow::hw {

constexpr int kPinPwdn  = -1;
constexpr int kPinReset = -1;
constexpr int kPinXclk  = 15;
constexpr int kPinSiod  = 4;
constexpr int kPinSioc  = 5;

constexpr int kPinD7    = 16; // Y9
constexpr int kPinD6    = 17; // Y8
constexpr int kPinD5    = 18; // Y7
constexpr int kPinD4    = 12; // Y6
constexpr int kPinD3    = 10; // Y5
constexpr int kPinD2    = 8;  // Y4
constexpr int kPinD1    = 9;  // Y3
constexpr int kPinD0    = 11; // Y2

constexpr int kPinVsync = 6;
constexpr int kPinHref  = 7;
constexpr int kPinPclk  = 13;

} // namespace edgeflow::hw
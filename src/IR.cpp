#include "IR.h"

IR::IR(uint8_t pin, uint16_t threshold)
    : _pin(pin), _threshold(threshold) {}

void IR::begin() {
    // GPIO15 thuộc ADC2 trên ESP32 — lưu ý: ADC2 không dùng được khi WiFi đang hoạt động
    pinMode(_pin, INPUT);
}

void IR::enablePWM(uint8_t pwmPin, uint8_t channel, uint32_t freq, uint8_t bits) {
    _pwmPin     = pwmPin;
    _pwmChannel = channel;
    _pwmBits    = bits;
    ledcSetup(channel, freq, bits);
    ledcAttachPin(pwmPin, channel);
    _pwmEnabled = true;
}

void IR::disablePWM() {
    if (_pwmEnabled) {
        ledcWrite(_pwmChannel, 0);
        ledcDetachPin(_pwmPin);
        _pwmEnabled = false;
    }
}

void IR::_applyPWM(int adcValue) {
    // Ánh xạ ADC 12-bit (0-4095) → PWM duty theo độ phân giải đã cấu hình
    uint32_t maxDuty = (1u << _pwmBits) - 1;
    _pwmDuty = map(adcValue, 0, 4095, 0, maxDuty);
    ledcWrite(_pwmChannel, _pwmDuty);
}

int IR::readRaw() {
    return analogRead(_pin);
}

int IR::readFiltered() {
    long sum = 0;
    for (int i = 0; i < IR_SAMPLE_COUNT; i++) {
        sum += analogRead(_pin);
        delayMicroseconds(200);
    }
    int value = (int)(sum / IR_SAMPLE_COUNT);
    if (_pwmEnabled) _applyPWM(value);
    return value;
}

bool IR::isDetected() {
    // LED thu hồng ngoại: khi nhận được tín hiệu IR, điện áp giảm → ADC giảm
    return readFiltered() < _threshold;
}

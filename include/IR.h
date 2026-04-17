#pragma once
#include <Arduino.h>

#define IR_PIN          34      // Chân analog đọc tín hiệu IR (GPIO34 / ADC1_CH6, input-only)
#define IR_THRESHOLD    2000    // Ngưỡng ADC (0-4095): dưới ngưỡng = có tín hiệu IR
#define IR_SAMPLE_COUNT 5       // Số lần lấy mẫu để lọc nhiễu

#define IR_PWM_PIN        2     // Chân PWM output mặc định
#define IR_PWM_CHANNEL    0     // LEDC channel (0-15)
#define IR_PWM_FREQ       5000  // Tần số PWM (Hz)
#define IR_PWM_RESOLUTION 8     // Độ phân giải PWM (bit): duty 0-255

class IR {
public:
    IR(uint8_t pin = IR_PIN, uint16_t threshold = IR_THRESHOLD);

    void begin();

    // Kích hoạt PWM output trên pwmPin, ánh xạ giá trị ADC → duty cycle
    void enablePWM(uint8_t pwmPin = IR_PWM_PIN,
                   uint8_t channel = IR_PWM_CHANNEL,
                   uint32_t freq   = IR_PWM_FREQ,
                   uint8_t  bits   = IR_PWM_RESOLUTION);

    // Tắt PWM output
    void disablePWM();

    // Đọc giá trị ADC thô (0 - 4095)
    int readRaw();

    // Đọc giá trị ADC trung bình sau khi lọc nhiễu; tự động cập nhật PWM nếu đã bật
    int readFiltered();

    // Trả về true nếu phát hiện có tín hiệu hồng ngoại
    bool isDetected();

    // Lấy duty cycle PWM hiện tại (0 - (2^bits - 1))
    uint32_t getPWMDuty() const { return _pwmDuty; }

    // Lấy ngưỡng hiện tại
    uint16_t getThreshold() const { return _threshold; }

    // Đặt lại ngưỡng
    void setThreshold(uint16_t threshold) { _threshold = threshold; }

private:
    uint8_t  _pin;
    uint16_t _threshold;

    bool     _pwmEnabled  = false;
    uint8_t  _pwmPin      = IR_PWM_PIN;
    uint8_t  _pwmChannel  = IR_PWM_CHANNEL;
    uint8_t  _pwmBits     = IR_PWM_RESOLUTION;
    uint32_t _pwmDuty     = 0;

    void _applyPWM(int adcValue);
};

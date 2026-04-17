#include <Arduino.h>
#include "IR.h"
#include "BLE.h"

IR ir(IR_PIN, IR_THRESHOLD);
BLEWiFiProvisioner& ble = BLEWiFiProvisioner::instance();

void setup() {
    Serial.begin(115200);

    ir.begin();
    ir.enablePWM(IR_PWM_PIN, IR_PWM_CHANNEL, IR_PWM_FREQ, IR_PWM_RESOLUTION);
    Serial.printf("IR ready — ADC pin: %d | PWM pin: %d\n", IR_PIN, IR_PWM_PIN);

    ble.begin(BLE_DEVICE_NAME);
}

void loop() {
    ble.update();

    int  filtered  = ir.readFiltered();
    bool detected  = ir.isDetected();
    bool wifiConn  = ble.isWiFiConnected();

    static int  prevFiltered = -1;
    static bool prevDetected = false;
    static bool prevWifi     = false;

    if (filtered != prevFiltered || detected != prevDetected || wifiConn != prevWifi) {
        prevFiltered = filtered;
        prevDetected = detected;
        prevWifi     = wifiConn;

        Serial.printf("ADC: %4d | PWM: %3d | IR: %s | WiFi: %s\n",
                      filtered,
                      ir.getPWMDuty(),
                      detected ? "DETECTED" : "none",
                      wifiConn ? ble.getIP().c_str() : "disconnected");
    }

    delay(50);
}

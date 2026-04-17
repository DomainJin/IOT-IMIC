#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define WIFI_CONFIG_SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define WIFI_SSID_CHAR_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define WIFI_PASS_CHAR_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define WIFI_STATUS_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define WIFI_CMD_CHAR_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define BLE_DEVICE_NAME           "IMIC-Config"

class BLEWiFiProvisioner {
public:
    static BLEWiFiProvisioner& instance();

    // Khởi tạo BLE server và bắt đầu advertising
    void begin(const char* name = BLE_DEVICE_NAME);

    // Gọi trong loop() để xử lý kết nối WiFi pending
    void update();

    bool isClientConnected() const { return _connected; }
    bool isWiFiConnected()   const;
    String getIP()           const;

    // Được gọi bởi BLE callbacks (public để callback truy cập)
    void onClientConnect();
    void onClientDisconnect();
    void onSSIDWrite(const String& ssid);
    void onPassWrite(const String& pass);
    void onCmdWrite(const String& cmd);

private:
    BLEWiFiProvisioner() = default;

    bool     _connected      = false;
    bool     _connectPending = false;
    String   _pendingSSID;
    String   _pendingPass;

    BLEServer*         _server     = nullptr;
    BLECharacteristic* _statusChar = nullptr;

    void _doConnectWiFi();
    void _setStatus(const String& msg);
};

#include "BLE.h"
#include <WiFi.h>

// ── Singleton ──────────────────────────────────────────────────────────────
BLEWiFiProvisioner& BLEWiFiProvisioner::instance() {
    static BLEWiFiProvisioner inst;
    return inst;
}

// ── Callbacks ──────────────────────────────────────────────────────────────
class _ServerCB : public BLEServerCallbacks {
    void onConnect(BLEServer*)    override { BLEWiFiProvisioner::instance().onClientConnect(); }
    void onDisconnect(BLEServer*) override { BLEWiFiProvisioner::instance().onClientDisconnect(); }
};

class _CharCB : public BLECharacteristicCallbacks {
public:
    enum Type { SSID, PASS, CMD };
    explicit _CharCB(Type t) : _type(t) {}
    void onWrite(BLECharacteristic* c) override {
        String val = c->getValue().c_str();
        auto& p = BLEWiFiProvisioner::instance();
        switch (_type) {
            case SSID: p.onSSIDWrite(val); break;
            case PASS: p.onPassWrite(val); break;
            case CMD:  p.onCmdWrite(val);  break;
        }
    }
private:
    Type _type;
};

// ── begin ──────────────────────────────────────────────────────────────────
void BLEWiFiProvisioner::begin(const char* name) {
    BLEDevice::init(name);
    _server = BLEDevice::createServer();
    _server->setCallbacks(new _ServerCB());

    BLEService* svc = _server->createService(WIFI_CONFIG_SERVICE_UUID);

    auto* ssidChar = svc->createCharacteristic(WIFI_SSID_CHAR_UUID,
                         BLECharacteristic::PROPERTY_WRITE);
    ssidChar->setCallbacks(new _CharCB(_CharCB::SSID));

    auto* passChar = svc->createCharacteristic(WIFI_PASS_CHAR_UUID,
                         BLECharacteristic::PROPERTY_WRITE);
    passChar->setCallbacks(new _CharCB(_CharCB::PASS));

    _statusChar = svc->createCharacteristic(WIFI_STATUS_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY);
    _statusChar->addDescriptor(new BLE2902());
    _statusChar->setValue("idle");

    auto* cmdChar = svc->createCharacteristic(WIFI_CMD_CHAR_UUID,
                        BLECharacteristic::PROPERTY_WRITE);
    cmdChar->setCallbacks(new _CharCB(_CharCB::CMD));

    svc->start();

    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(WIFI_CONFIG_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);  // giúp iOS nhận diện thiết bị
    adv->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf("[BLE] Advertising as \"%s\"\n", name);
}

// ── update (gọi trong loop) ────────────────────────────────────────────────
void BLEWiFiProvisioner::update() {
    if (_connectPending) {
        _connectPending = false;
        _doConnectWiFi();
    }
}

// ── client callbacks ───────────────────────────────────────────────────────
void BLEWiFiProvisioner::onClientConnect() {
    _connected = true;
    Serial.println("[BLE] Client connected");
}

void BLEWiFiProvisioner::onClientDisconnect() {
    _connected = false;
    Serial.println("[BLE] Client disconnected — restarting advertising");
    BLEDevice::startAdvertising();
}

// ── char callbacks ─────────────────────────────────────────────────────────
void BLEWiFiProvisioner::onSSIDWrite(const String& s) {
    _pendingSSID = s;
    Serial.println("[BLE] SSID received");
}

void BLEWiFiProvisioner::onPassWrite(const String& s) {
    _pendingPass = s;
    Serial.println("[BLE] Password received");
}

void BLEWiFiProvisioner::onCmdWrite(const String& cmd) {
    if (cmd == "1") {
        Serial.println("[BLE] Connect command received");
        _connectPending = true;
    }
}

// ── WiFi ───────────────────────────────────────────────────────────────────
bool BLEWiFiProvisioner::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String BLEWiFiProvisioner::getIP() const {
    return WiFi.localIP().toString();
}

void BLEWiFiProvisioner::_setStatus(const String& msg) {
    Serial.println("[BLE] status: " + msg);
    if (_statusChar) {
        _statusChar->setValue(msg.c_str());
        _statusChar->notify();
    }
}

void BLEWiFiProvisioner::_doConnectWiFi() {
    if (_pendingSSID.isEmpty()) {
        _setStatus("err:no_ssid");
        return;
    }
    _setStatus("connecting:" + _pendingSSID);
    WiFi.disconnect(true);
    WiFi.begin(_pendingSSID.c_str(), _pendingPass.c_str());

    uint8_t tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(500);
        tries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        _setStatus("ok:" + WiFi.localIP().toString());
        Serial.println("[WiFi] Connected: " + WiFi.localIP().toString());
    } else {
        WiFi.disconnect(true);
        _setStatus("err:failed");
        Serial.println("[WiFi] Connection failed");
    }
}

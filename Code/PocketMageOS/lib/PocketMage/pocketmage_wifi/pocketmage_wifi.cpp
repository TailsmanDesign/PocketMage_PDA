#include "pocketmage_wifi.h"

#include <pocketmage_i18n/pocketmage_i18n.h>

#include <esp_log.h>
#include <esp_task_wdt.h>

#include <cstring>

static const char* TAG = "PocketMageWifi";

PocketMageWifi& PocketMageWifi::getInstance() {
  static PocketMageWifi instance;
  return instance;
}

PocketMageWifi& P_WIFI = PocketMageWifi::getInstance();

PocketMageWifi::PocketMageWifi()
    : _mutex(xSemaphoreCreateRecursiveMutex()),
      _state(WifiRadioState::Off),
      _stateBeforeScan(WifiRadioState::Off),
      _scanResults(nullptr),
      _scanResultCount(0),
      _taskHandle(nullptr),
      _commandQueue(nullptr),
      _shutdownSem(xSemaphoreCreateBinary()),
      _staNetif(nullptr),
      _wifiEventHandler(nullptr),
      _ipEventHandler(nullptr),
      _initialized(false),
      _autoConnectEnabled(true),
      _retryCount(0),
      _retryAt(0),
      _eventCallback(nullptr),
      _eventPending(false) {
  _statusMessage[0] = 0;
  _connectedSSID[0] = 0;
  _ipAddress[0] = 0;
  _pendingSSID[0] = 0;
  _pendingPassword[0] = 0;
  _pendingSave = false;
  _retrySSID[0] = 0;
  _retryPassword[0] = 0;
  _connectError[0] = 0;
}

PocketMageWifi::~PocketMageWifi() {
  stop();
  if (_mutex)
    vSemaphoreDelete(_mutex);
  if (_shutdownSem)
    vSemaphoreDelete(_shutdownSem);
}

void PocketMageWifi::begin() {
  if (_initialized)
    return;
  // Raw esp_wifi_* calls post events to the default ESP event loop, which
  // Arduino only creates lazily when its own WiFi lib is used. Without it,
  // scan/connect succeed but WIFI_EVENT_* never arrive, so create it here.
  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    ESP_LOGE(TAG, "begin(): esp_event_loop_create_default -> %s", esp_err_to_name(err));
  err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    ESP_LOGE(TAG, "begin(): esp_netif_init -> %s", esp_err_to_name(err));
  
  _commandQueue = xQueueCreate(8, sizeof(Command));
  // Stack must cover esp_wifi_init (needs ~7KB) plus esp_wifi_scan_start.
  xTaskCreatePinnedToCore(wifiTaskFunc, "pmwifi", 10240, this, 2, &_taskHandle, 0);  // Pin to core 0
  _initialized = true;
  ESP_LOGI(TAG, "begin(): pmwifi task created");
}

void PocketMageWifi::stop() {
  if (_taskHandle) {
    Command cmd = Command::Shutdown;
    xQueueSend(_commandQueue, &cmd, portMAX_DELAY);
    xSemaphoreTake(_shutdownSem, pdMS_TO_TICKS(5000));
    _taskHandle = nullptr;
  }
  if (_commandQueue) {
    vQueueDelete(_commandQueue);
    _commandQueue = nullptr;
  }
  if (_scanResults) {
    free(_scanResults);
    _scanResults = nullptr;
  }
  _initialized = false;
}

void PocketMageWifi::enable() {
  Command cmd = Command::Enable;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::disable() {
  Command cmd = Command::Disable;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::scan() {
  Command cmd = Command::Scan;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::connect(const char* ssid, const char* password, bool save) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_pendingSSID, ssid, sizeof(_pendingSSID));
  _pendingSSID[sizeof(_pendingSSID) - 1] = 0;
  strncpy(_pendingPassword, password, sizeof(_pendingPassword));
  _pendingPassword[sizeof(_pendingPassword) - 1] = 0;
  _pendingSave = save;
  xSemaphoreGiveRecursive(_mutex);
  Command cmd = Command::Connect;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::disconnect() {
  Command cmd = Command::Disconnect;
  xQueueSend(_commandQueue, &cmd, 0);
}

void PocketMageWifi::reconnect() {
  Command cmd = Command::Reconnect;
  xQueueSend(_commandQueue, &cmd, 0);
}

WifiRadioState PocketMageWifi::getState() const {
  return _state;
}

bool PocketMageWifi::isConnected() const {
  return _state == WifiRadioState::Connected;
}

bool PocketMageWifi::isScanning() const {
  return _state == WifiRadioState::Scanning;
}

String PocketMageWifi::getStatusMessage() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String msg = String(_statusMessage);
  xSemaphoreGiveRecursive(_mutex);
  return msg;
}

String PocketMageWifi::getConnectedSSID() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String ssid = String(_connectedSSID);
  xSemaphoreGiveRecursive(_mutex);
  return ssid;
}

String PocketMageWifi::getIpAddress() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String ip = String(_ipAddress);
  xSemaphoreGiveRecursive(_mutex);
  return ip;
}

int PocketMageWifi::getRssi() const {
  wifi_ap_record_t info;
  if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
    return info.rssi;
  }
  return 0;
}

String PocketMageWifi::getLastError() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String err = String(_connectError);
  xSemaphoreGiveRecursive(_mutex);
  return err;
}

uint16_t PocketMageWifi::getScanResultCount() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  uint16_t count = _scanResultCount;
  xSemaphoreGiveRecursive(_mutex);
  return count;
}

bool PocketMageWifi::getScanResult(uint16_t index, WifiApInfo& out) const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool ok = false;
  if (index < _scanResultCount && _scanResults) {
    strncpy(out.ssid, (const char*)_scanResults[index].ssid, sizeof(out.ssid));
    out.ssid[sizeof(out.ssid) - 1] = 0;
    out.rssi = _scanResults[index].rssi;
    out.channel = _scanResults[index].primary;
    out.authmode = _scanResults[index].authmode;
    ok = true;
  }
  xSemaphoreGiveRecursive(_mutex);
  return ok;
}

bool PocketMageWifi::hasSavedCredentials(const char* ssid) const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool found = false;
  if (_prefs.begin(PREFS_NAMESPACE, true)) {
    found = _prefs.isKey(ssid);
    _prefs.end();
  }
  xSemaphoreGiveRecursive(_mutex);
  return found;
}

bool PocketMageWifi::loadSavedCredentials(const char* ssid, char* password, size_t maxLen) const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool ok = false;
  if (_prefs.begin(PREFS_NAMESPACE, true)) {
    String pass = _prefs.getString(ssid, "");
    _prefs.end();
    if (pass.length() > 0) {
      strncpy(password, pass.c_str(), maxLen);
      password[maxLen - 1] = 0;
      ok = true;
    }
  }
  xSemaphoreGiveRecursive(_mutex);
  return ok;
}

void PocketMageWifi::clearSavedCredentials(const char* ssid) {
  if (_prefs.begin(PREFS_NAMESPACE, false)) {
    _prefs.remove(ssid);
    _prefs.end();
  }
}

void PocketMageWifi::setEventCallback(WifiEventCallback cb) {
  _eventCallback = cb;
}

void PocketMageWifi::wifiTaskFunc(void* param) {
  static_cast<PocketMageWifi*>(param)->taskLoop();
}

void PocketMageWifi::taskLoop() {
  Command cmd = Command::None;
  unsigned long lastAutoScan = 0;
  setStatus(TR(STR_WIFI_IDLE));
  while (true) {
    esp_task_wdt_reset();  // Reset watchdog
    // Wait for command or periodic auto-scan
    if (xQueueReceive(_commandQueue, &cmd, pdMS_TO_TICKS(200)) == pdTRUE) {
      switch (cmd) {
        case Command::Enable:
          doEnable();
          break;
        case Command::Disable:
          doDisable();
          break;
        case Command::Scan:
          doScan();
          break;
        case Command::Connect:
          doConnect();
          break;
        case Command::Disconnect:
          doDisconnect();
          break;
        case Command::Reconnect:
          doAutoConnect();
          break;
        case Command::CheckAutoConnect:
          doAutoConnect();
          break;
        case Command::Shutdown:
          if (_state != WifiRadioState::Off && _state != WifiRadioState::TurningOff) {
            esp_wifi_disconnect();
            esp_wifi_stop();
            esp_wifi_deinit();
            if (_staNetif) {
              esp_netif_destroy(_staNetif);
              _staNetif = nullptr;
            }
            if (_wifiEventHandler) {
              esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _wifiEventHandler);
              _wifiEventHandler = nullptr;
            }
            if (_ipEventHandler) {
              esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, _ipEventHandler);
              _ipEventHandler = nullptr;
            }
            _state = WifiRadioState::Off;
          }
          xSemaphoreGive(_shutdownSem);
          vTaskDelete(NULL);
          return;
        default:
          break;
      }
    }
    // Auto-scan/auto-connect if enabled
    if (_autoConnectEnabled && _state == WifiRadioState::On) {
      unsigned long now = millis();
      if (now - lastAutoScan > AUTO_SCAN_INTERVAL) {
        lastAutoScan = now;
        doScan();
        doAutoConnect();
      }
    }
    // Check for pending retry
    if (_retryCount > 0 && _retryCount <= MAX_RETRIES && _retryAt > 0) {
      unsigned long now = millis();
      if (now >= _retryAt) {
        _retryAt = 0;
        xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
        strncpy(_pendingSSID, _retrySSID, sizeof(_pendingSSID));
        _pendingSSID[sizeof(_pendingSSID) - 1] = 0;
        strncpy(_pendingPassword, _retryPassword, sizeof(_pendingPassword));
        _pendingPassword[sizeof(_pendingPassword) - 1] = 0;
        _pendingSave = false;
        xSemaphoreGiveRecursive(_mutex);
        doConnect();
      }
    }
    vTaskDelay(10);
  }
}

void PocketMageWifi::espEventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) {
  PocketMageWifi* self = static_cast<PocketMageWifi*>(arg);
  if (base == WIFI_EVENT)
    self->handleWifiEvent(id, data);
  else if (base == IP_EVENT)
    self->handleIpEvent(id, data);
}

void PocketMageWifi::handleWifiEvent(int32_t id, void* data) {
  switch (id) {
    case WIFI_EVENT_STA_START:
      setStatus(TR(STR_WIFI_STARTED));
      break;
    case WIFI_EVENT_STA_CONNECTED: {
      auto* connected = static_cast<wifi_event_sta_connected_t*>(data);
      setStatus(TR(STR_WIFI_CONNECTED));
      _connectError[0] = 0;
      _retryCount = 0;
      size_t len = connected->ssid_len;
      if (len > sizeof(_connectedSSID) - 1)
        len = sizeof(_connectedSSID) - 1;
      memcpy(_connectedSSID, connected->ssid, len);
      _connectedSSID[len] = 0;
      _state = WifiRadioState::Connected;
      publishEvent();
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      auto* disconn = static_cast<wifi_event_sta_disconnected_t*>(data);
      _state = WifiRadioState::On;
      switch (disconn->reason) {
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
          _retryCount = MAX_RETRIES;
          strncpy(_connectError, TR(STR_WIFI_AUTH_FAILED), sizeof(_connectError) - 1);
          _connectError[sizeof(_connectError) - 1] = 0;
          setStatus(TR(STR_WIFI_AUTH_FAILED));
          break;
        case WIFI_REASON_NO_AP_FOUND:
          _retryCount = MAX_RETRIES;
          strncpy(_connectError, TR(STR_WIFI_NETWORK_NOT_FOUND), sizeof(_connectError) - 1);
          _connectError[sizeof(_connectError) - 1] = 0;
          setStatus(TR(STR_WIFI_NETWORK_NOT_FOUND));
          break;
        default:
          if (_retryCount < MAX_RETRIES) {
            unsigned long delay = RETRY_BASE_DELAY_MS << _retryCount;
            if (delay > RETRY_MAX_DELAY_MS)
              delay = RETRY_MAX_DELAY_MS;
            _retryAt = millis() + delay;
            _retryCount++;
            char buf[48];
            snprintf(buf, sizeof(buf), TR(STR_WIFI_RECONNECTING), delay);
            setStatus(buf);
          } else {
            strncpy(_connectError, TR(STR_WIFI_MAX_RETRIES), sizeof(_connectError) - 1);
            _connectError[sizeof(_connectError) - 1] = 0;
            setStatus(TR(STR_WIFI_CONN_FAILED));
          }
          break;
      }
      publishEvent();
      break;
    }
    case WIFI_EVENT_SCAN_DONE:
      setStatus(TR(STR_WIFI_SCAN_DONE));
      {
        uint16_t num = 0;
        esp_wifi_scan_get_ap_num(&num);
        ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE: %u APs found", (unsigned)num);
        xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
        if (_scanResults) {
          free(_scanResults);
          _scanResults = nullptr;
        }
        if (num > MAX_SCAN_RESULTS)
          num = MAX_SCAN_RESULTS;
        _scanResults = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * MAX_SCAN_RESULTS);
        if (_scanResults) {
          esp_wifi_scan_get_ap_records(&num, _scanResults);
          _scanResultCount = num;
        } else {
          _scanResultCount = 0;
        }
        xSemaphoreGiveRecursive(_mutex);
      }
      // esp_wifi stays associated across a scan; restore a pre-scan
      // Connected state instead of dropping the link back to On, otherwise
      // isConnected() reports false until STA_CONNECTED refires (it won't).
      _state = (_stateBeforeScan == WifiRadioState::Connected) ? WifiRadioState::Connected
                                                               : WifiRadioState::On;
      publishEvent();
      break;
    default:
      break;
  }
}

void PocketMageWifi::handleIpEvent(int32_t id, void* data) {
  if (id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)data;
    snprintf(_ipAddress, sizeof(_ipAddress), "%d.%d.%d.%d", IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: %s", _ipAddress);
    setStatus(TR(STR_WIFI_GOT_IP));
    _state = WifiRadioState::Connected;
    publishEvent();
  }
}

void PocketMageWifi::doEnable() {
  ESP_LOGI(TAG, "doEnable(): state=%d", (int)_state);
  // Radio must run at 240MHz; raise the CPU before touching esp_wifi.
  if (getCpuFrequencyMhz() != WIFI_CPU_FREQ_MHZ)
    setCpuFrequencyMhz(WIFI_CPU_FREQ_MHZ);
  if (_state == WifiRadioState::Off || _state == WifiRadioState::TurningOff) {
    _state = WifiRadioState::TurningOn;
    setStatus(TR(STR_WIFI_ENABLING));
    if (_staNetif)
      esp_netif_destroy(_staNetif);
    _staNetif = esp_netif_create_default_wifi_sta();
    if (!_staNetif) {
      setStatus(TR(STR_WIFI_CREATE_NETIF_FAILED));
      ESP_LOGE(TAG, "doEnable(): esp_netif_create_default_wifi_sta failed");
      _state = WifiRadioState::Off;
      return;
    }
    ESP_LOGI(TAG, "doEnable(): netif created");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    ESP_LOGI(TAG, "doEnable(): esp_wifi_init -> %s", esp_err_to_name(err));
    if (err != ESP_OK) {
      setStatus(TR(STR_WIFI_INIT_FAILED));
      _state = WifiRadioState::Off;
      return;
    }
    esp_err_t regErr = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &PocketMageWifi::espEventHandler, this, &_wifiEventHandler);
    if (regErr != ESP_OK)
      ESP_LOGE(TAG, "doEnable(): register WIFI_EVENT -> %s", esp_err_to_name(regErr));
    regErr = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &PocketMageWifi::espEventHandler, this, &_ipEventHandler);
    if (regErr != ESP_OK)
      ESP_LOGE(TAG, "doEnable(): register IP_EVENT -> %s", esp_err_to_name(regErr));
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_LOGI(TAG, "doEnable(): esp_wifi_set_mode -> %s", esp_err_to_name(err));
    if (err != ESP_OK) {
      setStatus(TR(STR_WIFI_SET_MODE_FAILED));
      _state = WifiRadioState::Off;
      return;
    }
    err = esp_wifi_start();
    ESP_LOGI(TAG, "doEnable(): esp_wifi_start -> %s", esp_err_to_name(err));
    if (err != ESP_OK) {
      setStatus(TR(STR_WIFI_START_FAILED));
      _state = WifiRadioState::Off;
      return;
    }
    _state = WifiRadioState::On;
    setStatus(TR(STR_WIFI_ENABLED));
    publishEvent();
  }
  ESP_LOGI(TAG, "doEnable(): done, state=%d", (int)_state);
}

void PocketMageWifi::doDisable() {
  if (_state != WifiRadioState::Off && _state != WifiRadioState::TurningOff) {
    _state = WifiRadioState::TurningOff;
    setStatus(TR(STR_WIFI_DISABLING));
    esp_wifi_stop();
    esp_wifi_deinit();
    if (_staNetif) {
      esp_netif_destroy(_staNetif);
      _staNetif = nullptr;
    }
    _state = WifiRadioState::Off;
    setStatus(TR(STR_WIFI_DISABLED));
    publishEvent();
  }
}

void PocketMageWifi::doScan() {
  ESP_LOGI(TAG, "doScan(): state=%d", (int)_state);
  // remember the prior state
  if (_state == WifiRadioState::On || _state == WifiRadioState::Connected) {
    _stateBeforeScan = _state;
    _state = WifiRadioState::Scanning;
    setStatus(TR(STR_WIFI_SCANNING));
    wifi_scan_config_t scanConf = {};
    scanConf.ssid = nullptr;
    scanConf.bssid = nullptr;
    scanConf.channel = 0;
    scanConf.show_hidden = true;
    esp_err_t err = esp_wifi_scan_start(&scanConf, false);
    ESP_LOGI(TAG, "doScan(): esp_wifi_scan_start -> %s", esp_err_to_name(err));
    publishEvent();
  }
}

void PocketMageWifi::doConnect() {
  ESP_LOGI(TAG, "doConnect(): entered");
  char ssid[33] = {0};
  char password[65] = {0};
  bool save = false;
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  if (_pendingSSID[0] == 0) {
    xSemaphoreGiveRecursive(_mutex);
    setStatus(TR(STR_WIFI_NO_SSID));
    return;
  }
  strncpy(ssid, _pendingSSID, sizeof(ssid));
  ssid[sizeof(ssid) - 1] = 0;
  strncpy(password, _pendingPassword, sizeof(password));
  password[sizeof(password) - 1] = 0;
  save = _pendingSave;
  xSemaphoreGiveRecursive(_mutex);
  setStatus(TR(STR_WIFI_CONNECTING));
  wifi_config_t config = {};
  strncpy((char*)config.sta.ssid, ssid, sizeof(config.sta.ssid));
  config.sta.ssid[sizeof(config.sta.ssid) - 1] = 0;
  strncpy((char*)config.sta.password, password, sizeof(config.sta.password));
  config.sta.password[sizeof(config.sta.password) - 1] = 0;
  config.sta.threshold.authmode = (password[0] != 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
  config.sta.pmf_cfg.capable = true;
  esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &config);
  ESP_LOGI(TAG, "doConnect(): esp_wifi_set_config(%s) -> %s", ssid, esp_err_to_name(err));
  err = esp_wifi_connect();
  ESP_LOGI(TAG, "doConnect(): esp_wifi_connect -> %s", esp_err_to_name(err));
  if (save)
    saveCredentials(ssid, password);
  _state = WifiRadioState::Connecting;
  _retryCount = 0;
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_retrySSID, ssid, sizeof(_retrySSID));
  _retrySSID[sizeof(_retrySSID) - 1] = 0;
  strncpy(_retryPassword, password, sizeof(_retryPassword));
  _retryPassword[sizeof(_retryPassword) - 1] = 0;
  xSemaphoreGiveRecursive(_mutex);
  publishEvent();
}

void PocketMageWifi::doDisconnect() {
  esp_wifi_disconnect();
  setStatus(TR(STR_WIFI_DISCONNECTING));
  _state = WifiRadioState::On;
  publishEvent();
}

void PocketMageWifi::doAutoConnect() {
  // Try to find a saved network in scan results
  char ssid[33] = {0};
  char password[65] = {0};
  if (findSavedNetwork(ssid, password)) {
    xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
    strncpy(_pendingSSID, ssid, sizeof(_pendingSSID));
    _pendingSSID[sizeof(_pendingSSID) - 1] = 0;
    strncpy(_pendingPassword, password, sizeof(_pendingPassword));
    _pendingPassword[sizeof(_pendingPassword) - 1] = 0;
    _pendingSave = false;
    xSemaphoreGiveRecursive(_mutex);
    doConnect();
  }
}

void PocketMageWifi::setStatus(const char* msg) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_statusMessage, msg, sizeof(_statusMessage));
  _statusMessage[sizeof(_statusMessage) - 1] = 0;
  xSemaphoreGiveRecursive(_mutex);
  publishEvent();
}

void PocketMageWifi::publishEvent() {
  _eventPending = true;
}

void PocketMageWifi::dispatchEvents() {
  if (_eventPending) {
    _eventPending = false;
    if (_eventCallback)
      _eventCallback();
  }
}

void PocketMageWifi::saveCredentials(const char* ssid, const char* password) {
  if (_prefs.begin(PREFS_NAMESPACE, false)) {
    _prefs.putString(ssid, password);
    _prefs.end();
  }
}

bool PocketMageWifi::findSavedNetwork(char* ssid, char* password) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  bool found = false;
  if (_scanResults && _scanResultCount > 0) {
    for (uint16_t i = 0; i < _scanResultCount; ++i) {
      if (hasSavedCredentials((const char*)_scanResults[i].ssid)) {
        strncpy(ssid, (const char*)_scanResults[i].ssid, 33);
        ssid[32] = 0;
        loadSavedCredentials(ssid, password, 65);
        found = true;
        break;
      }
    }
  }
  xSemaphoreGiveRecursive(_mutex);
  return found;
}

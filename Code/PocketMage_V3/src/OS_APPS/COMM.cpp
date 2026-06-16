#if !OTA_APP_FLAG

#include <globals.h>
#include <WiFi.h>
#include <esp_wifi.h>

extern "C" {
  #include "mesh_now.h"
  #include "message_queue.h"
}

static constexpr const char* TAG = "COMM";

// CONFIG
#define MAX_CHAT_MSGS 50
#define MAX_INPUT_LEN 120
#define MAX_VISIBLE_LINES 18

// TYPES
enum CommState { PEER_LIST, CHAT_VIEW, CHAT_COMPOSE };
enum ChatMode { LOCAL_CHAT, DIRECT_CHAT };

struct ChatMsg {
  uint32_t timestamp;
  char sender[18];
  char content[128];
  bool sentByLocal;
};

// STATE
static bool meshReady = false;
static CommState currentState = PEER_LIST;
static ChatMode chatMode = LOCAL_CHAT;

static ChatMsg msgs[MAX_CHAT_MSGS];
static int msgCount = 0;
static int scrollOff = 0;
static bool autoScroll = true;

static char inputBuf[MAX_INPUT_LEN + 1];
static int inputLen = 0;

static uint8_t myMAC[6];
static char myMacStr[18] = "00:00:00:00:00:00";
static uint8_t peerMAC[6];
static char peerMacStr[18] = "00:00:00:00:00:00";
static int selPeer = 0;
static bool comm_first_draw = true;

// MAC HELPERS
static void macToStr(const uint8_t* m, char* out) {
  sprintf(out, "%02X:%02X:%02X:%02X:%02X:%02X",
          m[0], m[1], m[2], m[3], m[4], m[5]);
}

static String displayName(const char* mac) {
  if (strcmp(mac, myMacStr) == 0) return "Me";
  if (strcmp(mac, "00:1A:2B:3C:4D:5E") == 0) return "Chris";
  return String(mac);
}

// SD LOGGING
static void logToSD(const ChatMsg* m) {
  if (PM_SDAUTO().getNoSD() || !global_fs) return;
  SDActive = true;
  global_fs->mkdir("/chat");
  char path[64];
  if (chatMode == LOCAL_CHAT) {
    snprintf(path, sizeof(path), "/chat/_broadcast.log");
  } else {
    char fm[18];
    macToStr(peerMAC, fm);
    for (char* p = fm; *p; p++) if (*p == ':') *p = '-';
    snprintf(path, sizeof(path), "/chat/dm_%s.log", fm);
  }
  File f = global_fs->open(path, FILE_APPEND);
  if (f) {
    f.printf("%u|%s|%s\n", m->timestamp, m->sender, m->content);
    f.close();
  }
  SDActive = false;
}

// MESSAGE BUFFER
static void addMsg(const char* sender, const char* content, bool local) {
  if (msgCount >= MAX_CHAT_MSGS) {
    int mv = MAX_CHAT_MSGS - 1;
    memmove(&msgs[0], &msgs[1], mv * sizeof(ChatMsg));
    msgCount = mv;
    if (scrollOff > 0) scrollOff--;
  }
  ChatMsg* m = &msgs[msgCount++];
  strncpy(m->sender, sender, sizeof(m->sender));
  m->sender[sizeof(m->sender) - 1] = '\0';
  strncpy(m->content, content, sizeof(m->content));
  m->content[sizeof(m->content) - 1] = '\0';
  m->sentByLocal = local;
  m->timestamp = millis() / 1000;

  if (autoScroll) {
    int vis = msgCount;
    if (vis > MAX_VISIBLE_LINES) vis = MAX_VISIBLE_LINES;
    scrollOff = msgCount - vis;
    if (scrollOff < 0) scrollOff = 0;
  }

  logToSD(m);
  newState = true;
}

// MESH-NOW RECEIVE CALLBACK
// Runs in WiFi task context so must be thread-safe, no blocking
static void meshRecvCb(const mesh_message_t* msg) {
  if (msg->type != MSG_TYPE_CHAT && msg->type != MSG_TYPE_DIRECT) {
    ESP_LOGD(TAG, "recvCb: ignored type=%d", msg->type);
    return;
  }
  if (msg->type == MSG_TYPE_DIRECT) {
    if (memcmp(msg->target_mac, myMAC, 6) != 0) {
      ESP_LOGD(TAG, "recvCb: direct msg not for us");
      return;
    }
  }
  ESP_LOGI(TAG, "recvCb: type=%d msg='%s'", msg->type, msg->message);
  message_t q{};
  strncpy(q.message, msg->message, sizeof(q.message));
  q.message[sizeof(q.message) - 1] = '\0';
  memcpy(q.sender_mac, msg->sender_mac, 6);
  q.timestamp = msg->timestamp;
  esp_err_t qret = message_queue_send(&q);
  if (qret != ESP_OK) {
    ESP_LOGE(TAG, "recvCb: queue_send failed");
  }
}

// INIT
void COMM_INIT() {
  setCpuFrequencyMhz(240);
  CurrentAppState = COMM;
  currentState = PEER_LIST;
  chatMode = LOCAL_CHAT;
  msgCount = 0;
  scrollOff = 0;
  autoScroll = true;
  selPeer = 0;
  newState = true;

  comm_first_draw = true;
  if (meshReady) {
    message_t discard{};
    while (message_queue_receive(&discard, 0) == ESP_OK) {}
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);

  // Set WiFi channel to match Mesh-NOW nodes (channel 1)
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  esp_read_mac(myMAC, ESP_MAC_WIFI_STA);
  macToStr(myMAC, myMacStr);
  ESP_LOGI(TAG, "Host MAC: %s", myMacStr);

  message_queue_init();

  mesh_now_set_receive_callback(meshRecvCb);

  mesh_now_deinit();
  esp_err_t err = mesh_now_init();
  if (err == ESP_OK) {
    meshReady = true;
    ESP_LOGI(TAG, "Mesh-NOW ready");
    { uint8_t ch; wifi_second_chan_t sc; esp_wifi_get_channel(&ch, &sc); ESP_LOGI(TAG, "WiFi mode=%d channel=%d", WiFi.getMode(), ch); }
  } else {
    ESP_LOGE(TAG, "mesh_now_init failed: %s", esp_err_to_name(err));
  }
}

// DRAIN INCOMING QUEUE
static void drainQueue() {
  if (!meshReady) return;
  message_t q{};
  while (message_queue_receive(&q, 0) == ESP_OK) {
    char s[18];
    macToStr(q.sender_mac, s);
    bool fromMe = (memcmp(q.sender_mac, myMAC, 6) == 0);
    if (chatMode == LOCAL_CHAT) {
      if (!fromMe) {
        ESP_LOGI(TAG, "drain: local from %s '%s'", s, q.message);
        addMsg(s, q.message, false);
      }
    } else {
      if (memcmp(q.sender_mac, peerMAC, 6) == 0) {
        ESP_LOGI(TAG, "drain: direct from %s '%s'", s, q.message);
        addMsg(s, q.message, false);
      }
    }
  }
}

// KEYBOARD / LOOP
void processKB_COMM() {
  int nowMs = millis();

  if (nowMs - KBBounceMillis >= KB_COOLDOWN) {
    char ch = KB().updateKeypress();
    if (ch != 0) {
      KBBounceMillis = nowMs;

      switch (currentState) {
        case PEER_LIST: {
          int totalRooms = 1 + mesh_now_get_peer_count();
          if (ch == 7 || ch == 29) {
            if (selPeer > 0) { selPeer--; newState = true; }
          } else if (ch == 6 || ch == 25 || ch == 30) {
            if (selPeer < totalRooms - 1) { selPeer++; newState = true; }
          } else if (ch == 13 || ch == ' ' || ch == 20) {
            if (selPeer == 0) {
              chatMode = LOCAL_CHAT;
              currentState = CHAT_VIEW;
              OLED().oledWord("Local Chat");
            } else {
              mesh_peer_t* peers = mesh_now_get_peers();
              int pc = mesh_now_get_peer_count();
              int peerIdx = selPeer - 1;
              if (pc > 0 && peerIdx < pc) {
                chatMode = DIRECT_CHAT;
                memcpy(peerMAC, peers[peerIdx].peer_addr, 6);
                macToStr(peerMAC, peerMacStr);
                currentState = CHAT_VIEW;
                OLED().oledWord(displayName(peerMacStr).c_str());
              }
            }
            newState = true;
          } else if (ch == 12 || ch == 8 || ch == 127) {
            HOME_INIT();
          }
          break;
        }

        case CHAT_VIEW: {
          if (ch == 13 || ch == ' ' || ch == 20) {
            currentState = CHAT_COMPOSE;
            inputLen = 0;
            inputBuf[0] = '\0';
            newState = true;
            OLED().oledWord("Type...");
          } else if (ch == 8 || ch == 127 || ch == 19) {
            currentState = PEER_LIST;
            newState = true;
            OLED().oledWord("Chat");
          } else if (ch == 12) {
            HOME_INIT();
          } else if (ch == 7 || ch == 29) {
            if (scrollOff > 0) { scrollOff--; autoScroll = false; newState = true; }
          } else if (ch == 6 || ch == 25 || ch == 30) {
            int maxOff = msgCount - MAX_VISIBLE_LINES;
            if (maxOff < 0) maxOff = 0;
            if (scrollOff < maxOff) { scrollOff++; newState = true; }
            autoScroll = (scrollOff >= maxOff);
          }
          break;
        }

        case CHAT_COMPOSE: {
          ESP_LOGD(TAG, "COMPOSE: ch=%d inputLen=%d", ch, inputLen);
          if (ch == 13) {
            if (inputLen > 0) {
              inputBuf[inputLen] = '\0';
              ESP_LOGI(TAG, "SENDING: '%s'", inputBuf);
              esp_err_t sendRet;
              if (chatMode == LOCAL_CHAT) sendRet = mesh_now_send_broadcast(inputBuf);
              else sendRet = mesh_now_send_direct(peerMAC, inputBuf);
              if (sendRet != ESP_OK) {
                ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(sendRet));
              }
              addMsg(myMacStr, inputBuf, true);
              ESP_LOGI(TAG, "addMsg done, msgCount=%d", msgCount);
            }
            inputLen = 0;
            inputBuf[0] = '\0';
            currentState = CHAT_VIEW;
            newState = true;
            OLED().oledWord("Chat");
          } else if (ch == 8 || ch == 127) {
            if (inputLen > 0) { inputLen--; inputBuf[inputLen] = '\0'; }
          } else if (ch == 12 || ch == 19) {
            inputLen = 0;
            inputBuf[0] = '\0';
            currentState = CHAT_VIEW;
            newState = true;
            OLED().oledWord("Chat");
          } else if (ch >= 32 && ch <= 126 && inputLen < MAX_INPUT_LEN) {
            inputBuf[inputLen++] = ch;
            inputBuf[inputLen] = '\0';
          }
          break;
        }
      }
    }
  }

  drainQueue();
  nowMs = millis();

  if (nowMs - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = nowMs;
    if (currentState == CHAT_COMPOSE) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_lubR18_tf);
      u8g2.setCursor(5, 24);
      u8g2.print(inputBuf);
      u8g2.sendBuffer();
    } else if (currentState == PEER_LIST) {
      OLED().oledWord("Chat");
    } else {
      if (chatMode == LOCAL_CHAT) OLED().oledWord("Local Chat");
      else OLED().oledWord(displayName(peerMacStr).c_str());
    }
  }
}

// E-INK DRAW
void einkHandler_COMM() {
  if (!newState && !comm_first_draw) return;
  comm_first_draw = false;
  newState = false;
  ESP_LOGI(TAG, "einkHandler: state=%d chatMode=%d msgCount=%d scrollOff=%d",
           currentState, chatMode, msgCount, scrollOff);

  display.fillScreen(GxEPD_WHITE);

  // Top bar
  display.fillRect(0, 0, display.width(), 20, GxEPD_BLACK);
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&FreeSans9pt7b);

  if (currentState == PEER_LIST) {
    display.setCursor(4, 16);
    display.print("Select Room");
    display.setFont(&Font5x7Fixed);
    display.setCursor(164, 14);
    display.print("Me " + String(myMacStr));
    display.setCursor(270, 14);
    display.print("P: " + String(mesh_now_get_peer_count()));

    int totalRooms = 1 + mesh_now_get_peer_count();
    mesh_peer_t* allPeers = mesh_now_get_peers();
    int listY = 32;
    int vis = min(totalRooms, MAX_VISIBLE_LINES);
    int scrollTop = max(selPeer - vis / 2, 0);
    if (scrollTop + vis > totalRooms) scrollTop = max(totalRooms - vis, 0);
    display.setFont(&FreeSans9pt7b);
    for (int i = 0; i < vis; i++) {
      int idx = scrollTop + i;
      if (idx >= totalRooms) break;
      int yPos = listY + i * 14;
      if (yPos > 218) break;
      bool selected = (idx == selPeer);
      String label;
      if (idx == 0) {
        label = "Local Chat";
      } else {
        int peerIdx = idx - 1;
        if (peerIdx < mesh_now_get_peer_count()) {
          char macStr[18];
          macToStr(allPeers[peerIdx].peer_addr, macStr);
          label = displayName(macStr);
        } else {
          label = "---";
        }
      }
      if (selected) {
        display.fillRect(2, yPos - 9, display.width() - 12, 13, GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
      } else {
        display.setTextColor(GxEPD_BLACK);
      }
      display.setCursor(8, yPos);
      display.print(label);
    }
    // Scrollbar
    if (totalRooms > vis) {
      int sbY = 26;
      int sbH = 220 - 26;
      float step = (float)sbH / totalRooms;
      int thumbY = sbY + (int)(selPeer * step);
      int thumbH = max((int)(vis * step), 8);
      display.fillRect(display.width() - 7, thumbY, 4, thumbH, GxEPD_BLACK);
    }
  } else {
    if (chatMode == LOCAL_CHAT) {
      display.setCursor(4, 16);
      display.print("Local Chat");
    } else {
      String name = displayName(peerMacStr);
      display.setCursor(4, 16);
      display.print("> " + name);
    }
    display.setFont(&Font5x7Fixed);
    display.setCursor(164, 14);
    display.print(chatMode == LOCAL_CHAT ? "ESP-NOW" : "Direct");
    display.setCursor(270, 14);
    display.print("P: " + String(mesh_now_get_peer_count()));
  }

  // Separator line
  display.drawFastHLine(0, 21, display.width(), GxEPD_BLACK);

  // Message area (CHAT_VIEW / CHAT_COMPOSE only)
  if (currentState != PEER_LIST) {
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&Font5x7Fixed);
    int y = 30;
    int lineH = 11;
    for (int i = scrollOff; i < msgCount && y < 220; i++) {
      ChatMsg* m = &msgs[i];
      String prefix = displayName(m->sender) + ": ";
      display.setCursor(2, y);
      display.print(prefix);
      int16_t x1, y1;
      uint16_t pw, ph;
      display.getTextBounds(prefix, 0, 0, &x1, &y1, &pw, &ph);
      int cx = 2 + pw;
      display.setCursor(cx, y);
      display.print(m->content);
      y += lineH;
    }
  }

  // Status bar
  display.drawFastHLine(0, 220, display.width(), GxEPD_BLACK);
  display.setCursor(4, 234);
  display.setFont(&Font5x7Fixed);
  if (currentState == CHAT_COMPOSE) {
    display.print("> ");
    display.print(inputBuf);
  } else if (currentState == CHAT_VIEW) {
    if (msgCount == 0) display.print("No messages yet. Press Enter to type.");
    else display.print("Enter: type  |  Up/Dn: scroll  |  Esc: back");
  } else {
    display.print("FN+7/6: up/dn  |  Enter: sel  |  FN+<-: home");
  }

  EINK().refresh();
}

#endif

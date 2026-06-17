#if !OTA_APP_FLAG

#include <globals.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <vector>

extern "C" {
  #include "mesh_now.h"
  #include "message_queue.h"
}

static constexpr const char* TAG = "COMM";

// CONFIG
#define MAX_CHAT_MSGS 50
#define BUBBLE_MAX_CHARS 46
#define MAX_VISIBLE_LINES 10

// TYPES
enum CommState { PEER_LIST, CHAT_VIEW };
enum ChatMode { LOCAL_CHAT, DIRECT_CHAT };

struct ChatMsg {
  uint32_t timestamp;
  uint8_t hr;
  uint8_t mn;
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
static bool autoScroll = true;

static uint8_t myMAC[6];
static char myMacStr[18] = "00:00:00:00:00:00";
static uint8_t peerMAC[6];
static char peerMacStr[18] = "00:00:00:00:00:00";
static int selPeer = 0;
static int prevSelPeer = 0;
static bool comm_first_draw = true;
static int last_peer_count = -1;

// UI FLAGS
static bool cursor_moved = false;

// CHAT INPUT STATE
static String chatInputBuffer = "";
static int chatCursorPos = 0;
static ulong chatScrollIndex = 0;

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

// TEXT WRAPPER
static std::vector<String> wrapText(String text, int maxLineChars) {
  std::vector<String> lines;
  while (text.length() > maxLineChars) {
    int split = -1;
    // Look backwards for a space or hyphen to split cleanly
    for (int i = maxLineChars; i >= 0; i--) {
      if (text[i] == ' ' || text[i] == '-') {
        split = i + 1; 
        break;
      }
    }
    if (split == -1) split = maxLineChars; // Force split if one giant word
    
    String line = text.substring(0, split);
    line.trim();
    lines.push_back(line);
    text = text.substring(split);
    text.trim();
  }
  if (text.length() > 0) {
    text.trim();
    if(text.length() > 0) lines.push_back(text);
  }
  return lines;
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
  }
  ChatMsg* m = &msgs[msgCount++];
  strncpy(m->sender, sender, sizeof(m->sender));
  m->sender[sizeof(m->sender) - 1] = '\0';
  strncpy(m->content, content, sizeof(m->content));
  m->content[sizeof(m->content) - 1] = '\0';
  m->sentByLocal = local;
  
  DateTime now = CLOCK().nowDT();
  m->timestamp = now.unixtime();
  m->hr = now.hour();
  m->mn = now.minute();

  logToSD(m);
  newState = true; // Incoming message forces an E-ink refresh
}

// MESH-NOW RECEIVE CALLBACK
static void meshRecvCb(const mesh_message_t* msg) {
  if (msg->type != MSG_TYPE_CHAT && msg->type != MSG_TYPE_DIRECT) {
    return;
  }
  if (msg->type == MSG_TYPE_DIRECT) {
    if (memcmp(msg->target_mac, myMAC, 6) != 0) {
      return;
    }
  }
  message_t q{};
  strncpy(q.message, msg->message, sizeof(q.message));
  q.message[sizeof(q.message) - 1] = '\0';
  memcpy(q.sender_mac, msg->sender_mac, 6);
  q.timestamp = msg->timestamp;
  message_queue_send(&q);
}

// INIT
void COMM_INIT() {
  setCpuFrequencyMhz(240);
  CurrentAppState = COMM;
  currentState = PEER_LIST;
  chatMode = LOCAL_CHAT;
  msgCount = 0;
  autoScroll = true;
  selPeer = 0;
  prevSelPeer = 0;
  cursor_moved = false;
  chatInputBuffer = "";
  chatCursorPos = 0;
  chatScrollIndex = 0;
  last_peer_count = -1;
  newState = true;

  comm_first_draw = true;
  if (meshReady) {
    message_t discard{};
    while (message_queue_receive(&discard, 0) == ESP_OK) {}
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  esp_read_mac(myMAC, ESP_MAC_WIFI_STA);
  macToStr(myMAC, myMacStr);

  message_queue_init();
  mesh_now_set_receive_callback(meshRecvCb);
  mesh_now_deinit();
  if (mesh_now_init() == ESP_OK) {
    meshReady = true;
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
      if (!fromMe) addMsg(s, q.message, false);
    } else {
      if (memcmp(q.sender_mac, peerMAC, 6) == 0) addMsg(s, q.message, false);
    }
  }
}

void chatScrollPreview() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  int startLine = 0;
  if (chatScrollIndex >= 1) startLine = chatScrollIndex - 1;
  int y = 7;
  for (int i = startLine; i < startLine + 4; i++) {
    if (i >= msgCount) break;
    if (i == (int)chatScrollIndex) u8g2.drawTriangle(0, y - 6, 0, y, 4, y - 3);
    u8g2.setFont(u8g2_font_5x7_tf);
    String dispStr = String(msgs[i].sender) + ": " + String(msgs[i].content);
    if (dispStr.length() > 38) dispStr = dispStr.substring(0, 38) + "..";
    u8g2.drawUTF8(6, y, dispStr.c_str());
    y += 8;
  }
  u8g2.sendBuffer();
}

// KEYBOARD / LOOP
void processKB_COMM() {
  drainQueue(); 

  int current_pc = mesh_now_get_peer_count();
  if (current_pc != last_peer_count) {
    last_peer_count = current_pc;
    if (currentState == PEER_LIST) {
      newState = true;
    }
  }

  int nowMs = millis();

  // Handle TOUCH scrolling
  if (currentState == CHAT_VIEW) {
      int maxScrollIndex = 0;
      if (msgCount > 0) {
          int totalH = 0;
          int top = msgCount - 1;
          while (top >= 0) {
              std::vector<String> lines = wrapText(msgs[top].content, BUBBLE_MAX_CHARS);
              int bH = (lines.size() * 10) + 12 + 10 + 4; 
              if (totalH + bH > 214) { top++; break; }
              totalH += bH;
              if (top == 0) break;
              top--;
          }
          maxScrollIndex = top;
      }
      if (TOUCH().updateScroll(maxScrollIndex, chatScrollIndex)) {
          newState = true;
          autoScroll = (chatScrollIndex >= maxScrollIndex);
      }
  }

  if (nowMs - KBBounceMillis >= KB_COOLDOWN) {
    char ch = KB().updateKeypress();
    if (ch != 0) {
      KBBounceMillis = nowMs;

      if (currentState == PEER_LIST) {
        int totalRooms = 1 + mesh_now_get_peer_count();
        prevSelPeer = selPeer;

        if (ch == 19 || ch == 7 || ch == 29) {
          if (selPeer > 0) { selPeer--; cursor_moved = true; }
        } 
        else if (ch == 21 || ch == 6 || ch == 25 || ch == 30) {
          if (selPeer < totalRooms - 1) { selPeer++; cursor_moved = true; }
        } 
        else if (ch == 13 || ch == ' ' || ch == 20) {
          chatInputBuffer = "";
          chatCursorPos = 0;
          autoScroll = true;

          if (selPeer == 0) {
            chatMode = LOCAL_CHAT;
            currentState = CHAT_VIEW;
            newState = true;
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
              newState = true;
              OLED().oledWord(displayName(peerMacStr).c_str());
            }
          }
        } 
        else if (ch == 12 || ch == 8 || ch == 127) {
          HOME_INIT();
        }
      } 
      else if (currentState == CHAT_VIEW) {
        if (ch == 13) { 
            if (chatInputBuffer.length() > 0) {
                esp_err_t sendRet;
                if (chatMode == LOCAL_CHAT) sendRet = mesh_now_send_broadcast(chatInputBuffer.c_str());
                else sendRet = mesh_now_send_direct(peerMAC, chatInputBuffer.c_str());
                
                if (sendRet == ESP_OK) {
                    addMsg(myMacStr, chatInputBuffer.c_str(), true);
                }
                chatInputBuffer = "";
                chatCursorPos = 0;
                autoScroll = true;
                newState = true;
            }
        }
        else if (ch == 12) { 
            currentState = PEER_LIST;
            chatInputBuffer = "";
            chatCursorPos = 0;
            newState = true;
            OLED().oledWord("Chat");
        }
        else if (ch == 17) {
            if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(SHIFT);
        }
        else if (ch == 18) {
            if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(FUNC);
        }
        else if (ch == 8) { 
            if (chatInputBuffer.length() > 0 && chatCursorPos != 0) {
                int old_cursor = chatCursorPos;
                do { chatCursorPos--; } while (chatCursorPos > 0 && (chatInputBuffer[chatCursorPos] & 0xC0) == 0x80);
                int bytesToDelete = old_cursor - chatCursorPos;
                chatInputBuffer.remove(chatCursorPos, bytesToDelete);
            }
        }
        else if (ch == 19) { 
            if (chatCursorPos > 0) {
                do { chatCursorPos--; } while (chatCursorPos > 0 && (chatInputBuffer[chatCursorPos] & 0xC0) == 0x80);
            }
        }
        else if (ch == 21) { 
            if (chatCursorPos < chatInputBuffer.length()) {
                do { chatCursorPos++; } while (chatCursorPos < chatInputBuffer.length() && (chatInputBuffer[chatCursorPos] & 0xC0) == 0x80);
            }
        }
        else if (ch == 28) { chatCursorPos = 0; KB().setKeyboardState(NORMAL); }
        else if (ch == 30) { chatCursorPos = chatInputBuffer.length(); KB().setKeyboardState(NORMAL); }
        else if (ch == 29) { KB().setKeyboardState(NORMAL); }
        else if (ch == 6) { KB().setKeyboardState(NORMAL); }
        else if (ch == 7) { chatInputBuffer = ""; chatCursorPos = 0; KB().setKeyboardState(NORMAL); }
        else if (ch == 24 || ch == 25 || ch == 26) { KB().setKeyboardState(NORMAL); }
        else if (ch == 9 || ch == 14) { KB().setKeyboardState(NORMAL); } 
        else if (ch == 20 || ch == 23) {} 
        else { 
            if (chatCursorPos == 0) {
                chatInputBuffer = ch + chatInputBuffer;
            } else if (chatCursorPos == chatInputBuffer.length()) {
                chatInputBuffer += ch;
            } else {
                String left = chatInputBuffer.substring(0, chatCursorPos);
                String right = chatInputBuffer.substring(chatCursorPos);
                chatInputBuffer = left + ch + right;
            }
            chatCursorPos++;
            if (ch >= 48 && ch <= 57) {} 
            else if (KB().getKeyboardState() != NORMAL) KB().setKeyboardState(NORMAL);
        }
      }
    }
  }

  nowMs = millis();
  if (nowMs - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = nowMs;
    if (currentState == PEER_LIST) {
      if (TOUCH().getLastTouch() == -1) OLED().oledWord("Chat");
    } else {
      if (TOUCH().getLastTouch() == -1) {
        OLED().oledLine(chatInputBuffer, chatCursorPos, false, "Message: ");
      } else {
        chatScrollPreview();
      }
    }
  }
}

// E-INK DRAW
void einkHandler_COMM() {
  if (cursor_moved) {
    cursor_moved = false;
    
    int totalRooms = 1 + mesh_now_get_peer_count();
    int vis = min(totalRooms, MAX_VISIBLE_LINES);
    
    int scrollTop = max(selPeer - vis / 2, 0);
    if (scrollTop + vis > totalRooms) scrollTop = max(totalRooms - vis, 0);
    
    int prevScrollTop = max(prevSelPeer - vis / 2, 0);
    if (prevScrollTop + vis > totalRooms) prevScrollTop = max(totalRooms - vis, 0);
    
    if (scrollTop != prevScrollTop || newState) {
      newState = true; 
    } else {
      // Safely perform native partial window update without blanking the rest of the screen
      display.fillRect(0, 28, 16, 218, GxEPD_WHITE);
      display.setTextColor(GxEPD_BLACK);
      display.setFont(&FreeSans9pt7b);
      
      for (int i = 0; i < vis; i++) {
        int idx = scrollTop + i;
        if (idx == selPeer) {
          int yPos = 36 + i * 20;
          display.setCursor(4, yPos);
          display.print(">");
        }
      }
      
      // Push only this rectangle to the display
      display.displayWindow(0, 28, 16, 218); 
      return;
    }
  }

  if (!newState && !comm_first_draw) return;
  comm_first_draw = false;
  newState = false;

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
    
    int listY = 36;
    int vis = min(totalRooms, MAX_VISIBLE_LINES);
    int scrollTop = max(selPeer - vis / 2, 0);
    if (scrollTop + vis > totalRooms) scrollTop = max(totalRooms - vis, 0);
    
    display.setFont(&FreeSans9pt7b);
    for (int i = 0; i < vis; i++) {
      int idx = scrollTop + i;
      if (idx >= totalRooms) break;
      int yPos = listY + i * 20;
      if (yPos > 238) break; 
      
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
      
      display.setTextColor(GxEPD_BLACK);
      
      if (selected) {
        display.setCursor(4, yPos);
        display.print(">");
      }
      display.setCursor(20, yPos);
      display.print(label);
    }
    
    // Scrollbar (Extended to bottom)
    if (totalRooms > vis) {
      int sbY = 26;
      int sbH = 240 - 26; 
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

  // Message area (CHAT_VIEW only)
  if (currentState == CHAT_VIEW) {
    display.setFont(&Font5x7Fixed);
    
    int maxScrollIndex = 0;
    if (msgCount > 0) {
        int totalH = 0;
        int top = msgCount - 1;
        while (top >= 0) {
            std::vector<String> lines = wrapText(msgs[top].content, BUBBLE_MAX_CHARS);
            int bH = (lines.size() * 10) + 12 + 10 + 4; 
            if (totalH + bH > 214) { top++; break; }
            totalH += bH;
            if (top == 0) break;
            top--;
        }
        maxScrollIndex = top;
    }

    if (autoScroll) chatScrollIndex = maxScrollIndex;
    if (chatScrollIndex > (ulong)maxScrollIndex) chatScrollIndex = maxScrollIndex;

    int y = 26; 
    int barWidth = 3;
    
    for (int i = chatScrollIndex; i < msgCount && y < 240; i++) {
      ChatMsg* m = &msgs[i];
      std::vector<String> lines = wrapText(m->content, BUBBLE_MAX_CHARS);
      
      String nameText = displayName(m->sender);
      String timeText = String(m->hr) + ":" + (m->mn < 10 ? "0" : "") + String(m->mn);
      
      int nameW = nameText.length() * 6;
      int timeW = timeText.length() * 6;
      int metaW = nameW + timeW + 10; // Extra inner-gap between name and time
      
      int textW = 0;
      for (const String& l : lines) {
        int lw = l.length() * 6;
        if (lw > textW) textW = lw;
      }

      int bubbleW = max(textW, metaW) + 16; // 8px padding on both sides
      int bubbleH = (lines.size() * 10) + 12 + 10;
      
      // Shift left to make room for scrollbar
      int x = m->sentByLocal ? (display.width() - bubbleW - 6 - (barWidth + 2)) : 6;
      
      if (m->sentByLocal) {
          display.fillRoundRect(x, y, bubbleW, bubbleH, 10, GxEPD_BLACK);
          display.setTextColor(GxEPD_WHITE);
      } else {
          display.drawRoundRect(x, y, bubbleW, bubbleH, 10, GxEPD_BLACK);
          display.setTextColor(GxEPD_BLACK);
      }
      
      display.setCursor(x + 8, y + 11);
      display.print(nameText);
      
      display.setCursor(x + bubbleW - 8 - timeW, y + 11);
      display.print(timeText);
      
      display.drawFastHLine(x + 8, y + 14, bubbleW - 16, m->sentByLocal ? GxEPD_WHITE : GxEPD_BLACK);
      
      for(size_t l=0; l<lines.size(); l++){
          display.setCursor(x + 8, y + 26 + (l*10));
          display.print(lines[l]);
      }
      
      y += bubbleH + 4; 
    }

    if (maxScrollIndex > 0) {
      float visibleRatio = 214.0 / ((maxScrollIndex + 1) * 30.0); 
      if (visibleRatio > 1.0) visibleRatio = 1.0;
      int handleHeight = max((int)(214 * visibleRatio), 15);
      float scrollFraction = (float)chatScrollIndex / maxScrollIndex;
      int handleY = 26 + scrollFraction * (214 - handleHeight);
      
      display.fillRect(display.width() - barWidth - 1, handleY, barWidth, handleHeight, GxEPD_BLACK);
    }
  }

  EINK().refresh();
}

#endif
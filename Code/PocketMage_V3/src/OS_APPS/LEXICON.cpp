// AUDIT 1

#include <globals.h>
#if !OTA_APP  // POCKETMAGE_OS
enum LexState { MENU, DEF };
LexState CurrentLexState = MENU;

// -----------------------------
// Nth-definition helpers
// -----------------------------

struct LexQuery {
  String word;
  int requestedIndex;  // -1 if not specified, 0-based otherwise
};

LexQuery parseLexQuery(const String& input) {
  LexQuery q;
  q.word = input;
  q.requestedIndex = -1;

  String trimmed = input;
  trimmed.trim();
  if (trimmed.length() == 0)
    return q;

  int lastSpace = trimmed.lastIndexOf(' ');
  if (lastSpace == -1) {
    q.word = trimmed;
    return q;
  }

  String maybeNumber = trimmed.substring(lastSpace + 1);
  bool isNumber = true;
  for (size_t i = 0; i < maybeNumber.length(); i++) {
    if (!isDigit(maybeNumber[i])) {
      isNumber = false;
      break;
    }
  }

  if (isNumber) {
    q.word = trimmed.substring(0, lastSpace);
    q.word.trim();
    int n = maybeNumber.toInt();
    if (n > 0)
      q.requestedIndex = n - 1;  // convert to 0-based
  } else {
    q.word = trimmed;
  }

  return q;
}

int clampDefinitionIndex(int idx, int total) {
  if (total <= 0)
    return 0;
  if (idx < 0)
    return 0;
  if (idx >= total)
    return total - 1;
  return idx;
}

static String currentLine = "";
static int cursor_pos = 0;

// Vector to hold the definitions
std::vector<std::pair<String, String>> defList;
int definitionIndex = 0;

void LEXICON_INIT() {
  currentLine = "";
  CurrentAppState = LEXICON;
  CurrentLexState = MENU;
  KB().setKeyboardState(NORMAL);
  newState = true;
  definitionIndex = 0;

  // Verify that dict is installed
  pocketmage::setCpuSpeed(240);
  delay(50);
  if (!global_fs->exists("/dict/A.txt")) {
    OLED().sysMessage("Please install dict from GitHub!",5000);
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    HOME_INIT();
  }
}

void loadDefinitions(String input) {
  OLED().oledWord("Loading Definitions");
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  defList.clear();  // Clear previous results

  // Parse query (word + optional index)
  LexQuery query = parseLexQuery(input);
  String word = query.word;

  if (word.length() == 0 || PM_SDAUTO().getNoSD()) {
    SDActive = false;
    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  char firstChar = tolower(word[0]);
  if (firstChar < 'a' || firstChar > 'z') {
    SDActive = false;
    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  String filePath = "/dict/" + String((char)toupper(firstChar)) + ".txt";

  File file = global_fs->open(filePath);
  if (!file) {
    OLED().sysMessage("Missing Dictionary!",2000);
    SDActive = false;
    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  word.toLowerCase();

  int lineScanCount = 0;

  while (file.available()) {

    if (++lineScanCount % 100 == 0) yield(); 

    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      continue;

    int defSplit = line.indexOf(')');
    if (defSplit == -1)
      continue;

    // Extract key and definition
    String key = line.substring(0, defSplit + 1);
    String def = line.substring(defSplit + 1);
    def.trim();

    String keyLower = key;
    keyLower.toLowerCase();

    if (keyLower.startsWith(word)) {
      defList.push_back({key, def});
    } else if (!defList.empty()) {
      // No more definitions for this word (Dictionary is alphabetically sorted)
      break;
    }
  }

  file.close();

  if (defList.empty()) {
    OLED().sysMessage("No definitions found",2000);    

    CurrentLexState = MENU; 
    newState = true;

  } else {
    CurrentLexState = DEF;
    KB().setKeyboardState(NORMAL);

    if (query.requestedIndex >= 0) {
      definitionIndex = clampDefinitionIndex(query.requestedIndex, defList.size());

    } else {
      definitionIndex = 0;
    }

    newState = true;
  }

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

void processKB_LEXICON() {
  int currentMillis = millis();
  String left = "";
  String right = "";
  String command = "";

  switch (CurrentLexState) {
    case MENU:
      KB().setKeyboardState(NORMAL);
      command = textPrompt();
      if (command == "_RETURN_") return;
      else if (command != "_EXIT_") loadDefinitions(command);
      else HOME_INIT();
      break;

    case DEF:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          ;
        // CR Recieved
        else if (inchar == 13) {
          loadDefinitions(currentLine);
          currentLine = "";
          cursor_pos = 0;
        }                                       
        // SHIFT Recieved
        else if (inchar == 17) {
          if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) {
            KB().setKeyboardState(NORMAL);
          } else if (KB().getKeyboardState() == FUNC) {
            KB().setKeyboardState(FN_SHIFT);
          } else {
            KB().setKeyboardState(SHIFT);
          }
        }
        // FN Recieved
        else if (inchar == 18) {
          if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) {
            KB().setKeyboardState(NORMAL);
          } else if (KB().getKeyboardState() == SHIFT) {
            KB().setKeyboardState(FN_SHIFT);
          } else {
            KB().setKeyboardState(FUNC);
          }
        }
        // BKSP Recieved
        else if (inchar == 8) {
          if (currentLine.length() > 0 && cursor_pos != 0) {
            if (cursor_pos == currentLine.length()) {
              currentLine.remove(currentLine.length() - 1, 1);
            } else { 
              currentLine.remove(cursor_pos - 1, 1);
            }
            cursor_pos--;
          }
        }
        // LEFT
        else if (inchar == 19) {
          if (currentLine.length() == 0) {
            definitionIndex--;
            if (definitionIndex < 0)
              definitionIndex = 0;
            newState = true;
          } else {
            if (cursor_pos > 0) {
              cursor_pos--;
            }
          }
        }
        // RIGHT
        else if (inchar == 21) {
          if (currentLine.length() == 0) {
            definitionIndex++;
            if (definitionIndex >= defList.size())
              definitionIndex = defList.size() - 1;
            newState = true;
          } else {
            if (cursor_pos < currentLine.length()) {
              cursor_pos++;
            }
          }
        }
        // CENTER
        else if (inchar == 20) {
        }
        // SHIFT+LEFT
        else if (inchar == 28) {
          cursor_pos = 0;
          KB().setKeyboardState(NORMAL);
        }
        // SHIFT+RIGHT
        else if (inchar == 30) {
          cursor_pos = currentLine.length();
          KB().setKeyboardState(NORMAL);
        }
        // SHIFT+CENTER
        else if (inchar == 29) {
          KB().setKeyboardState(NORMAL);
        }
        // FN+LEFT
        else if (inchar == 12 ) {
           HOME_INIT();
        }
        // FN+RIGHT
        else if (inchar == 6) {
          KB().setKeyboardState(NORMAL);
        }
        // FN+CENTER
        else if (inchar == 7) {
          currentLine = "";
          cursor_pos = 0;
          KB().setKeyboardState(NORMAL);
        }
        // TAB, SHIFT+TAB / FN+TAB, FN+SHIFT+TAB
        else if (inchar == 9 || inchar == 14) {
          KB().setKeyboardState(NORMAL);
        }
        else {
          //split line at cursor_pos
          if (cursor_pos == 0) {
            currentLine = inchar + currentLine;
          } else if (cursor_pos == currentLine.length()) {
            currentLine += inchar;
          } else {
            left = currentLine.substring(0, cursor_pos);
            right = currentLine.substring(cursor_pos);
            currentLine = left + inchar + right;
          }
          cursor_pos++;
          if (inchar >= 48 && inchar <= 57) {
          }  // Only leave FN on if typing numbers
          else if (KB().getKeyboardState() != NORMAL) {
            KB().setKeyboardState(NORMAL);
          }
        }

        currentMillis = millis();
        // Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, cursor_pos, false);
        }
      }
      break;
  }
}

#define LEX_MARGIN 8
#define LEX_LINE_HEIGHT 20

void einkHandler_LEXICON() {
  switch (CurrentLexState) {
    case MENU:
      if (newState) {
        newState = false;
        EINK().resetDisplay(false);
        display.drawBitmap(0, 0, _lex0, 320, 218, GxEPD_BLACK);

        EINK().drawStatusBar("Type a Word:");

        EINK().multiPassRefresh(2);
      }
      break;
      
    case DEF:
      if (newState) {
        newState = false;
        

        EINK().resetDisplay(); 

        display.drawBitmap(0, 0, _lex1, 320, 218, GxEPD_BLACK);
        u8g2f.setForegroundColor(GxEPD_BLACK);

        // Draw Word
        u8g2f.setFont(u8g2_font_ncenR12_tf);
        u8g2f.setFontMode(1);
        u8g2f.setCursor(12, 50);
        u8g2f.print(defList[definitionIndex].first);

        // Draw Definition with Word Wrap
        u8g2f.setFont(u8g2_font_ncenR10_tf);
        u8g2f.setFontMode(1);
        
        String defText = defList[definitionIndex].second;
        int maxW = display.width() - (2 * LEX_MARGIN);
        int cursorY = 87;

        String currentLine = "";
        String currentWord = "";

        // Iterate through every character in the definition string
        for (int i = 0; i <= defText.length(); i++) {
          char c = (i < defText.length()) ? defText[i] : ' '; // Treat end of string as a space to flush the last word

          // If we hit a space, a newline, or the end of the string, process the word
          if (c == ' ' || c == '\n' || i == defText.length()) {
            if (currentWord.length() > 0) {
              String testLine = currentLine;
              if (testLine.length() > 0) testLine += " ";
              testLine += currentWord;

              // Measure the line with the new word added
              int w = u8g2f.getUTF8Width(testLine.c_str());

              // If it exceeds the max width, wrap it to the next line
              if (w > maxW && currentLine.length() > 0) {
                u8g2f.setCursor(LEX_MARGIN, cursorY);
                u8g2f.print(currentLine);
                cursorY += LEX_LINE_HEIGHT;
                currentLine = currentWord; // Start the next line with the word that didn't fit
              } else {
                currentLine = testLine; // Word fits, append it to the current line
              }
              currentWord = ""; // Reset word buffer
            }

            // If the character itself was a newline, force a wrap immediately
            if (c == '\n') {
              u8g2f.setCursor(LEX_MARGIN, cursorY);
              u8g2f.print(currentLine);
              cursorY += LEX_LINE_HEIGHT;
              currentLine = "";
            }
          } else {
            // Build the current word character by character
            currentWord += c;
          }
        }

        // Flush any remaining text in the buffer after the loop ends
        if (currentLine.length() > 0) {
          u8g2f.setCursor(LEX_MARGIN, cursorY);
          u8g2f.print(currentLine);
        }

        EINK().drawStatusBar("Type a New Word:");

        #if POCKETMAGE_HW_VERSION != 2
          EINK().forceSlowFullUpdate(true);
        #endif
        EINK().refresh();
      }
      break;
  }
}
#endif
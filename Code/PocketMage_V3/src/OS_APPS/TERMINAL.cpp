// AUDIT 1
#include <globals.h>

#if !OTA_APP // PocketMage OS Only
#include "wrench.h"

// --- UTF-8 Helpers ---
static inline uint16_t decodeUTF8(const char* str, uint16_t* index, uint16_t len) {
  if (*index >= len) return 0;
  uint8_t c = str[*index];
  (*index)++;
  
  if (c < 0x80) return c; // Standard ASCII
  if ((c & 0xE0) == 0xC0) { // 2-byte sequence
    if (*index >= len) return c; // Incomplete, fallback safely
    uint8_t c2 = str[*index]; (*index)++;
    return ((c & 0x1F) << 6) | (c2 & 0x3F);
  }
  if ((c & 0xF0) == 0xE0) { // 3-byte sequence
    if (*index + 1 >= len) return c;
    uint8_t c2 = str[*index]; (*index)++;
    uint8_t c3 = str[*index]; (*index)++;
    return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
  }
  return c; // Fallback
}

static inline uint8_t mapUnicodeToFontIndex(uint16_t unicode) {
  if (unicode < 0x80) return unicode;
  if (unicode >= 0x00A0 && unicode <= 0x00FF) return unicode - 0x20;
  return 0x7F; // Replacement char
}

static void printUTF8ToEink(const String& s, int maxChars = 9999) {
  uint16_t i = 0;
  int charCount = 0;
  while (i < s.length() && charCount < maxChars) {
    uint16_t unicode = decodeUTF8(s.c_str(), &i, s.length());
    display.write(mapUnicodeToFontIndex(unicode));
    charCount++;
  }
}

// General
enum TERMINAL_functions { PROMPT, POTION };
TERMINAL_functions CurrentTERMfunc = PROMPT;

// Terminal
static std::vector<String> terminalOutputs;
static String currentDir = "/";
static String terminalCommand = "";
static ulong termScrollIndex = 0;
static bool termLargeFont = true;
static int termLinesPerPage = 14;
static int termMaxLineLen = 28;
static bool termDarkTheme = true;

// Potion
static String editFile = "";
static ulong currentPotionLine = 0;
static std::vector<String> potionLines;
static long lastInput = millis();

// Command Links
static std::vector<String> potLinkAliases;
static std::vector<String> potLinkPaths;
static bool potLinksLoaded = false;

// Functions

void loadPotLinks() {
  pocketmage::setCpuSpeed(240);
  potLinkAliases.clear();
  potLinkPaths.clear();
  if (!global_fs->exists("/sys/pot_links.txt")) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }
  
  File f = global_fs->open("/sys/pot_links.txt", FILE_READ);
  if (!f || f.isDirectory()) {
    if (f) f.close();
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }
  
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    
    int comma = line.indexOf(',');
    if (comma != -1) {
      potLinkAliases.push_back(line.substring(0, comma));
      potLinkPaths.push_back(line.substring(comma + 1));
    }
  }
  f.close();
  potLinksLoaded = true;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void savePotLinks() {
  pocketmage::setCpuSpeed(240);
  if (!global_fs->exists("/sys")) global_fs->mkdir("/sys");
  File f = global_fs->open("/sys/pot_links.txt", FILE_WRITE);
  if (!f) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }
  
  for (size_t i = 0; i < potLinkAliases.size(); i++) {
    f.println(potLinkAliases[i] + "," + potLinkPaths[i]);
  }
  f.close();
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

#pragma region POTION
void terminalScrollPreview() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);

  int startLine = 0;
  if (termScrollIndex >= 1) {
    startLine = termScrollIndex - 1;
  }

  int y = 7; 
  for (int i = startLine; i < startLine + 4; i++) {
    if (i >= (int)terminalOutputs.size()) {
      break;
    }

    if (i == (int)termScrollIndex) {
      u8g2.drawTriangle(0, y - 6, 0, y, 4, y - 3);
    }

    u8g2.setFont(u8g2_font_5x7_tf);
    
    // Truncate preview for OLED
    String dispStr = terminalOutputs[i];
    if (dispStr.length() > 21) dispStr = dispStr.substring(0, 21);
    u8g2.drawUTF8(6, y, dispStr.c_str());

    y += 8;
  }

  u8g2.sendBuffer();
}

void potionScrollPreview() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);

  // Calculate sliding window to keep cursor vertically centered
  int startLine = 0;
  if (currentPotionLine >= 1) {
    startLine = currentPotionLine - 1;
  }

  int y = 7; // Baseline for the first row of text
  
  for (int i = startLine; i < startLine + 4; i++) {
    if (i >= (int)potionLines.size()) {
      break;
    }

    String lineNum = String(i);
    while (lineNum.length() < 3) {
      lineNum = "0" + lineNum;
    }
    lineNum = "[" + lineNum + "]";

    if (i == currentPotionLine) {
      // Draw left-aligned right-pointing triangle
      u8g2.drawTriangle(0, y - 6, 0, y, 4, y - 3);
    }

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawUTF8(6, y, lineNum.c_str());
    u8g2.drawUTF8(30, y, potionLines[i].c_str());

    y += 8;
  }

  u8g2.sendBuffer();
}

void loadPotionFile(String path) {
  potionLines.clear();
  pocketmage::setCpuSpeed(240);

  File file = global_fs->open(path);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  String line = "";
  while (file.available()) {
    char c = file.read();

    if (c == '\n') {
      if (line.endsWith("\r")) {
        line.remove(line.length() - 1);
      }
      potionLines.push_back(line);
      line = "";
    } else {
      line += c;
    }
  }

  if (line.length() > 0) {
    if (line.endsWith("\r")) {
      line.remove(line.length() - 1);
    }
    potionLines.push_back(line);
  }

  file.close();

  if (potionLines.size() == 0)
    potionLines.push_back("");

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void savePotionFile(String path) {
  pocketmage::setCpuSpeed(240);

  File file = global_fs->open(path, FILE_WRITE);
  if (!file) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  for (size_t i = 0; i < potionLines.size(); i++) {
    file.print(potionLines[i]);
    if (i < potionLines.size() - 1) {
      file.print('\n');
    }
  }

  file.close();
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  OLED().sysMessage("FILE SAVED",500);
}

void potionInit() {
  CurrentTERMfunc = POTION;
  newState = true;
  KB().setKeyboardState(NORMAL);
  lastInput = millis();
  if (editFile != "")
    loadPotionFile(editFile);
  else
    TERMINAL_INIT();
}

#pragma region TERMINAL
void updateTerminalDisp() {
  newState = false;
  uint16_t bgColor = termDarkTheme ? GxEPD_BLACK : GxEPD_WHITE;
  uint16_t fgColor = termDarkTheme ? GxEPD_WHITE : GxEPD_BLACK;

  display.fillRect(0, 0, display.width(), display.height(), bgColor);

  int maxScroll = 0;
  if (terminalOutputs.size() > termLinesPerPage) {
    maxScroll = (int)(terminalOutputs.size() - termLinesPerPage);
  }
  
  if (termScrollIndex > (ulong)maxScroll) termScrollIndex = maxScroll;

  int y = termLargeFont ? 14 : 10;
  int yStep = termLargeFont ? 16 : 10;
  int startIdx = (int)termScrollIndex;
  int endIdx = startIdx + termLinesPerPage;
  if (endIdx > (int)terminalOutputs.size()) endIdx = (int)terminalOutputs.size();

  for (int i = startIdx; i < endIdx; i++) {
    const String& s = terminalOutputs[i];
    display.setTextColor(fgColor);
    if (termLargeFont) display.setFont(&FreeMonoBold9pt7b);
    else display.setFont(&Font5x7Fixed);
    display.setCursor(5, y);
    printUTF8ToEink(s, termMaxLineLen); // Use the length limiter inside our renderer
    y += yStep;
  }

  // Draw Terminal Scrollbar
  if (maxScroll > 0) {
    int barWidth = 3;
    int barX = display.width() - barWidth - 1;
    
    float visibleRatio = (float)termLinesPerPage / terminalOutputs.size();
    int handleHeight = max((int)(display.height() * visibleRatio), 15);
    
    float scrollFraction = (float)termScrollIndex / maxScroll;
    int handleY = scrollFraction * (display.height() - handleHeight);
    
    display.fillRect(barX, handleY, barWidth, handleHeight, fgColor);

    // Ensure bar never visually touches top/bottom edges
    display.drawFastHLine(barX, display.height()-1,barWidth, bgColor);
    display.drawFastHLine(barX, 0,barWidth, bgColor);
  }

  display.setTextColor(GxEPD_BLACK);
  EINK().refresh();
}

bool deleteRecursive(String path) {
  File dir = global_fs->open(path);
  if (!dir) return false;
  if (!dir.isDirectory()) {
    dir.close();
    return global_fs->remove(path);
  }
  
  File file = dir.openNextFile();
  while (file) {
    String childPath = path;
    if (!childPath.endsWith("/")) childPath += "/";
    childPath += file.name();
    
    bool isDir = file.isDirectory();
    file.close();
    
    if (isDir) {
      deleteRecursive(childPath);
    } else {
      global_fs->remove(childPath);
    }
    file = dir.openNextFile();
  }
  dir.close();
  return global_fs->rmdir(path);
}

void funcSelect(String command) {
  String returnText = "";

  String totalMsg = currentDir + ">" + command;
  terminalOutputs.push_back(totalMsg);

  command.toLowerCase();

  // Clear command window
  if (command == "clear") {
    terminalOutputs.clear();
    termScrollIndex = 0;
    newState = true;
    return;
  }

  // Exit terminal
  else if (command == "exit" || command == "quit" || command == "q") {
    HOME_INIT();
    return;
  }

  // Help
  else if (command == "help") {
    terminalOutputs.push_back("Available commands:");
    terminalOutputs.push_back("ls                  List dir");
    terminalOutputs.push_back("cd <dir>          Change dir");
    terminalOutputs.push_back("rm <file>        Remove file");
    terminalOutputs.push_back("rm -r <dir>       Remove dir");
    terminalOutputs.push_back("cp <src> <dest>    Copy file");
    terminalOutputs.push_back("mv <src> <dest>    Mv/rename");
    terminalOutputs.push_back("touch <file>     Create file");
    terminalOutputs.push_back("mkdir <name>   Create folder");
    terminalOutputs.push_back("clear         Clear terminal");
    terminalOutputs.push_back("txt <file>       Open in TXT");
    terminalOutputs.push_back("potion/pot <file>  Edit prgm");
    terminalOutputs.push_back("brew <file>         Run prgm");
    terminalOutputs.push_back("pot link <file> <alias> link");
    terminalOutputs.push_back("pot unlink <alias>    unlink");
    terminalOutputs.push_back("setfont <l/s>  Set font size");
    terminalOutputs.push_back("theme <light/dark> Set theme");

    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;

  }

  // Enter directory
  else if (command.startsWith("cd")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(2);
    arg.trim();
    if (arg.length() == 0) {
      currentDir = "/";  // 'cd' alone returns to root
    } else {
      String newPath = arg;
      if (!newPath.startsWith("/")) {
        if (!currentDir.endsWith("/"))
          currentDir += "/";
        newPath = currentDir + newPath;
      }
      if (newPath.length() > 1 && newPath.endsWith("/")) {
        newPath.remove(newPath.length() - 1);
      }

      if (global_fs->exists(newPath)) {
        File f = global_fs->open(newPath);
        if (f && f.isDirectory()) {
          currentDir = newPath;
        } else {
          returnText = "cd: Not a directory";
        }
        if (f) f.close();
      } else {
        returnText = "cd: No such directory";
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // List directory
  else if (command.startsWith("ls")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(2);
    arg.trim();
    String listPath = currentDir;
    if (arg.length() > 0) {
      if (arg.startsWith("/"))
        listPath = arg;
      else {
        if (!currentDir.endsWith("/"))
          listPath = currentDir + "/";
        listPath += arg;
      }
    }

    if (global_fs->exists(listPath)) {
      File dir = global_fs->open(listPath);
      if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
          String lineOutput = "";
          if (file.isDirectory())
            lineOutput += "[DIR]";
          else
            lineOutput += "     ";
          lineOutput += file.name();
          if (!file.isDirectory()) {
            lineOutput += " * ";
            lineOutput += String(file.size()) + "b";
          }

          terminalOutputs.push_back(lineOutput);

          lineOutput = "";
          file = dir.openNextFile();
        }
        dir.close();
      } else {
        returnText = "ls: Not a directory";
      }
    } else {
      returnText = "ls: No such directory";
    }
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Make directory
  else if (command.startsWith("mkdir")) {
    pocketmage::setCpuSpeed(240);

    String arg = command.substring(5);
    arg.trim();
    String newDirPath = currentDir;
    if (arg.length() > 0) {
      if (arg.startsWith("/"))
        newDirPath = arg;
      else {
        if (!currentDir.endsWith("/"))
          newDirPath = currentDir + "/";
        newDirPath += arg;
      }
    } else {
      returnText = "Path not defined";
    }

    if (!global_fs->exists(newDirPath))
      global_fs->mkdir(newDirPath);
    currentDir = newDirPath;

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Remove directory
  else if (command.startsWith("rm -r")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(5);
    arg.trim();

    String dirPath = currentDir;
    if (arg.length() > 0) {
      if (arg.startsWith("/"))
        dirPath = arg;
      else {
        if (!currentDir.endsWith("/"))
          dirPath += "/";
        dirPath += arg;
      }
    } else {
      returnText = "Path not defined";
    }

    if (returnText == "" && global_fs->exists(dirPath)) {
      if (!deleteRecursive(dirPath)) {
        returnText = "Failed to remove directory";
      }
    } else if (returnText == "") {
      returnText = "Path not found";
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }

    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Remove file
  else if (command.startsWith("rm ") && !command.startsWith("rm -r")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(2);
    arg.trim();

    String dirPath = currentDir;
    if (arg.length() > 0) {
      if (arg.startsWith("/"))
        dirPath = arg;
      else {
        if (!currentDir.endsWith("/"))
          dirPath += "/";
        dirPath += arg;
      }
    } else {
      returnText = "Path not defined";
    }

    if (returnText == "" && global_fs->exists(dirPath)) {
      File f = global_fs->open(dirPath);
      if (!f) {
        returnText = "Failed to open file";
      } else if (f.isDirectory()) {
        returnText = "Not a file - use <rm -r>";
      } else {
        f.close();
        if (!global_fs->remove(dirPath))
          returnText = "Delete failed";
      }
      if (f) f.close();
    } else if (returnText == "") {
      returnText = "Path not found";
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Copy file
  else if (command.startsWith("cp ")) {
    pocketmage::setCpuSpeed(240);

    String args = command.substring(3);
    args.trim();

    int spaceIdx = args.indexOf(' ');
    if (spaceIdx == -1) {
      returnText = "Usage: cp <src> <dest>";
    } else {
      String src = args.substring(0, spaceIdx);
      String dest = args.substring(spaceIdx + 1);
      src.trim();
      dest.trim();

      String srcPath = src.startsWith("/") ? src : (currentDir + (currentDir.endsWith("/") ? "" : "/") + src);
      String destPath = dest.startsWith("/") ? dest : (currentDir + (currentDir.endsWith("/") ? "" : "/") + dest);

      if (!global_fs->exists(srcPath)) {
        returnText = "Source not found";
      } else {
        File srcFile = global_fs->open(srcPath, FILE_READ);
        if (!srcFile || srcFile.isDirectory()) {
          returnText = "Source is not a file";
        } else {
          File destFile = global_fs->open(destPath, FILE_WRITE);
          if (!destFile) {
            returnText = "Failed to create destination";
          } else {
            uint8_t buf[512];
            size_t n;
            while ((n = srcFile.read(buf, sizeof(buf))) > 0) {
              destFile.write(buf, n);
            }
            destFile.close();
          }
          srcFile.close();
        }
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Move file
  else if (command.startsWith("mv ")) {
    pocketmage::setCpuSpeed(240);

    String args = command.substring(3);
    args.trim();

    int spaceIdx = args.indexOf(' ');
    if (spaceIdx == -1) {
      returnText = "Usage: mv <src> <dest>";
    } else {
      String src = args.substring(0, spaceIdx);
      String dest = args.substring(spaceIdx + 1);
      src.trim();
      dest.trim();

      String srcPath = src.startsWith("/") ? src : (currentDir + (currentDir.endsWith("/") ? "" : "/") + src);
      String destPath = dest.startsWith("/") ? dest : (currentDir + (currentDir.endsWith("/") ? "" : "/") + dest);

      if (!global_fs->exists(srcPath)) {
        returnText = "Source not found";
      } else {
        if (!global_fs->rename(srcPath, destPath)) {
          File srcFile = global_fs->open(srcPath, FILE_READ);
          if (!srcFile || srcFile.isDirectory()) {
            returnText = "Source is not a file";
          } else {
            File destFile = global_fs->open(destPath, FILE_WRITE);
            if (!destFile) {
              returnText = "Failed to create destination";
            } else {
              uint8_t buf[512];
              size_t n;
              while ((n = srcFile.read(buf, sizeof(buf))) > 0) {
                destFile.write(buf, n);
              }
              destFile.close();
              global_fs->remove(srcPath);
            }
            srcFile.close();
          }
        }
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Create empty file (touch)
  else if (command.startsWith("touch ")) {
    pocketmage::setCpuSpeed(240);

    String arg = command.substring(6);
    arg.trim();

    if (arg.length() == 0) {
      returnText = "Usage: touch <file>";
    } else {
      String filePath = arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

      if (global_fs->exists(filePath)) {
        File f = global_fs->open(filePath);
        if (f && f.isDirectory()) {
          returnText = "Is a directory";
        }
        if (f) f.close();
      } else {
        File f = global_fs->open(filePath, FILE_WRITE);
        if (!f) {
          returnText = "Failed to create file";
        } else {
          f.close();
        }
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Open in text editor
  else if (command.startsWith("txt ")) {
    pocketmage::setCpuSpeed(240);

    String arg = command.substring(4);
    arg.trim();

    if (arg.length() == 0) {
      returnText = "Usage: txt <filename>";
    } else {
      if (!arg.endsWith(".txt")) {
        int dotIdx = arg.lastIndexOf('.');
        if (dotIdx != -1) {
          returnText = "Only .txt files supported";
        } else {
          arg += ".txt";
        }
      }

      if (returnText == "") {
        String filePath = arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

        if (!global_fs->exists(filePath)) {
          returnText = "File not found";
        } else {
          PM_SDAUTO().setEditingFile(filePath);
          OLED().oledWord("Opening: " + PM_SDAUTO().getEditingFile());
          if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
          delay(1000);
          TXT_INIT(filePath);
          return;
        }
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Link Program
  else if (command.startsWith("pot link ")) {
    pocketmage::setCpuSpeed(240);

    String args = command.substring(9);
    args.trim();

    int spaceIdx = args.indexOf(' ');
    if (spaceIdx == -1) {
      returnText = "Usage: pot link <file> <alias>";
    } else {
      String fileArg = args.substring(0, spaceIdx);
      String aliasArg = args.substring(spaceIdx + 1);
      fileArg.trim();
      aliasArg.trim();

      String filePath = fileArg.startsWith("/") ? fileArg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + fileArg);
      
      if (!filePath.endsWith(".c")) {
        int dotIdx = fileArg.lastIndexOf('.');
        if (dotIdx == -1) filePath += ".c";
      }

      if (!global_fs->exists(filePath)) {
        returnText = "File not found";
      } else {
        // Find existing or add new
        int existingIdx = -1;
        for(size_t i=0; i<potLinkAliases.size(); i++){
            if(potLinkAliases[i] == aliasArg){ existingIdx = i; break; }
        }
        if(existingIdx != -1){
            potLinkPaths[existingIdx] = filePath;
        } else {
            potLinkAliases.push_back(aliasArg);
            potLinkPaths.push_back(filePath);
        }
        
        savePotLinks();
        returnText = "Linked " + aliasArg + " -> " + fileArg;
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText, 1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Unlink Program
  else if (command.startsWith("pot unlink ")) {
    pocketmage::setCpuSpeed(240);
    
    String aliasArg = command.substring(11);
    aliasArg.trim();

    int linkIdx = -1;
    for(size_t i=0; i<potLinkAliases.size(); i++){
        if(potLinkAliases[i] == aliasArg){ linkIdx = i; break; }
    }

    if (linkIdx != -1) {
      potLinkAliases.erase(potLinkAliases.begin() + linkIdx);
      potLinkPaths.erase(potLinkPaths.begin() + linkIdx);
      savePotLinks();
      returnText = "Unlinked " + aliasArg;
    } else {
      returnText = "Link not found";
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText, 1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Open in potion
  else if (command.startsWith("potion") || command.startsWith("pot")) {
    pocketmage::setCpuSpeed(240);

    String arg = "";
    if (command.startsWith("potion"))
      arg = command.substring(6);
    else if (command.startsWith("pot"))
      arg = command.substring(3);
    arg.trim();

    if (arg.length() == 0) {
      returnText = "Usage: potion <filename>";
    } else {
      String filePath = arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

      if (!filePath.endsWith(".c") && !filePath.endsWith(".txt")) {
        int dotIdx = arg.lastIndexOf('.');
        if (dotIdx != -1) {
          returnText = "Only .c and .txt files supported";
        } else {
          bool hasC = global_fs->exists(filePath + ".c");
          bool hasTxt = global_fs->exists(filePath + ".txt");

          if (hasTxt && !hasC) {
            filePath += ".txt";
          } else {
            filePath += ".c"; 
          }
        }
      }

      if (returnText == "") {
        if (!global_fs->exists(filePath)) {
          returnText = "File not found";
        } else {
          editFile = filePath;
          if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
          potionInit();
          return;
        }
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Compile program
  else if (command.startsWith("brew")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(4);
    arg.trim();

    if (arg.length() == 0) {
      returnText = "Usage: brew <filename>";
    } else {
      if (!arg.endsWith(".c")) {
        int dotIdx = arg.lastIndexOf('.');
        if (dotIdx != -1) {
          returnText = "Only .c files supported";
        } else {
          arg += ".c";
        }
      }

      if (returnText == "") {
        String filePath = arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

        if (!global_fs->exists(filePath)) {
          returnText = "File not found";
        } 
        else {
          const char* wrenchCode = readCFile(filePath);
          if (wrenchCode) {
            compileWrench(wrenchCode);
            free((void*)wrenchCode);
          } else {
            returnText = "Failed to read or empty file";
          }

          if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
          termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
          return;
        }
      }
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText,1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Set font
  else if (command.startsWith("setfont ")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(8);
    arg.trim();
    if (arg == "l") {
      termLargeFont = true;
      termLinesPerPage = 14;
      termMaxLineLen = 28;
      returnText = "Font set to Large";
    } else if (arg == "s") {
      termLargeFont = false;
      termLinesPerPage = 23; 
      termMaxLineLen = 52;
      returnText = "Font set to Small";
    } else {
      returnText = "Usage: setfont <l/s>";
    }

    if (returnText.startsWith("Font")) {
      prefs.begin("PocketMage", false);
      prefs.putBool("termLargeFont", termLargeFont);
      prefs.end();
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText, 1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Theme
  else if (command.startsWith("theme ")) {
    pocketmage::setCpuSpeed(240);
    String arg = command.substring(6);
    arg.trim();
    if (arg == "light" || arg == "l") {
      termDarkTheme = false;
      returnText = "Theme set to Light";
    } else if (arg == "dark" || arg == "d") {
      termDarkTheme = true;
      returnText = "Theme set to Dark";
    } else {
      returnText = "Usage: theme <light/dark>";
    }

    if (returnText.startsWith("Theme")) {
      prefs.begin("PocketMage", false);
      prefs.putBool("termDarkTheme", termDarkTheme);
      prefs.end();
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText, 1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Check whether command is a linked alias
  int resolvedLinkIdx = -1;
  for(size_t i=0; i<potLinkAliases.size(); i++){
      if(potLinkAliases[i] == command){ resolvedLinkIdx = i; break; }
  }
  
  if (resolvedLinkIdx != -1) {
    pocketmage::setCpuSpeed(240);
    String filePath = potLinkPaths[resolvedLinkIdx];

    if (!global_fs->exists(filePath)) {
      returnText = "Linked file missing";
    } else {
      const char* wrenchCode = readCFile(filePath);
      if (wrenchCode) {
        compileWrench(wrenchCode);
        free((void*)wrenchCode);
      } else {
        returnText = "Failed to read linked file";
      }
      if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
      return;
    }

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().sysMessage(returnText, 1000);
    }
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }

  // Check whether command is a home/settings command
  returnText = commandSelect(command);
  if (returnText != "") {
    terminalOutputs.push_back(returnText);
    termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
    newState = true;
    return;
  }
}

#pragma region BREW
// ---------- Wrench functions ---------- //
// ----- In/Out ----- //
void wr_oledWord(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[1024];

  const char* s = argv[0].asString(buf, 1024);
  OLED().oledWord(s);
}

void wr_print(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[1024];

  const char* s = argv[0].asString(buf, 1024);

  terminalOutputs.push_back(s);
  termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
}

void wr_prompt(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char inbuf[1024];
  char retbuf[1024];

  const char* promptText = argv[0].asString(inbuf, 1024);

  String entered = textPrompt(promptText);
  if (entered == "_RETURN_") return;
  else if (entered == "_EXIT_") entered = "";
  entered.toCharArray(retbuf, sizeof(retbuf));

  wr_makeString(c, &ret, retbuf);
}

void wr_boolPrompt(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[128];
  int result = boolPrompt(argv[0].asString(buf, 128));
  wr_makeInt(&ret, result);
}

void wr_timePrompt(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int defaultTime = argv[0].asInt();
  wr_makeInt(&ret, timePrompt(defaultTime));
}

void wr_datePrompt(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[16];
  char retbuf[16];
  String defaultDate = argv[0].asString(buf, 16);
  String result = datePrompt(defaultDate);
  result.toCharArray(retbuf, sizeof(retbuf));
  wr_makeString(c, &ret, retbuf);
}

void wr_waitForKeypress(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[128];
  waitForKeypress(argv[0].asString(buf, 128));
}

void wr_updateTerm(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  updateTerminalDisp();
}

// ----- E-Ink Display ----- //
void wr_updateInk(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  EINK().refresh();
}

void wr_inkBackground(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  bool bgColor = (argv[0].asInt() == 0);

  display.fillScreen(bgColor);
}

void wr_inkRect(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int x_origin      = argv[0].asInt();
  int y_origin      = argv[1].asInt();
  int width         = argv[2].asInt();
  int height        = argv[3].asInt();
  bool borderColor  = (argv[4].asInt() == 0);
  bool fillColor    = (argv[5].asInt() == 0);
  
  if ((fillColor && borderColor) || (!fillColor && !borderColor)) {
    display.fillRect(x_origin, y_origin, width, height, !fillColor);
  }
  else if (borderColor && !fillColor) {
    display.drawRect(x_origin, y_origin, width, height, 1);
  }
  else if (!borderColor && fillColor) {
    display.drawRect(x_origin, y_origin, width, height, 0);
    display.fillRect(x_origin+1, y_origin+1, width-2, height-2, 1);
  }
}

void wr_inkCircle(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int x_origin      = argv[0].asInt();
  int y_origin      = argv[1].asInt();
  int radius        = argv[2].asInt();
  bool borderColor  = (argv[3].asInt() == 0);
  bool fillColor    = (argv[4].asInt() == 0);
  
  if ((fillColor && borderColor) || (!fillColor && !borderColor)) {
    display.fillCircle(x_origin, y_origin, radius, fillColor);
  }
  else if (borderColor && !fillColor) {
    display.drawCircle(x_origin, y_origin, radius, 1);
  }
  else if (!borderColor && fillColor) {
    display.drawCircle(x_origin, y_origin, radius, 0);
    display.fillCircle(x_origin, y_origin, radius-2, 1);
  }
}

void wr_inkText(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[1024];

  int x_origin      = argv[0].asInt();
  int y_origin      = argv[1].asInt();
  int size          = argv[2].asInt();
  bool color        = (argv[3].asInt() != 0);
  const char* text  = argv[4].asString(buf, 1024);
  
  if (color) display.setTextColor(GxEPD_BLACK);
  else display.setTextColor(GxEPD_WHITE);

  switch (size) {
    case 1:
      display.setFont(&Font5x7Fixed);
      break;
    case 2:
      display.setFont(&FreeMonoBold9pt7b);
      break;
    case 3:
      display.setFont(&FreeMonoBold12pt7b);
      break;
    default:
      display.setFont(&FreeMonoBold9pt7b);
      break;
  }
  
  display.setCursor(x_origin, y_origin);
  printUTF8ToEink(text);
}

// ----- OLED Display ----- //
void wr_updateOled(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  u8g2.sendBuffer();
  u8g2.clearBuffer();
}

void wr_oledBackground(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  bool bgColor = (argv[0].asInt() == 0);

  u8g2.setDrawColor(bgColor);
  u8g2.drawBox(0,0,u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
  u8g2.setDrawColor(1);
}

void wr_oledRect(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int x_origin      = argv[0].asInt();
  int y_origin      = argv[1].asInt();
  int width         = argv[2].asInt();
  int height        = argv[3].asInt();
  bool borderColor  = (argv[4].asInt() == 0);
  bool fillColor    = (argv[5].asInt() == 0);
  
  if ((fillColor && borderColor) || (!fillColor && !borderColor)) {
    u8g2.setDrawColor(!fillColor);
    u8g2.drawBox(x_origin, y_origin, width, height);
  }
  else if (borderColor && !fillColor) {
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x_origin, y_origin, width, height);
  }
  else if (!borderColor && fillColor) {
    u8g2.setDrawColor(0);
    u8g2.drawFrame(x_origin, y_origin, width, height);
    u8g2.setDrawColor(1);
    u8g2.drawBox(x_origin+1, y_origin+1, width-2, height-2);
  }

  u8g2.setDrawColor(1);
}

void wr_oledCircle(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int x_origin      = argv[0].asInt();
  int y_origin      = argv[1].asInt();
  int radius        = argv[2].asInt();
  bool borderColor  = (argv[3].asInt() == 0);
  
  u8g2.setDrawColor(borderColor);
  u8g2.drawCircle(x_origin, y_origin, radius);

  u8g2.setDrawColor(1);
}

void wr_oledText(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[1024];

  int x_origin      = argv[0].asInt();
  int y_origin      = argv[1].asInt();
  int size          = argv[2].asInt();
  bool color        = (argv[3].asInt() == 0);
  const char* text  = argv[4].asString(buf, 1024);
  
  u8g2.setDrawColor(color);

  switch (size) {
    case 1:
      u8g2.setFont(u8g2_font_5x7_tf);
      break;
    case 2:
      u8g2.setFont(u8g2_font_7x13B_tf);
      break;
    case 3:
      u8g2.setFont(u8g2_font_helvB14_tf);
      break;
    default:
      u8g2.setFont(u8g2_font_lubR18_tf);
      break;
  }
  
  u8g2.drawUTF8(x_origin, y_origin, text);

  u8g2.setDrawColor(1);
}

void wr_sysMessage(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char buf[128];
  const char* msg = argv[0].asString(buf, 128);
  int delayMs = argv[1].asInt();
  
  OLED().sysMessage(String(msg), delayMs);
}

// ----- Hardware / System ----- //
void wr_getKey(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char key = KB().updateKeypress();
  wr_makeInt(&ret, (int)key);
}

void wr_getTouch(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int vector = TOUCH().getScrollVector();
  wr_makeInt(&ret, vector);
}

void wr_getBattery(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  extern volatile int battState;
  wr_makeInt(&ret, battState);
}

void wr_getBatteryVoltage(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  wr_makeFloat(&ret, getBatteryVoltage());
}

void wr_setCpuSpeed(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int freq = argv[0].asInt();
  pocketmage::setCpuSpeed(freq);
}

void wr_sleep(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  pocketmage::deepSleep(false);
}

void wr_getTime(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  DateTime now = CLOCK().nowDT();
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  wr_makeString(c, &ret, buf);
}

void wr_getDate(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  DateTime now = CLOCK().nowDT();
  char buf[16];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", now.year(), now.month(), now.day());
  wr_makeString(c, &ret, buf);
}

// ----- Filesystem ----- //
void wr_readFile(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  pocketmage::setCpuSpeed(240);
  char path[256];
  argv[0].asString(path, 256);
  String content = PM_SDAUTO().readFileToString(*global_fs, path);
  wr_makeString(c, &ret, content.c_str());
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void wr_writeFile(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  pocketmage::setCpuSpeed(240);
  char path[256];
  char content[1024];
  argv[0].asString(path, 256);
  argv[1].asString(content, 1024);
  PM_SDAUTO().writeFile(*global_fs, path, content);
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

// ----- Helpers ----- //
void wr_delay(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  if (argv[0].asInt() > 0) {
    delay(argv[0].asInt());
  }
}

void wr_toInt(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char inbuf[1024];

  const char* inString = argv[0].asString(inbuf, 1024);
  
  wr_makeInt(&ret, atoi(inString));
}

void wr_random(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  int min = argv[0].asInt();
  int max = argv[1].asInt();

  wr_makeInt(&ret, ((esp_random() % ((max-min)+1)) + min));
}

// ----- Wrench Functions ----- //
const char* readCFile(const String& path) {
  File f = global_fs->open(path);
  if (!f || f.isDirectory()) {
    if(f) f.close();
    return nullptr;
  }

  size_t len = f.size();
  if (len == 0) {
    f.close();
    return nullptr;
  }

  char* buf = (char*)malloc(len + 1);
  if (!buf) {
    f.close();
    return nullptr;
  }

  size_t readBytes = f.readBytes(buf, len);
  buf[readBytes] = '\0';
  f.close();

  return buf;
}

void compileWrench(const char* wrenchCode) {
  WRState* w = wr_newState();

  wr_registerFunction(w, "oledWord", wr_oledWord);
  wr_registerFunction(w, "print", wr_print);
  wr_registerFunction(w, "prompt", wr_prompt);
  wr_registerFunction(w, "updateTerm", wr_updateTerm);
  wr_registerFunction(w, "delay", wr_delay);
  wr_registerFunction(w, "toInt", wr_toInt);
  wr_registerFunction(w, "inkCircle", wr_inkCircle);
  wr_registerFunction(w, "inkRect", wr_inkRect);
  wr_registerFunction(w, "updateInk", wr_updateInk);
  wr_registerFunction(w, "inkBackground", wr_inkBackground);
  wr_registerFunction(w, "inkText", wr_inkText);
  wr_registerFunction(w, "oledCircle", wr_oledCircle);
  wr_registerFunction(w, "oledRect", wr_oledRect);
  wr_registerFunction(w, "updateOled", wr_updateOled);
  wr_registerFunction(w, "oledBackground", wr_oledBackground);
  wr_registerFunction(w, "oledText", wr_oledText);
  wr_registerFunction(w, "random", wr_random);
  wr_registerFunction(w, "sysMessage", wr_sysMessage);
  wr_registerFunction(w, "getKey", wr_getKey);
  wr_registerFunction(w, "getTouch", wr_getTouch);
  wr_registerFunction(w, "getBattery", wr_getBattery);
  wr_registerFunction(w, "getBatteryVoltage", wr_getBatteryVoltage);
  wr_registerFunction(w, "setCpuSpeed", wr_setCpuSpeed);
  wr_registerFunction(w, "sleep", wr_sleep);
  wr_registerFunction(w, "getTime", wr_getTime);
  wr_registerFunction(w, "getDate", wr_getDate);
  wr_registerFunction(w, "readFile", wr_readFile);
  wr_registerFunction(w, "writeFile", wr_writeFile);
  wr_registerFunction(w, "boolPrompt", wr_boolPrompt);
  wr_registerFunction(w, "timePrompt", wr_timePrompt);
  wr_registerFunction(w, "datePrompt", wr_datePrompt);
  wr_registerFunction(w, "waitForKeypress", wr_waitForKeypress);

  unsigned char* outBytes;
  int outLen;

  WRstr errMsg;
  int err = wr_compile(wrenchCode, strlen(wrenchCode), &outBytes, &outLen, &errMsg);

  if (err == 0) {
    wr_run(w, outBytes, outLen, true);
  }

  if (errMsg.c_str() && errMsg[0] != '\0') {
    const char* p = errMsg;
    const char* lineStart = p;

    while (*p) {
      if (*p == '\n') {
        int len = p - lineStart;
        if (len > 0 && lineStart[len - 1] == '\r') {
          len--;
        }

        terminalOutputs.push_back(String(lineStart).substring(0, len));

        lineStart = p + 1;
      }
      p++;
    }

    if (lineStart != p) {
      int len = p - lineStart;
      terminalOutputs.push_back(String(lineStart).substring(0, len));
    }
  }

  wr_destroyState(w);

  termScrollIndex = terminalOutputs.size() > termLinesPerPage ? terminalOutputs.size() - termLinesPerPage : 0;
  newState = true;
}

#pragma region MAIN
void TERMINAL_INIT() {
  CurrentAppState = TERMINAL;
  CurrentTERMfunc = PROMPT;
  potionLines.clear();
  potionLines.push_back("");
  terminalCommand = "";
  
  prefs.begin("PocketMage", true);
  termLargeFont = prefs.getBool("termLargeFont", true);
  termDarkTheme = prefs.getBool("termDarkTheme", true);
  prefs.end();

  if (termLargeFont) {
    termLinesPerPage = 14;
    termMaxLineLen = 28;
  } else {
    termLinesPerPage = 23;
    termMaxLineLen = 52;
  }
  
  if (terminalOutputs.size() > termLinesPerPage) {
      termScrollIndex = (ulong)(terminalOutputs.size() - termLinesPerPage);
  } else {
      termScrollIndex = 0;
  }
  
  KB().setKeyboardState(NORMAL);
  if (!potLinksLoaded) loadPotLinks();
  newState = true;
}

void processKB_TERMINAL() {
  // Prompt
  String outLine = "";
  String command = "";

  // Potion
  static int cursor_pos = 0;
  int currentMillis = millis();

  switch (CurrentTERMfunc) {
    case PROMPT:
    {
      pocketmage::setCpuSpeed(240);
      char inchar = KB().updateKeypress();
      if (inchar == 0) {
        if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      }

      int maxScroll = 0;
      if (terminalOutputs.size() > termLinesPerPage) {
          maxScroll = (int)(terminalOutputs.size() - termLinesPerPage);
      }

      ulong maxS = (ulong)maxScroll;
      // Handle Terminal Hardware Scrolling
      if (TOUCH().updateScroll(maxS, termScrollIndex)) {
        newState = true;
      }

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        if (inchar != 0) {
          lastInput = millis();
          KBBounceMillis = currentMillis;

          // Make typing jump to the newest line
          if (termScrollIndex != (ulong)maxScroll) {
            termScrollIndex = maxScroll;
            newState = true;
          }

          if (inchar == 13) { // CR
            funcSelect(terminalCommand);
            terminalCommand = "";
            
            if (terminalOutputs.size() > termLinesPerPage) {
                termScrollIndex = (ulong)(terminalOutputs.size() - termLinesPerPage);
            } else {
                termScrollIndex = 0;
            }
            
            KB().setKeyboardState(NORMAL);
            newState = true;
          }
          else if (inchar == 17) { // SHIFT
            if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(SHIFT);
          }
          else if (inchar == 18) { // FUNC
            if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(FUNC);
          }
          else if (inchar == 8) { // BKSP
            if (terminalCommand.length() > 0) {
              terminalCommand.remove(terminalCommand.length() - 1);
            }
          }
          else if (inchar == 12) { // FN+LEFT (Escape/Home)
            HOME_INIT();
            break;
          }
          else if (inchar >= 32 && inchar <= 126) {
            terminalCommand += inchar;
            if (inchar >= 48 && inchar <= 57) {} // keep FN on for numbers
            else if (KB().getKeyboardState() != NORMAL) KB().setKeyboardState(NORMAL);
          }
        }
      }

      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        
        if (TOUCH().getLastTouch() == -1) {
          OLED().oledLine(terminalCommand, terminalCommand.length(), false, currentDir + "> ");
        } else {
          terminalScrollPreview();
        }
      }
      break;
    }

    case POTION:
      String left = "";
      String right = "";

      // 1. Drain the hardware buffer continuously at loop speed
      pocketmage::setCpuSpeed(240);
      char inchar = KB().updateKeypress();
      if (inchar == 0) {
        if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      }

      // update scroll (Independent of keyboard debounce)
      if (TOUCH().updateScroll(potionLines.size() - 1, currentPotionLine)) {
        // Put cursor at the end
        cursor_pos = potionLines[currentPotionLine].length();
        newState = true;
      }

      // 2. Only process the actual input if the cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        if (inchar != 0) {
          lastInput = millis();
          KBBounceMillis = currentMillis;

          // HANDLE INPUTS
          // CR Recieved
          if (inchar == 13) {
            // Add a line and go to it
            if (cursor_pos == 0 && potionLines[currentPotionLine].length() > 0) {
              potionLines.insert(potionLines.begin() + currentPotionLine, "");
            }
            else {
              potionLines.insert(potionLines.begin() + currentPotionLine + 1, "");
              currentPotionLine++;
            }
            KB().setKeyboardState(NORMAL);
            cursor_pos = 0;
            newState = true;
            break;
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
            if (potionLines[currentPotionLine].length() > 0 && cursor_pos != 0) {
              uint16_t old_cursor = cursor_pos;
              // Safely leap over UTF-8 continuation bytes
              do { cursor_pos--; } while (cursor_pos > 0 && (potionLines[currentPotionLine][cursor_pos] & 0xC0) == 0x80);
              
              int bytesToDelete = old_cursor - cursor_pos;
              potionLines[currentPotionLine].remove(cursor_pos, bytesToDelete);
            }
            else if (potionLines[currentPotionLine].length() == 0) {
              if (potionLines.size() > 1) {
                potionLines.erase(potionLines.begin() + currentPotionLine);
                
                // If the deleted line was the last line, shift the active index up
                if (currentPotionLine >= potionLines.size()) {
                  currentPotionLine--;
                }
                // Safely clamp cursor
                cursor_pos = potionLines[currentPotionLine].length(); 
                newState = true;
              }
            }
          }
          // LEFT
          else if (inchar == 19) {
            if (cursor_pos > 0) {
              do { cursor_pos--; } while (cursor_pos > 0 && (potionLines[currentPotionLine][cursor_pos] & 0xC0) == 0x80);
            }
          }
          // RIGHT
          else if (inchar == 21) {
            if (cursor_pos < potionLines[currentPotionLine].length()) {
              do { cursor_pos++; } while (cursor_pos < potionLines[currentPotionLine].length() && (potionLines[currentPotionLine][cursor_pos] & 0xC0) == 0x80);
            }
          }
          // CENTER
          else if (inchar == 20) {
            KB().setKeyboardState(FUNC);
            command = textPrompt("GOTO LINE:");
            if (command == "_RETURN_") return;
            else if (command != "_EXIT_") {
              int line = atoi(command.c_str());
              // Line is in bounds
              if (line >= 0 && line < potionLines.size()) {
                currentPotionLine = line;
              }
              else if (line < 0) currentPotionLine = 0;
              else if (line >= potionLines.size()) currentPotionLine = potionLines.size() - 1;
              
              cursor_pos = potionLines[currentPotionLine].length();
              newState = true;
            }
            KB().setKeyboardState(NORMAL);
          }
          // SHIFT+LEFT
          else if (inchar == 28) {
            cursor_pos = 0;
            KB().setKeyboardState(NORMAL);
          }
          // SHIFT+RIGHT
          else if (inchar == 30) {
            cursor_pos = potionLines[currentPotionLine].length();
            KB().setKeyboardState(NORMAL);
          }
          // SHIFT+CENTER
          else if (inchar == 29) {
            KB().setKeyboardState(NORMAL);
          }
          // FN+LEFT
          else if (inchar == 12) {
            TERMINAL_INIT();
            break;
          }
          // FN+RIGHT
          else if (inchar == 6) {
            if (editFile != "")
              savePotionFile(editFile);
              newState = true;
            break;
          }
          // FN+CENTER
          else if (inchar == 7) {
            potionLines[currentPotionLine] = "";
            cursor_pos = 0;
            KB().setKeyboardState(NORMAL);
          }
          // FN+SHIFT+LEFT
          else if (inchar == 24) {
            KB().setKeyboardState(NORMAL);
          }
          // FN+SHIFT+RIGHT
          else if (inchar == 26) {
            KB().setKeyboardState(NORMAL);
          }
          // FN+SHIFT+CENTER
          else if (inchar == 25) {
            KB().setKeyboardState(NORMAL);
          }
          // TAB, SHIFT+TAB / FN+TAB, FN+SHIFT+TAB
          else if (inchar == 9 || inchar == 14) {
            potionLines[currentPotionLine] = "  " + potionLines[currentPotionLine];
            cursor_pos += 2;
          } 
          else {
            // split line at cursor_pos
            if (cursor_pos == 0) {
              potionLines[currentPotionLine] = inchar + potionLines[currentPotionLine];
            } else if (cursor_pos == potionLines[currentPotionLine].length()) {
              potionLines[currentPotionLine] += inchar;
            } else {
              left = potionLines[currentPotionLine].substring(0, cursor_pos);
              right = potionLines[currentPotionLine].substring(cursor_pos);
              potionLines[currentPotionLine] = left + inchar + right;
            }
            cursor_pos++;
            if (inchar >= 48 && inchar <= 57) {
            }  // Only leave FN on if typing numbers
            else if (KB().getKeyboardState() != NORMAL) {
              KB().setKeyboardState(NORMAL);
            }
          }
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        
        if (CurrentTERMfunc == POTION) { // Verify we didnt just exit back to PROMPT
          if (TOUCH().getLastTouch() == -1) {
            String lineNum = String(currentPotionLine);
            while (lineNum.length() < 3) {
              lineNum = "0" + lineNum;
            }
            String cursor = String(cursor_pos);
            while (cursor.length() < 2) {
              cursor = "0" + cursor;
            }

            String promptText = "[" + lineNum + "][" + cursor + "] - " + editFile;
            OLED().oledLine(potionLines[currentPotionLine], cursor_pos, false, promptText);
          } else {
            // Scrolling display function
            lastInput = millis();
            potionScrollPreview();
          }
        }
      }

      break;
  }
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void einkHandler_TERMINAL() {
  switch (CurrentTERMfunc) {
    case PROMPT:
      if (newState) {
        updateTerminalDisp();
      }
      break;
    case POTION:
      if (newState) {
        newState = false;
        uint16_t bgColor = termDarkTheme ? GxEPD_BLACK : GxEPD_WHITE;
        uint16_t fgColor = termDarkTheme ? GxEPD_WHITE : GxEPD_BLACK;
        display.fillRect(0, 0, display.width(), display.height(), bgColor);

        if (potionLines.size() < 24) {
          int y = 10;
          for (size_t i = 0; i < potionLines.size(); i++) {
            const String& s = potionLines[i];

            String lineNum = String(i);
            while (lineNum.length() < 3) {
              lineNum = "0" + lineNum;
            }

            if (i == currentPotionLine) {
              display.fillRect(0, y - 9, display.width(), 11, fgColor);
              display.setTextColor(bgColor);
            } else
              display.setTextColor(fgColor);
            display.setFont(&Font5x7Fixed);
            display.setCursor(5, y);
            display.print("[" + lineNum + "]");
            display.setCursor(35, y);
            printUTF8ToEink(s);
            y += 10;
          }
        } 
        else {
          if (currentPotionLine <= 20) {
            int y = 10;
            for (size_t i = 0; i < potionLines.size(); i++) {
              if (i >= potionLines.size() || y < 0 || y > (display.height()+10)) continue;

              const String& s = potionLines[i];

              String lineNum = String(i);
              while (lineNum.length() < 3) {
                lineNum = "0" + lineNum;
              }

              if (i == currentPotionLine) {
                display.fillRect(0, y - 9, display.width(), 11, fgColor);
                display.setTextColor(bgColor);
              } else
                display.setTextColor(fgColor);
              display.setFont(&Font5x7Fixed);
              display.setCursor(5, y);
              display.print("[" + lineNum + "]");
              display.setCursor(35, y);
              printUTF8ToEink(s);
              y += 10;
            }
          }
          else {
            int y = 10;
            for (size_t i = currentPotionLine - 20; i < currentPotionLine + 3; i++) {
              if (i >= potionLines.size() || y < 0 || y > (display.height()+10)) continue;

              const String& s = potionLines[i];

              String lineNum = String(i);
              while (lineNum.length() < 3) {
                lineNum = "0" + lineNum;
              }

              if (i == currentPotionLine) {
                display.fillRect(0, y - 9, display.width(), 11, fgColor);
                display.setTextColor(bgColor);
              } else
                display.setTextColor(fgColor);
              display.setFont(&Font5x7Fixed);
              display.setCursor(5, y);
              display.print("[" + lineNum + "]");
              display.setCursor(35, y);
              printUTF8ToEink(s);
              y += 10;
            }
          }
        }

        EINK().refresh();
        display.setTextColor(GxEPD_BLACK);
      }

      break;
  }
}

#endif
#include <globals.h>

#include "wrench.h"

// General
enum TERMINAL_functions { PROMPT, POTION };
TERMINAL_functions CurrentTERMfunc = PROMPT;

// Terminal
static std::vector<String> terminalOutputs;
static String currentDir = "/";

// Potion
static String editFile = "";
static ulong currentPotionLine = 0;
static std::vector<String> potionLines;
static long lastInput = millis();

// Functions
#pragma region POTION
void potionScrollPreview() {
  u8g2.clearBuffer();

  if (currentPotionLine < 2) {
    int y = 8;
    for (int i = 0; i < potionLines.size(); i++) {
      // Skip out of bounds lines
      if (i < 0 || i > (potionLines.size() - 1)) {
        continue;
      }

      // Draw each line
      String lineNum = String(i);
      while (lineNum.length() < 3) {
        lineNum = "0" + lineNum;
      }
      lineNum = "[" + lineNum + "]";

      if (i == currentPotionLine) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(0, y - 8, u8g2.getDisplayWidth(), 9);
        u8g2.setDrawColor(0);
      } else
        u8g2.setDrawColor(1);

      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(0, y, lineNum.c_str());
      u8g2.drawStr(35, y, potionLines[i].c_str());

      y += 8;
    }
  } else {
    int y = 8;
    for (int i = (currentPotionLine - 2); i <= (currentPotionLine + 1); i++) {
      // Skip out of bounds lines
      if (i < 0 || i > (potionLines.size() - 1) || y > (u8g2.getDisplayHeight() + 10)) {
        continue;
      }
      // Draw each line
      String lineNum = String(i);
      while (lineNum.length() < 3) {
        lineNum = "0" + lineNum;
      }
      lineNum = "[" + lineNum + "]";

      if (i == currentPotionLine) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(0, y - 8, u8g2.getDisplayWidth(), 9);
        u8g2.setDrawColor(0);
      } else
        u8g2.setDrawColor(1);

      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(0, y, lineNum.c_str());
      u8g2.drawStr(35, y, potionLines[i].c_str());

      y += 8;
    }
  }

  u8g2.sendBuffer();
  u8g2.setDrawColor(1);
}

void loadPotionFile(String path) {
  potionLines.clear();
  pocketmage::setCpuSpeed(240);

  File file = global_fs->open(path);
  if (!file || file.isDirectory()) {
    return;
  }

  String line = "";
  while (file.available()) {
    char c = file.read();

    if (c == '\n') {
      // Strip optional '\r'
      if (line.endsWith("\r")) {
        line.remove(line.length() - 1);
      }
      potionLines.push_back(line);
      line = "";
    } else {
      line += c;
    }
  }

  // Handle last line if file doesn't end with '\n'
  if (line.length() > 0) {
    if (line.endsWith("\r")) {
      line.remove(line.length() - 1);
    }
    potionLines.push_back(line);
  }

  file.close();

  // If document is blank, open a line
  if (potionLines.size() == 0)
    potionLines.push_back("");

  pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void savePotionFile(String path) {
  pocketmage::setCpuSpeed(240);

  File file = global_fs->open(path, FILE_WRITE);
  if (!file) {
    return;
  }

  for (size_t i = 0; i < potionLines.size(); i++) {
    file.print(potionLines[i]);
    if (i < potionLines.size() - 1) {
      file.print('\n');
    }
  }

  file.close();
  pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  OLED().oledWord("FILE SAVED");
  delay(500);
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
  display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

  if (terminalOutputs.size() < 14) {
    int y = 14;
    for (const String& s : terminalOutputs) {
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&FreeMonoBold9pt7b);
      display.setCursor(5, y);
      display.print(s);
      y += 16;
    }
  } else {
    int y = display.height() - 5;
    for (int i = terminalOutputs.size() - 1; i >= 0; i--) {
      if (y < 0)
        break;
      const String& s = terminalOutputs[i];
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&FreeMonoBold9pt7b);
      display.setCursor(5, y);
      display.print(s);
      y -= 16;

      display.setTextColor(GxEPD_BLACK);
    }
  }

  EINK().refresh();
}

void funcSelect(String command) {
  String returnText = "";

  // Add inputted command to terminal outputs
  String totalMsg = currentDir + ">" + command;
  if (totalMsg.length() > 28)
    totalMsg = totalMsg.substring(0, 28);
  terminalOutputs.push_back(totalMsg);

  command.toLowerCase();


  // Clear command window
  if (command == "clear") {
    terminalOutputs.clear();
    newState = true;
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
    terminalOutputs.push_back("mv <src> <dest>  Move/rename");
    terminalOutputs.push_back("touch <file>     Create file");
    terminalOutputs.push_back("clear         Clear terminal");
    terminalOutputs.push_back("txt <file>       Open in TXT");
    terminalOutputs.push_back("potion/pot <file>  Edit prgm");
    terminalOutputs.push_back("brew <file>         Run prgm");

    newState = true;
    return;
  }

  // Enter directory
  else if (command.startsWith("cd")) {
    pocketmage::setCpuSpeed(240);
    // Remove "cd " prefix and trim whitespace
    String arg = command.substring(2);
    arg.trim();
    if (arg.length() == 0) {
      currentDir = "/";  // 'cd' alone returns to root
    } else {
      String newPath = arg;
      // Handle relative paths
      if (!newPath.startsWith("/")) {
        if (!currentDir.endsWith("/"))
          currentDir += "/";
        newPath = currentDir + newPath;
      }
      // Remove trailing '/' unless root
      if (newPath.length() > 1 && newPath.endsWith("/")) {
        newPath.remove(newPath.length() - 1);
      }

      // Check if directory exists
      if (global_fs->exists(newPath)) {
        File f = global_fs->open(newPath);
        if (f && f.isDirectory()) {
          currentDir = newPath;
        } else {
          returnText = "cd: Not a directory";
        }
        f.close();
      } else {
        returnText = "cd: No such directory";
      }
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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
          if (lineOutput.length() > 28)
            lineOutput = lineOutput.substring(0, 28);
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
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
    newState = true;
    return;
  }

  // Make directory
  else if (command.startsWith("mkdir")) {
    pocketmage::setCpuSpeed(240);

    // Format the path
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

    // Create the directory
    if (!global_fs->exists(newDirPath))
      global_fs->mkdir(newDirPath);
    currentDir = newDirPath;

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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
      File root = global_fs->open(dirPath);
      if (!root) {
        returnText = "Failed to open path";
      } else {
        if (!root.isDirectory()) {
          // Simple file delete
          if (!global_fs->remove(dirPath))
            returnText = "Failed to remove file";
        } else {
          // Recursive directory delete
          File entry;
          while (true) {
            entry = root.openNextFile();
            if (!entry)
              break;

            String entryPath = dirPath;
            if (!entryPath.endsWith("/"))
              entryPath += "/";
            entryPath += entry.name();

            if (entry.isDirectory()) {
              // recurse inline by reusing rm -r logic
              String subCmd = "rm -r " + entryPath;
              command = subCmd;
              root.close();
              pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
              newState = true;
              return;
            } else {
              global_fs->remove(entryPath);
            }
            entry.close();
          }
          root.close();

          // directory now empty
          if (!global_fs->rmdir(dirPath))
            returnText = "Failed to remove directory";
        }
      }
    } else if (returnText == "") {
      returnText = "Path not found";
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }

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
        f.close();  // REQUIRED
        if (!global_fs->remove(dirPath))
          returnText = "Delete failed";
      }
      if (f)
        f.close();
    } else if (returnText == "") {
      returnText = "Path not found";
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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

      String srcPath =
          src.startsWith("/") ? src : (currentDir + (currentDir.endsWith("/") ? "" : "/") + src);
      String destPath =
          dest.startsWith("/") ? dest : (currentDir + (currentDir.endsWith("/") ? "" : "/") + dest);

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

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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

      String srcPath =
          src.startsWith("/") ? src : (currentDir + (currentDir.endsWith("/") ? "" : "/") + src);
      String destPath =
          dest.startsWith("/") ? dest : (currentDir + (currentDir.endsWith("/") ? "" : "/") + dest);

      if (!global_fs->exists(srcPath)) {
        returnText = "Source not found";
      } else {
        // Try fast rename first
        if (!global_fs->rename(srcPath, destPath)) {
          // Fallback: copy + delete
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

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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
      String filePath =
          arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

      if (global_fs->exists(filePath)) {
        File f = global_fs->open(filePath);
        if (f && f.isDirectory()) {
          returnText = "Is a directory";
        }
        if (f)
          f.close();
      } else {
        File f = global_fs->open(filePath, FILE_WRITE);
        if (!f) {
          returnText = "Failed to create file";
        } else {
          f.close();
        }
      }
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
    newState = true;
    return;
  }

  // Open in text editor
  else if (command.startsWith("txt ")) {
    pocketmage::setCpuSpeed(240);

    String arg = command.substring(4);  // everything after "txt "
    arg.trim();

    if (arg.length() == 0) {
      returnText = "Usage: txt <filename>";
    } else {
      // Ensure .txt extension or add it
      if (!arg.endsWith(".txt")) {
        int dotIdx = arg.lastIndexOf('.');
        if (dotIdx != -1) {
          returnText = "Only .txt files supported";
        } else {
          arg += ".txt";
        }
      }

      if (returnText == "") {
        // Compute full path
        String filePath =
            arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

        // Verify that file exists
        if (!global_fs->exists(filePath)) {
          returnText = "File not found";
        } else {
          // Open in TXT
          PM_SDAUTO().setEditingFile(filePath);
          OLED().oledWord("Opening: " + PM_SDAUTO().getEditingFile());
          pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
          delay(1000);
          TXT_INIT(filePath);
          return;
        }
      }
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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
      // Ensure .txt extension or add it
      if (!arg.endsWith(".c")) {
        // Check if there's an extension at all
        int dotIdx = arg.lastIndexOf('.');
        if (dotIdx != -1) {
          returnText = "Only .c files supported";
        } else {
          // Append .txt automatically
          arg += ".c";
        }
      }

      if (returnText == "") {
        // Compute full path
        String filePath =
            arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

        // Verify that file exists
        if (!global_fs->exists(filePath)) {
          returnText = "File not found";
        } else {
          // Open in Potion
          editFile = filePath;
          pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
          potionInit();
          return;
        }
      }
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
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
      // Ensure .txt extension or add it
      if (!arg.endsWith(".c")) {
        // Check if there's an extension at all
        int dotIdx = arg.lastIndexOf('.');
        if (dotIdx != -1) {
          returnText = "Only .c files supported";
        } else {
          // Append .txt automatically
          arg += ".c";
        }
      }

      if (returnText == "") {
        // Compute full path
        String filePath =
            arg.startsWith("/") ? arg : (currentDir + (currentDir.endsWith("/") ? "" : "/") + arg);

        // Verify that file exists
        if (!global_fs->exists(filePath)) {
          returnText = "File not found";
        } 
        else {
          // Compile and run with Wrench
          // filePath
          const char* wrenchCode = readCFile(filePath);
          compileWrench(wrenchCode);

          pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
          return;
        }
      }
    }

    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    if (returnText != "") {
      terminalOutputs.push_back(returnText);
      OLED().oledWord(returnText);
      delay(1000);
    }
    newState = true;
    return;
  }

  // Check whether command is a home/settings command
  returnText = commandSelect(command);
  if (returnText != "") {
    terminalOutputs.push_back(returnText);
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
}

void wr_prompt(WRContext* c, const WRValue* argv, int argn, WRValue& ret, void* usr) {
  char inbuf[1024];
  char retbuf[1024];

  const char* promptText = argv[0].asString(inbuf, 1024);

  String entered = textPrompt(promptText);
  if (entered == "_EXIT_") entered = "";
  entered.toCharArray(retbuf, sizeof(retbuf));

  wr_makeString(c, &ret, retbuf);
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
  
  // Black on black or white on white
  if ((fillColor && borderColor) || (!fillColor && !borderColor)) {
    display.fillRect(x_origin, y_origin, width, height, !fillColor);
  }
  // Black border white fill
  else if (borderColor && !fillColor) {
    display.drawRect(x_origin, y_origin, width, height, 1);
  }
  //White border, black fill
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
  
  // Black on black or white on white
  if ((fillColor && borderColor) || (!fillColor && !borderColor)) {
    display.fillCircle(x_origin, y_origin, radius, fillColor);
  }
  // Black border white fill
  else if (borderColor && !fillColor) {
    display.drawCircle(x_origin, y_origin, radius, 1);
  }
  //White border, black fill
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
  
  // Set color
  if (color) display.setTextColor(GxEPD_BLACK);
  else display.setTextColor(GxEPD_WHITE);

  // Set font
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
  display.print(text);
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
  
  // Black on black or white on white
  if ((fillColor && borderColor) || (!fillColor && !borderColor)) {
    u8g2.setDrawColor(!fillColor);
    u8g2.drawBox(x_origin, y_origin, width, height);
  }
  // Black border white fill
  else if (borderColor && !fillColor) {
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x_origin, y_origin, width, height);
  }
  //White border, black fill
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
  
  // Set color
  u8g2.setDrawColor(color);

  // Set font
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
      u8g2.setFont(u8g2_font_lubR18_tf);  // regular
      break;
  }
  
  u8g2.drawStr(x_origin, y_origin, text);

  u8g2.setDrawColor(1);
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
    return nullptr;  // file doesn't exist or is a directory
  }

  size_t len = f.size();
  if (len == 0) {
    f.close();
    return nullptr;  // empty file
  }

  char* buf = (char*)malloc(len + 1);
  if (!buf) {
    f.close();
    return nullptr;  // allocation failed
  }

  size_t readBytes = f.readBytes(buf, len);
  buf[readBytes] = '\0';  // null-terminate
  f.close();

  return buf;  // caller must free()
}

void compileWrench(const char* wrenchCode) {
  // Create a state
  WRState* w = wr_newState();

  // Register functions
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

  // Allocate compiled code
  unsigned char* outBytes;
  int outLen;

  // Compile code
  WRstr errMsg;
  int err = wr_compile(wrenchCode, strlen(wrenchCode), &outBytes, &outLen, &errMsg);

  // Run the code
  if (err == 0) {
    wr_run(w, outBytes, outLen, true);  // load and run the code!
  }

  // Output error message
  if (errMsg.c_str() && errMsg[0] != '\0') {
    const char* p = errMsg;
    const char* lineStart = p;

    while (*p) {
      if (*p == '\n') {
        int len = p - lineStart;
        if (len > 0 && lineStart[len - 1] == '\r') {
          len--; // handle CRLF
        }

        // limit to 29 chars
        int outLen = (len > 29) ? 29 : len;
        terminalOutputs.push_back(String(lineStart).substring(0, outLen));

        lineStart = p + 1;
      }
      p++;
    }

    // last line (if no trailing newline)
    if (lineStart != p) {
      int len = p - lineStart;
      int outLen = (len > 29) ? 29 : len;
      terminalOutputs.push_back(String(lineStart).substring(0, outLen));
    }
  }

  // Close code
  wr_destroyState(w);

  newState = true;
}

#pragma region MAIN
void TERMINAL_INIT() {
  CurrentAppState = TERMINAL;
  CurrentTERMfunc = PROMPT;
  potionLines.push_back("");
  KB().setKeyboardState(NORMAL);
  newState = true;
}

void processKB_TERMINAL() {
  // Prompt
  String outLine = "";
  String command = "";

  // Potion
  static int cursor_pos = 0;

  switch (CurrentTERMfunc) {
    case PROMPT:
      KB().setKeyboardState(NORMAL);
      command = textPrompt("", currentDir + ">");
      if (command != "_EXIT_") {
        funcSelect(command);
      } else
        HOME_INIT();
      break;
    case POTION:
      int currentMillis = millis();
      String left = "";
      String right = "";

      // update scroll
      if (TOUCH().updateScroll(potionLines.size() - 1, currentPotionLine)) {
        // Put cursor at the end
        cursor_pos = potionLines[currentPotionLine].length();
        newState = true;
      }

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        pocketmage::setCpuSpeed(240);
        char inchar = KB().updateKeypress();
        if (inchar != 0)
          lastInput = millis();

        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
        // CR Recieved
        else if (inchar == 13) {
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
            if (cursor_pos == potionLines[currentPotionLine].length()) {
              potionLines[currentPotionLine].remove(potionLines[currentPotionLine].length() - 1, 1);
            } else {
              potionLines[currentPotionLine].remove(cursor_pos - 1, 1);
            }
            cursor_pos--;
          }
          else if (potionLines[currentPotionLine].length() == 0) {
            potionLines.erase(potionLines.begin() + currentPotionLine);
            newState = true;
          }
        }
        // LEFT
        else if (inchar == 19) {
          if (cursor_pos > 0) {
            cursor_pos--;
          }
        }
        // RIGHT
        else if (inchar == 21) {
          if (cursor_pos < potionLines[currentPotionLine].length()) {
            cursor_pos++;
          }
        }
        // CENTER
        else if (inchar == 20) {
          KB().setKeyboardState(FUNC);
          command = textPrompt("GOTO LINE:");
          if (command != "_EXIT_") {
            int line = atoi(command.c_str());
            // Line is in bounds
            if (line >= 0 && line < potionLines.size()) {
              currentPotionLine = line;
            }
            else if (line < 0) currentPotionLine = 0;
            else if (line >= potionLines.size()) currentPotionLine = potionLines.size() - 1;
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

        currentMillis = millis();
        // Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
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
  pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
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
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

        if (potionLines.size() < 24) {
          int y = 10;
          for (size_t i = 0; i < potionLines.size(); i++) {
            const String& s = potionLines[i];

            String lineNum = String(i);
            while (lineNum.length() < 3) {
              lineNum = "0" + lineNum;
            }

            if (i == currentPotionLine) {
              display.fillRect(0, y - 9, display.width(), 11, GxEPD_WHITE);
              display.setTextColor(GxEPD_BLACK);
            } else
              display.setTextColor(GxEPD_WHITE);
            display.setFont(&Font5x7Fixed);
            display.setCursor(5, y);
            display.print("[" + lineNum + "]");
            display.setCursor(35, y);
            display.print(s);
            y += 10;
          }
        } 
        else {
          if (currentPotionLine <= 20) {
            int y = 10;
            for (size_t i = 0; i < potionLines.size(); i++) {
              if (i < 0 || i > potionLines.size() || y < 0 || y > (display.height()+10)) continue;

              const String& s = potionLines[i];

              String lineNum = String(i);
              while (lineNum.length() < 3) {
                lineNum = "0" + lineNum;
              }

              if (i == currentPotionLine) {
                display.fillRect(0, y - 9, display.width(), 11, GxEPD_WHITE);
                display.setTextColor(GxEPD_BLACK);
              } else
                display.setTextColor(GxEPD_WHITE);
              display.setFont(&Font5x7Fixed);
              display.setCursor(5, y);
              display.print("[" + lineNum + "]");
              display.setCursor(35, y);
              display.print(s);
              y += 10;
            }
          }
          else {
            int y = 10;
            for (size_t i = currentPotionLine - 20; i < currentPotionLine + 3; i++) {
              if (i < 0 || i > potionLines.size() || y < 0 || y > (display.height()+10)) continue;

              const String& s = potionLines[i];

              String lineNum = String(i);
              while (lineNum.length() < 3) {
                lineNum = "0" + lineNum;
              }

              if (i == currentPotionLine) {
                display.fillRect(0, y - 9, display.width(), 11, GxEPD_WHITE);
                display.setTextColor(GxEPD_BLACK);
              } else
                display.setTextColor(GxEPD_WHITE);
              display.setFont(&Font5x7Fixed);
              display.setCursor(5, y);
              display.print("[" + lineNum + "]");
              display.setCursor(35, y);
              display.print(s);
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
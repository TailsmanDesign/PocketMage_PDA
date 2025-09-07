//  oooooooooooo ooooo ooooo        oooooooooooo oooooo   oooooo     oooo ooooo  oooooooooooo  //
//  `888'     `8 `888' `888'        `888'     `8  `888.    `888.     .8'  `888' d'""""""d888'  //
//   888          888   888          888           `888.   .8888.   .8'    888        .888P    //
//   888oooo8     888   888          888oooo8       `888  .8'`888. .8'     888       d888'     //
//   888    "     888   888          888    "        `888.8'  `888.8'      888     .888P       //
//   888          888   888       o  888       o      `888'    `888'       888    d888'    .P  //
//  o888o        o888o o888ooooood8 o888ooooood8       `8'      `8'       o888o .8888888888P   //
#include <pocketmage.h>


enum FileWizState { WIZ0_, WIZ1_, WIZ1_YN, WIZ2_R, WIZ2_C, WIZ3_ };
FileWizState CurrentFileWizState = WIZ0_;

String currentWord = "";
static String currentLine = "";

std::vector<String> excludedPaths = {
  "/sys",
  "/System Volume Information"
};

void FILEWIZ_INIT() {
  CurrentAppState = FILEWIZ;
  CurrentKBState  = NORMAL;
  EINK().forceSlowFullUpdate(true);
  newState = true;
}

// OLED file display
struct FileObject {
  String address;    // Full path, e.g. "/files/test.txt"
  String name;       // Base name without extension, e.g. "test"
  String extension;  // Extension including dot, e.g. ".txt"
  char type;         // 'T' = txt, 'F' = folder, 'G' = other, 'A' = app (.tar)

  void init(const String &path, bool isDirectory) {
    address = path;

    if (isDirectory) {
      type = 'F';
      name = path.substring(path.lastIndexOf('/') + 1);
      extension = "";
      return;
    }

    // Extract filename
    String filename = path.substring(path.lastIndexOf('/') + 1);

    // Split into name + extension
    int dot = filename.lastIndexOf('.');
    if (dot > 0) {
      name = filename.substring(0, dot);
      extension = filename.substring(dot); // includes the dot
    } else {
      name = filename;
      extension = "";
    }

    // Determine type
    if (extension.equalsIgnoreCase(".txt")) {
      type = 'T';
    } else if (extension.equalsIgnoreCase(".tar")) {
      type = 'A';
    } else {
      type = 'G';
    }
  }
};

String renderWizMini(String folder, int8_t scrollDelta) {
  static long scroll = 0;
  static String prevFolder = "";
  static std::vector<FileObject> cachedFiles;

  // Reload directory if folder changed
  if (folder != prevFolder) {
    scroll = 0;
    scrollDelta = 0;
    cachedFiles.clear();

    File dir = SD_MMC.open(folder);
    if (dir && dir.isDirectory()) {
      File entry;
      while ((entry = dir.openNextFile())) {
        // Normalize full path
        String fullPath = folder;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += entry.name();

        // Skip folder itself if in excludedPaths
        bool skip = false;
        for (auto &ex : excludedPaths) {
          if (fullPath.equalsIgnoreCase(ex)) {
            skip = true;
            break;
          }
        }
        if (skip) {
          entry.close();
          continue;
        }

        FileObject f;
        f.init(fullPath, entry.isDirectory());
        cachedFiles.push_back(f);
        entry.close();
      }
    }
    dir.close();

    // Sort: folders first (alphabetical), then files (alphabetical)
    std::sort(cachedFiles.begin(), cachedFiles.end(), [](const FileObject &a, const FileObject &b) {
      if (a.type == 'F' && b.type != 'F') return true;
      if (a.type != 'F' && b.type == 'F') return false;
      return a.name.compareTo(b.name) < 0;
    });

    prevFolder = folder;
  }

  // Empty folder
  if (cachedFiles.empty()) {
    String msg = folder + " is empty!";
    OLED().oledWord(msg);
    return "";
  }

  // Clamp scroll
  if ((scroll + scrollDelta) < 0) scroll = 0;
  else if ((scroll + scrollDelta) >= (long)cachedFiles.size()) scroll = cachedFiles.size() - 1;
  else scroll += scrollDelta;

  // Display Icons
  u8g2.clearBuffer();
  const int maxDisplay = 14;
  for (size_t i = scroll; i < cachedFiles.size() && i < scroll + maxDisplay; i++) {
    FileObject &f = cachedFiles[i];

    // Big icon for first visible
    if (i == scroll) {
      switch (f.type) {
        case 'T': u8g2.drawXBMP(1, 1, 30, 30, _LFileIcons[0]); break;
        case 'F': u8g2.drawXBMP(1, 1, 30, 30, _LFileIcons[1]); break;
        case 'A': u8g2.drawXBMP(1, 1, 30, 30, _LFileIcons[2]); break;
        default:  u8g2.drawXBMP(1, 1, 30, 30, _LFileIcons[3]); break;
      }
      String dispName = f.name + f.extension;
      u8g2.setFont(u8g2_font_helvB14_tf);
      u8g2.drawStr(34,30,dispName.c_str());
    }
    else {
      int x = 34 + 18 * (i - scroll - 1);
      switch (f.type) {
        case 'T': u8g2.drawXBMP(x, 1, 15, 15, _SFileIcons[0]); break;
        case 'F': u8g2.drawXBMP(x, 1, 15, 15, _SFileIcons[1]); break;
        case 'A': u8g2.drawXBMP(x, 1, 15, 15, _SFileIcons[2]); break;
        default:  u8g2.drawXBMP(x, 1, 15, 15, _SFileIcons[3]); break;
      }
    }
  }

  // Display KB state
  u8g2.setFont(u8g2_font_5x7_tf);
  switch (CurrentKBState) {
    case 1:
      u8g2.setDrawColor(0);
      u8g2.drawBox(u8g2.getDisplayWidth() - u8g2.getStrWidth("SHIFT"), u8g2.getDisplayHeight(), u8g2.getStrWidth("SHIFT"), -8);
      u8g2.setDrawColor(1);
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("SHIFT")), u8g2.getDisplayHeight(), "SHIFT");
      break;
    case 2:
      u8g2.setDrawColor(0);
      u8g2.drawBox(u8g2.getDisplayWidth() - u8g2.getStrWidth("FN"), u8g2.getDisplayHeight(), u8g2.getStrWidth("FN"), -8);
      u8g2.setDrawColor(1);
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("FN")), u8g2.getDisplayHeight(), "FN");
      break;
  }

  u8g2.sendBuffer();

  return cachedFiles[scroll].address;
}

String fileWizardMini(bool allowRecentSelect) {
  setCpuFrequencyMhz(240);

  int8_t scrollDelta = 0;
  static String selectedPath = "";
  // start at root once
  static String selectedDirectory = "/";

  // Handle Inputs
  int currentMillis = millis();
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
    char inchar = KB().updateKeypress();

    // HANDLE INPUTS
    if (inchar == 0);
    // SHIFT Recieved
    else if (inchar == 17) {
      if (CurrentKBState == SHIFT)
        CurrentKBState = NORMAL;
      else
        CurrentKBState = SHIFT;
    }
    // FN Recieved
    else if (inchar == 18) {
      if (CurrentKBState == FUNC)
        CurrentKBState = NORMAL;
      else
        CurrentKBState = FUNC;
    }
    // Left received
    else if (inchar == 19) {
      scrollDelta = -1;
    }  
    // Right receive
    else if (inchar == 21) {
      scrollDelta = 1;
    }  
    // Exit received
    else if (inchar == 12) {
      return "_EXIT_";
    }
    // Back received
    else if (inchar == 8) {
      // If not at root, go up one directory
      if (selectedDirectory != "/") {
        int lastSlash = selectedDirectory.lastIndexOf('/');
        if (lastSlash > 0) {
          selectedDirectory = selectedDirectory.substring(0, lastSlash);
        } else {
          selectedDirectory = "/";
        }
      }
    }
    // Select received
    else if (inchar == 20 || inchar == 29 || inchar == 7 || inchar == 13) {
      if (selectedPath != "") {
        File entry = SD_MMC.open(selectedPath);
        // If selectedPath is a folder, open it and change the selectedDirectory
        if (entry && entry.isDirectory()) selectedDirectory = selectedPath;
        // If selectedPath is a file, return the selectedPath as a String 
        else return selectedPath;
      }
    }
    else if (allowRecentSelect && (inchar >= '0' && inchar <= '9')) {
      int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
      // SET WORKING FILE
      String selectedFile = filesList[fileIndex - 1];
      if (selectedFile != "-" && selectedFile != "") {
        workingFile = selectedFile;
        // GO TO WIZ1_
        CurrentFileWizState = WIZ1_;
        newState = true;
      }
    }

    KBBounceMillis = currentMillis;  // reset debounce timer

    // Make sure OLED only updates at OLED_MAX_FPS
    if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
      OLEDFPSMillis = currentMillis;
      // Display OLED file list
      String temp_selectedPath = renderWizMini(selectedDirectory, scrollDelta);
      if (temp_selectedPath != "") selectedPath = temp_selectedPath;
    }
  }

  if (SAVE_POWER) setCpuFrequencyMhz(40);
  return "";
}

void processKB_FILEWIZ() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }
  int currentMillis = millis();
  String outPath = "";

  switch (CurrentFileWizState) {
    case WIZ0_:
      disableTimeout = false;

      outPath = fileWizardMini(true);
      if (outPath == "_EXIT_") {
        HOME_INIT();
        break;
      }
      else if (outPath != "") {
        // Open file
        if (outPath != "-" && outPath != "") {
          workingFile = outPath;
          // GO TO WIZ1_
          CurrentFileWizState = WIZ1_;
          newState = true;
        }
      }

      break;

    case WIZ1_:
      disableTimeout = false;

      CurrentKBState = FUNC;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentFileWizState = WIZ0_;
          newState = true;
          break;
        }
        else if (inchar >= '1' && inchar <= '4') {
          int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
          // SELECT OPTION
          switch (fileIndex) {
            case 1: // RENAME
              CurrentFileWizState = WIZ2_R;
              newState = true;
              break;
            case 2: //DELETE
              CurrentFileWizState = WIZ1_YN;
              newState = true;
              break;
            case 3: // COPY
              CurrentFileWizState = WIZ2_C;
              newState = true;
              break;
            case 4: // ELABORATE
              break;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentWord, false);
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case WIZ1_YN:
      disableTimeout = false;

      CurrentKBState = NORMAL;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentFileWizState = WIZ1_;
          newState = true;
          break;
        }
        // Y RECIEVED
        else if (inchar == 'y' || inchar == 'Y') {
          // DELETE FILE
          pocketmage::file::delFile(workingFile);
          
          // RETURN TO FILE WIZ HOME
          CurrentFileWizState = WIZ0_;
          newState = true;
          break;
        }
        // N RECIEVED
        else if (inchar == 'n' || inchar == 'N') {
          // GO BACK
          CurrentFileWizState = WIZ1_;
          newState = true;
          break;
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentWord, false);
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case WIZ2_R:
      disableTimeout = false;

      //CurrentKBState = NORMAL;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        else if (inchar == 12) {
          CurrentFileWizState = WIZ1_;
          CurrentKBState = NORMAL;
          currentWord = "";
          currentLine = "";
          newState = true;
          break;
        }
        //ENTER Recieved
        else if (inchar == 13) {      
          // RENAME FILE                    
          String newName = "/" + currentWord + ".txt";
          pocketmage::file::renFile(workingFile, newName);

          // RETURN TO WIZ0
          CurrentFileWizState = WIZ0_;
          CurrentKBState = NORMAL;
          newState = true;
          currentWord = "";
          currentLine = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentWord, false);
        }
      }
      break;
    case WIZ2_C:
      disableTimeout = false;

      //CurrentKBState = NORMAL;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        //No char recieved
        if (inchar == 0);                                         
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {}
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentWord = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
          }
        }
        else if (inchar == 12) {
          CurrentFileWizState = WIZ1_;
          CurrentKBState = NORMAL;
          currentWord = "";
          currentLine = "";
          newState = true;
          break;
        }
        //ENTER Recieved
        else if (inchar == 13) {      
          // RENAME FILE                    
          String newName = "/" + currentWord + ".txt";
          pocketmage::file::copyFile(workingFile, newName);

          // RETURN TO WIZ0
          CurrentFileWizState = WIZ0_;
          CurrentKBState = NORMAL;
          newState = true;
          currentWord = "";
          currentLine = "";
        }
        //All other chars
        else {
          //Only allow char to be added if it's an allowed char
          if (isalnum(inchar) || inchar == '_' || inchar == '-' || inchar == '.') currentWord += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL){
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentWord, false);
        }
      }
      break;
  
  }
}

void einkHandler_FILEWIZ() {
  switch (CurrentFileWizState) {
    case WIZ0_:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        EINK().drawStatusBar("Select a File (0-9)");
        display.drawBitmap(0, 0, fileWizardallArray[0], 320, 218, GxEPD_BLACK);

        // DRAW FILE LIST
        keypad.disableInterrupts();
        SD().listDir(SD_MMC, "/");
        keypad.enableInterrupts();

        for (int i = 0; i < MAX_FILES; i++) {
          display.setCursor(30, 54+(17*i));
          display.print(filesList[i]);
        }

        EINK().refresh();
      }
      break;
    case WIZ1_:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        EINK().drawStatusBar("- " + workingFile);
        display.drawBitmap(0, 0, fileWizardallArray[1], 320, 218, GxEPD_BLACK);

        EINK().refresh();
      }
      break;
    case WIZ1_YN:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        EINK().drawStatusBar("DEL:" + workingFile + "?(Y/N)");
        display.drawBitmap(0, 0, fileWizardallArray[1], 320, 218, GxEPD_BLACK);

        EINK().refresh();
      }
      break;
    case WIZ2_R:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        EINK().drawStatusBar("Enter New Filename:");
        display.drawBitmap(0, 0, fileWizardallArray[2], 320, 218, GxEPD_BLACK);

        EINK().refresh();
      }
      break;
    case WIZ2_C:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        EINK().drawStatusBar("Enter Name For Copy:");
        display.drawBitmap(0, 0, fileWizardallArray[2], 320, 218, GxEPD_BLACK);

        EINK().refresh();
      }
      break;
  }
}
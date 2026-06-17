//  oooooooooooo ooooo ooooo        oooooooooooo oooooo   oooooo     oooo ooooo  oooooooooooo  //
//  `888'     `8 `888' `888'        `888'     `8  `888.    `888.     .8'  `888' d'""""""d888'  //
//   888          888   888          888           `888.   .8888.   .8'    888        .888P    //
//   888oooo8     888   888          888oooo8       `888  .8'`888. .8'     888       d888'     //
//   888    "     888   888          888    "        `888.8'  `888.8'      888     .888P       //
//   888          888   888       o  888       o      `888'    `888'       888    d888'    .P  //
//  o888o        o888o o888ooooood8 o888ooooood8       `8'      `8'       o888o .8888888888P   //
// AUDIT 1

#include <globals.h>
#if !OTA_APP // POCKETMAGE_OS

enum FileWizState { WIZ0_, WIZ1_ };
FileWizState CurrentFileWizState = WIZ0_;

String currentWord = "";
static String currentLine = "";
bool refreshFiles = false;

#define MAX_CACHED_FILES 200 

std::vector<String> excludedPaths = {
  "/sys",
  "/System Volume Information",
  "/apps/temp",
  "/temp"
};

void FILEWIZ_INIT() {
  CurrentAppState = FILEWIZ;
  CurrentFileWizState = WIZ0_;
  KB().setKeyboardState(NORMAL);
  #if POCKETMAGE_HW_VERSION != 2
    EINK().forceSlowFullUpdate(true);
  #endif
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
    if (extension.equalsIgnoreCase(".txt") || extension.equalsIgnoreCase(".md") || extension.equalsIgnoreCase(".c")) {
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

  // Force reload directory if file changed/deleted
  if (refreshFiles) {
    prevFolder = ""; // Force the if-statement below to trigger
    refreshFiles = false;
  }

  // Reload directory if folder changed
  if (folder != prevFolder) {
    SDActive = true;
    pocketmage::setCpuSpeed(240);

    scroll = 0;
    scrollDelta = 0;
    cachedFiles.clear();

    File dir = global_fs->open(folder);
    if (dir && dir.isDirectory()) {
      File entry;
      while ((entry = dir.openNextFile()) && cachedFiles.size() < MAX_CACHED_FILES) {
        
        String fullPath = folder;
        if (!fullPath.endsWith("/")) fullPath += "/";
        
        String entryName = entry.name();
        if (entryName.startsWith("/")) entryName = entryName.substring(1);
        fullPath += entryName;

        // Skip folder itself if in excludedPaths
        bool skip = false;
        for (auto &ex : excludedPaths) {
          if (fullPath.equalsIgnoreCase(ex)) {
            skip = true;
            break;
          }
        }
        // Skip MacOS metadata files (they are dumb)
        if (entryName.startsWith("._")) {
          skip = true;
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

    if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    SDActive = false;
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
      u8g2.setFont(u8g2_font_7x13B_tf);
      u8g2.drawStr(34,29,dispName.c_str());
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
  switch (KB().getKeyboardState()) {
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
    case 3:
      u8g2.setDrawColor(0);
      u8g2.drawBox(u8g2.getDisplayWidth() - u8g2.getStrWidth("FN+SHIFT"), u8g2.getDisplayHeight(), u8g2.getStrWidth("FN+SHIFT"), -8);
      u8g2.setDrawColor(1);
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("FN+SHIFT")), u8g2.getDisplayHeight(), "FN+SHIFT");
      break;
  }

  u8g2.sendBuffer();

  return cachedFiles[scroll].address;
}

String fileWizardMini(bool allowRecentSelect, String rootDir, char inchar_) {
  pocketmage::setCpuSpeed(240);

  int8_t scrollDelta = 0;
  static String selectedPath = "";
  static String selectedDirectory = "";

  // Initialize or clamp selectedDirectory to rootDir
  if (selectedDirectory == "" || selectedDirectory.length() < rootDir.length() || 
      !selectedDirectory.startsWith(rootDir)) {
    selectedDirectory = rootDir;
  }

  // 1. Always drain the buffer continuously
  int currentMillis = millis();
  char inchar = 0;
  if (inchar_ == 0) {
    inchar = KB().updateKeypress();
  } else {
    inchar = inchar_;
  }

  // 2. Process input only if cooldown has expired
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    if (inchar != 0) {
      KBBounceMillis = currentMillis;  // reset debounce timer

      // HANDLE INPUTS
      if (inchar == 17) {
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
      // Left received
      else if (inchar == 19) {
        scrollDelta = -1;
      }  
      // Right received
      else if (inchar == 21) {
        scrollDelta = 1;
      } 
      // Exit received
      else if (inchar == 12) {
        return "_EXIT_";
      }
      // Back received
      else if (inchar == 8) {
        // If not at rootDir, go up one directory
        if (selectedDirectory != rootDir) {
          int lastSlash = selectedDirectory.lastIndexOf('/');
          if (lastSlash > 0) {
            selectedDirectory = selectedDirectory.substring(0, lastSlash);
          } else {
            selectedDirectory = rootDir;
          }
        }
      }
      // Select received
      else if (inchar == 20 || inchar == 29 || inchar == 7 || inchar == 13) {
        if (selectedPath != "") {
          File entry = global_fs->open(selectedPath);
          if (entry) {
            bool isDirectory = entry.isDirectory();
            entry.close();

            if (isDirectory) {
              selectedDirectory = selectedPath;
              // Clamp to rootDir if needed
              if (selectedDirectory.length() < rootDir.length() || 
                  !selectedDirectory.startsWith(rootDir)) {
                selectedDirectory = rootDir;
              }
            }
            else {
              return selectedPath;
            }
          }
        }
      }
      else if (allowRecentSelect && (inchar >= '0' && inchar <= '9')) {
        int fileIndex = (inchar == '0') ? 10 : (inchar - '0');
        // SET WORKING FILE
        String selectedFile = PM_SDAUTO().getFilesListIndex(fileIndex - 1);
        if (selectedFile != "-" && selectedFile != "") {
          PM_SDAUTO().setWorkingFile(selectedFile);
          // GO TO WIZ1_
          CurrentFileWizState = WIZ1_;
          newState = true;
        }
      }
    }
  }

  // 3. Make sure OLED only updates at OLED_MAX_FPS (Independent of KB_COOLDOWN)
  currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;
    // Display OLED file list
    String temp_selectedPath = renderWizMini(selectedDirectory, scrollDelta);
    if (temp_selectedPath != "") selectedPath = temp_selectedPath;
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  return "";
}


// EInk file display
struct MetaEntry {
  String path;
  String timestampStr;
};

void updateRecentFilesList() {
  std::vector<MetaEntry> recentFiles;
  
  keypad.disableInterrupts();
  File metaFile = global_fs->open("/sys/SDMMC_META.txt", FILE_READ);
  
  if (metaFile) {
    while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      line.trim();
      
      // Skip empty lines or malformed lines
      if (line.length() == 0 || line.indexOf('|') == -1) continue;
      
      int firstPipe = line.indexOf('|');
      int secondPipe = line.indexOf('|', firstPipe + 1);
      
      if (firstPipe > 0 && secondPipe > firstPipe) {
        String path = line.substring(0, firstPipe);
        String timeStr = line.substring(firstPipe + 1, secondPipe);
        
        // Remove the hyphen for clean string comparison sorting
        timeStr.replace("-", ""); 
        
        // Check against the global excludedPaths vector
        bool skip = false;
        for (const String& ex : excludedPaths) {
          // Check if it's the exact path OR inside the excluded directory
          if (path.equalsIgnoreCase(ex) || path.startsWith(ex + "/")) {
            skip = true;
            break;
          }
        }
        
        if (!skip) {
           recentFiles.push_back({path, timeStr});
        }
      }
    }
    metaFile.close();
  }
  keypad.enableInterrupts();

  // Sort descending (newest first) using String comparison
  std::sort(recentFiles.begin(), recentFiles.end(), [](const MetaEntry& a, const MetaEntry& b) {
    return a.timestampStr.compareTo(b.timestampStr) > 0;
  });

  // Update the global file list array
  int displayCount = min((int)recentFiles.size(), 10);
  for (int i = 0; i < displayCount; i++) {
    PM_SDAUTO().setFilesListIndex(i, recentFiles[i].path);
  }
  
  // Clear any unused slots
  for (int i = displayCount; i < 10; i++) {
    PM_SDAUTO().setFilesListIndex(i, "-");
  }
}


// Main loops
void processKB_FILEWIZ() {
  OLED().setPowerSave(false);
  int currentMillis = millis();
  String outPath = "";
  char inchar = 0;

  switch (CurrentFileWizState) {
    case WIZ0_:
      disableTimeout = false;

      outPath = fileWizardMini(true, "/");
      if (outPath == "_EXIT_") {
        HOME_INIT();
        break;
      }
      else if (outPath != "") {
        // Open file
        if (outPath != "-" && outPath != "") {
          PM_SDAUTO().setWorkingFile(outPath);
          CurrentFileWizState = WIZ1_;
          newState = true;
        }
      }
      break;

    case WIZ1_:
      disableTimeout = false;
      KB().setKeyboardState(FUNC);
      
      // 1. Drain the hardware buffer continuously at loop speed
      inchar = KB().updateKeypress();
      
      // 2. Only process the actual input if the cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          // BKSP Recieved (Go back to WIZ0)
          if (inchar == 127 || inchar == 8 || inchar == 12) {
            FILEWIZ_INIT();
            break;
          }
          else if (inchar >= '1' && inchar <= '4') {
            
            // Get the current file path and isolate its directory & extension
            String oldPath = PM_SDAUTO().getWorkingFile();
            int lastSlash = oldPath.lastIndexOf('/');
            int lastDot = oldPath.lastIndexOf('.');
            
            String dirPath = (lastSlash != -1) ? oldPath.substring(0, lastSlash + 1) : "/";
            String extension = (lastDot != -1 && lastDot > lastSlash) ? oldPath.substring(lastDot) : "";

            // --- 1. RENAME FILE ---
            if (inchar == '1') { 
              KB().setKeyboardState(NORMAL);
              String input = textPrompt("Enter new filename:");
              if (input == "_RETURN_") return;
              else if (input != "_EXIT_" && input != "") {
                OLED().oledWord("Renaming...");
                
                String newName = dirPath + input + extension; 
                PM_SDAUTO().renFile(oldPath, newName);
                
                refreshFiles = true;
                CurrentFileWizState = WIZ0_;
                newState = true;
              }
            }
            // --- 2. DELETE FILE ---
            else if (inchar == '2') { 
              int response = boolPrompt("Delete File?");
              if (response == 1) {
                OLED().oledWord("Deleting...");
                PM_SDAUTO().delFile(oldPath);
                
                refreshFiles = true;
                CurrentFileWizState = WIZ0_;
                newState = true;
              }
            }
            // --- 3. COPY FILE ---
            else if (inchar == '3') { 
              KB().setKeyboardState(NORMAL);
              String input = textPrompt("Enter copy name:");
              if (input == "_RETURN_") return;
              else if (input != "_EXIT_" && input != "") {
                OLED().oledWord("Copying...");
                
                String newName = dirPath + input + extension;
                PM_SDAUTO().copyFile(oldPath, newName);
                
                refreshFiles = true;
                CurrentFileWizState = WIZ0_;
                newState = true;
              }
            }
            // --- 4. ELABORATE (INFO) ---
            else if (inchar == '4') { 
              File f = global_fs->open(oldPath, FILE_READ);
              if (f) {
                size_t fSize = f.size();
                f.close(); // Safety close
                
                String info = "Size: " + String(fSize) + " Bytes.";
                waitForKeypress(info);
              } else {
                OLED().sysMessage("Error reading file",1500);
              }
              KB().setKeyboardState(FUNC); 
            }
          }
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        if (CurrentFileWizState == WIZ1_) { // Make sure we didn't just jump back to WIZ0_
          OLED().oledWord("Select an operation");
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
        EINK().resetDisplay();

        // DRAW APP
        EINK().drawStatusBar("Select a File (0-9)");
        display.drawBitmap(0, 0, fileWizardallArray[0], 320, 218, GxEPD_BLACK);

        // Update the backend array with the newest files
        updateRecentFilesList();

        // Draw the file list
        u8g2f.setFont(u8g2_font_5x7_tf);
        u8g2f.setFontMode(1);
        for (int i = 0; i < 10; i++) {
          String dispPath = PM_SDAUTO().getFilesListIndex(i);
          
          if (dispPath != "-") {
            // Truncate long paths for the UI
            if (dispPath.length() > 30) {
               dispPath = "..." + dispPath.substring(dispPath.length() - 27);
            }
            
            u8g2f.setCursor(30, 54 + (17 * i));
            u8g2f.print(dispPath);
          }
        }

        EINK().multiPassRefresh(2);
      }
      break;
    case WIZ1_:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        // DRAW APP
        EINK().drawStatusBar("- " + PM_SDAUTO().getWorkingFile());
        display.drawBitmap(0, 0, fileWizardallArray[1], 320, 218, GxEPD_BLACK);

        EINK().multiPassRefresh(2);
      }
      break;
  }
}

#endif
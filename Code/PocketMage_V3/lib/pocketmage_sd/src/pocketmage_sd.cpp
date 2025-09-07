// .d88888b  888888ba  //
// 88.    "' 88    `8b //
// `Y88888b. 88     88 //
//       `8b 88     88 //
// d8'   .8P 88    .8P //
//  Y88888P  8888888P  //

#include <pocketmage_sd.h>
#include <pocketmage_oled.h> 
#include <pocketmage_eink.h> 
#include <config.h> // for FULL_REFRESH_AFTER

extern bool SAVE_POWER;

// ===================== main functions =====================
// Low-Level SDMMC Operations switch to using internal fs::FS*
void PocketmageSD::listDir(fs::FS &fs, const char *dirname) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Listing directory %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
      if (noTimeout_) *noTimeout_ = false;
      ESP_LOGE(tag, "Failed to open directory: %s", root.path());
      return;
    }
    if (!root.isDirectory()) {
      if (noTimeout_) *noTimeout_ = false;
      ESP_LOGE(tag, "Not a directory: %s", root.path());
      
      return;
    }

    // Reset fileIndex and initialize filesList with "-"
    fileIndex_ = 0; // Reset fileIndex
    for (int i = 0; i < MAX_FILES; i++) {
      filesList_[i] = "-";
    }

    File file = root.openNextFile();
    while (file && fileIndex_ < MAX_FILES) {
      if (!file.isDirectory()) {
        String fileName = String(file.name());
        
        // Check if file is in the exclusion list
        bool excluded = false;
        for (const String &excludedFile : excludedFiles_) {
          if (fileName.equals(excludedFile) || ("/"+fileName).equals(excludedFile)) {
            excluded = true;
            break;
          }
        }

        if (!excluded) {
          filesList_[fileIndex_++] = fileName; // Store file name if not excluded
        }
      }
      file = root.openNextFile();
    }

    // for (int i = 0; i < fileIndex_; i++) { // Only print valid entries
    //   Serial.println(filesList_[i]);       // NOTE: This prints out valid files
    // }

    if (noTimeout_) *noTimeout_ = false;
    if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}
void PocketmageSD::readFile(fs::FS &fs, const char *path) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Reading file %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      if (noTimeout_) *noTimeout_ = false;
      ESP_LOGE(tag, "Failed to open file for reading: %s", file.path());
      return;
    }

    file.close();
    if (noTimeout_) *noTimeout_ = false;
    if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}
String PocketmageSD::readFileToString(fs::FS &fs, const char *path) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return "";
  }
  else { 
    setCpuFrequencyMhz(240);
    delay(50);

    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      if (noTimeout_) *noTimeout_ = false;
      ESP_LOGE(tag, "Failed to open file for reading: %s", path);
      if (oled_) oled_->oledWord("Load Failed");
      delay(500);
      return "";  // Return an empty string on failure
    }

    // TODO: Can't we use readString()?
    // Serial.println("- reading from file:");
    ESP_LOGI(tag, "Reading from file: %s", file.path());
    String content = file.readString();

    file.close();
    if (eink_) eink_->setFullRefreshAfter(FULL_REFRESH_AFTER); //Force a full refresh
    if (noTimeout_) *noTimeout_ = false;
    return content;  // Return the complete String
  }
}
void PocketmageSD::writeFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Writing file: %s\r\n", path);
    delay(200);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      if (noTimeout_) *noTimeout_ = false;
      ESP_LOGE(tag, "Failed to open %s for writing", path);
      return;
    }
    if (file.print(message)) {
      ESP_LOGV(tag, "File written %s", path);
    } 
    else {
      ESP_LOGE(tag, "Write failed for %s", path);
    }
    file.close();
    if (noTimeout_) *noTimeout_ = false;
    if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}
void PocketmageSD::appendFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      if (noTimeout_) *noTimeout_ = false;
      ESP_LOGE(tag, "Failed to open for appending: %s", path);
      return;
    }
    if (file.println(message)) {
      ESP_LOGV(tag, "Message appended to %s", path);
    } 
    else {
      ESP_LOGE(tag, "Append failed: %s", path);
    }
    file.close();
    if (noTimeout_) *noTimeout_ = false;
    if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}
void PocketmageSD::renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Renaming file %s to %s\r\n", path1, path2);

    if (fs.rename(path1, path2)) {
      ESP_LOGV(tag, "Renamed %s to %s\r\n", path1, path2);
    } 
    else {
      ESP_LOGE(tag, "Rename failed: %s to %s", path1, path2);
    }
    if (noTimeout_) *noTimeout_ = false;
    if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}
void PocketmageSD::deleteFile(fs::FS &fs, const char *path) {
  if (noSD_ && *noSD_) {
    if (oled_) oled_->oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    setCpuFrequencyMhz(240);
    delay(50);
    if (noTimeout_) *noTimeout_ = true;
    ESP_LOGI(tag, "Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
      ESP_LOGV(tag, "File deleted: %s", path);
    } 
    else {
      ESP_LOGE(tag, "Delete failed for %s", path);
    }
    if (noTimeout_) *noTimeout_ = false;
    if (SAVE_POWER) setCpuFrequencyMhz(40);
  }
}

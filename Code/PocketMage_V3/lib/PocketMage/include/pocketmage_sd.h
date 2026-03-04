//  .d88888b  888888ba   //
//  88.    "' 88    `8b  //
//  `Y88888b. 88     88  //
//        `8b 88     88  //
//  d8'   .8P 88    .8P  //
//   Y88888P  8888888P   //


#pragma once
#include <Arduino.h>
#include <FS.h>

// forward-declaration to avoid including U8g2lib.h, GxEPD2_BW.h, pocketmage_oled.h, and pocketmage_eink.h
class PocketmageOled;
class PocketmageEink;

static bool noSD = false;
static String editingFile = "";
static String workingFile = "";
static String filesList[10];

// ===================== SD CLASS =====================
class PocketmageSDAUTO {
public:
  explicit PocketmageSDAUTO() {}

  void saveFile();
  void writeMetadata(const String& path);
  void loadFile(bool showOLED = true);
  void delFile(String fileName);
  void deleteMetadata(String path);
  void renFile(String oldFile, String newFile);
  void renMetadata(String oldPath, String newPath);
  void copyFile(String oldFile, String newFile);
  void appendToFile(String path, String inText);

  // Getters / Setters
  bool getNoSD()  {return noSD;}
  void setNoSD(bool in) {noSD = in;}

  String getWorkingFile()  {return workingFile;}
  void setWorkingFile(String in) {workingFile = in;}

  String getEditingFile()  {return editingFile;}
  void setEditingFile(String in) {editingFile = in;}

  String getFilesListIndex(int index) {return filesList[index];}
  void setFilesListIndex(int index, String content) {filesList[index] = content;}

  // low level methods  To Do: remove arguments for fs::FS &fs and reference internal fs::FS* instead
  void listDir(fs::FS &fs, const char *dirname);
  void readFile(fs::FS &fs, const char *path);
  String readFileToString(fs::FS &fs, const char *path);
  void writeFile(fs::FS &fs, const char *path, const char *message);
  void appendFile(fs::FS &fs, const char *path, const char *message);
  void renameFile(fs::FS &fs, const char *path1, const char *path2);
  void deleteFile(fs::FS &fs, const char *path);
  // Read a binary file fully into a buffer
  bool readBinaryFile(const char* path, uint8_t* buf, size_t len);
  // Convenience: read file size
  size_t getFileSize(const char* path);

private:

  static constexpr const char*  tag               = "MAGE_SD";

  String editingFile_ = "";
  String filesList_[MAX_FILES];
  String workingFile_ = "";

  uint8_t                       fileIndex_        = 0;
  String                        excludedFiles_[3] = { "/temp.txt", "/settings.txt", "/tasks.txt" };

  // Flags / counters
  bool                          noSD_              = false;
};

class PocketmageSDMMC {
public:
  explicit PocketmageSDMMC() {}

  void saveFile();
  void writeMetadata(const String& path);
  void loadFile(bool showOLED = true);
  void delFile(String fileName);
  void deleteMetadata(String path);
  void renFile(String oldFile, String newFile);
  void renMetadata(String oldPath, String newPath);
  void copyFile(String oldFile, String newFile);
  void appendToFile(String path, String inText);

  // Getters / Setters
  bool getNoSD()  {return noSD;}
  void setNoSD(bool in) {noSD = in;}

  String getWorkingFile()  {return workingFile;}
  void setWorkingFile(String in) {workingFile = in;}

  String getEditingFile()  {return editingFile;}
  void setEditingFile(String in) {editingFile = in;}

  String getFilesListIndex(int index) {return filesList[index];}
  void setFilesListIndex(int index, String content) {filesList[index] = content;}

  // low level methods  To Do: remove arguments for fs::FS &fs and reference internal fs::FS* instead
  void listDir(fs::FS &fs, const char *dirname);
  void readFile(fs::FS &fs, const char *path);
  String readFileToString(fs::FS &fs, const char *path);
  void writeFile(fs::FS &fs, const char *path, const char *message);
  void appendFile(fs::FS &fs, const char *path, const char *message);
  void renameFile(fs::FS &fs, const char *path1, const char *path2);
  void deleteFile(fs::FS &fs, const char *path);
  // Read a binary file fully into a buffer
  bool readBinaryFile(const char* path, uint8_t* buf, size_t len);
  // Convenience: read file size
  size_t getFileSize(const char* path);

private:

  static constexpr const char*  tag               = "MAGE_SD";

  String editingFile_ = "";
  String filesList_[MAX_FILES];
  String workingFile_ = "";

  uint8_t                       fileIndex_        = 0;
  String                        excludedFiles_[3] = { "/temp.txt", "/settings.txt", "/tasks.txt" };

  // Flags / counters
  bool                          noSD_              = false;
};

class PocketmageSDSPI {
public:
  explicit PocketmageSDSPI() {}

  void saveFile();
  void writeMetadata(const String& path);
  void loadFile(bool showOLED = true);
  void delFile(String fileName);
  void deleteMetadata(String path);
  void renFile(String oldFile, String newFile);
  void renMetadata(String oldPath, String newPath);
  void copyFile(String oldFile, String newFile);
  void appendToFile(String path, String inText);

  // Getters / Setters
  bool getNoSD()  {return noSD;}
  void setNoSD(bool in) {noSD = in;}

  String getWorkingFile()  {return workingFile;}
  void setWorkingFile(String in) {workingFile = in;}

  String getEditingFile()  {return editingFile;}
  void setEditingFile(String in) {editingFile = in;}

  String getFilesListIndex(int index) {return filesList[index];}
  void setFilesListIndex(int index, String content) {filesList[index] = content;}

  // low level methods  To Do: remove arguments for fs::FS &fs and reference internal fs::FS* instead
  void listDir(fs::FS &fs, const char *dirname);
  void readFile(fs::FS &fs, const char *path);
  String readFileToString(fs::FS &fs, const char *path);
  void writeFile(fs::FS &fs, const char *path, const char *message);
  void appendFile(fs::FS &fs, const char *path, const char *message);
  void renameFile(fs::FS &fs, const char *path1, const char *path2);
  void deleteFile(fs::FS &fs, const char *path);
  // Read a binary file fully into a buffer
  bool readBinaryFile(const char* path, uint8_t* buf, size_t len);
  // Convenience: read file size
  size_t getFileSize(const char* path);

private:

  static constexpr const char*  tag               = "MAGE_SD";

  String editingFile_ = "";
  String filesList_[MAX_FILES];
  String workingFile_ = "";

  uint8_t                       fileIndex_        = 0;
  String                        excludedFiles_[3] = { "/temp.txt", "/settings.txt", "/tasks.txt" };

  // Flags / counters
  bool                          noSD_              = false;
};

void setupSD();
PocketmageSDAUTO& PM_SDAUTO();
PocketmageSDMMC& PM_SDMMC();
PocketmageSDSPI& PM_SDSPI();

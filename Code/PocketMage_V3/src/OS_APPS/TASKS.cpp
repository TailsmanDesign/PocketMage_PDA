// ooooooooooooo       .o.        .oooooo..o oooo    oooo  .oooooo..o //
// 8'   888   `8      .888.      d8P'    `Y8 `888   .8P'  d8P'    `Y8 //
//      888          .8"888.     Y88bo.       888  d8'    Y88bo.      //
//      888         .8' `888.     `"Y8888o.   88888[       `"Y8888o.  //
//      888        .88ooo8888.        `"Y88b  888`88b.         `"Y88b //
//      888       .8'     `888.  oo     .d8P  888  `88b.  oo     .d8P //
//     o888o     o88o     o8888o 8""88888P'  o888o  o888o 8""88888P'  //  

#include <globals.h>
#include "esp32-hal-log.h"
#include "esp_log.h"
#if !OTA_APP // POCKETMAGE_OS
enum TasksState { TASKS0, TASKS0_NEWTASK, TASKS1, TASKS1_EDITTASK };
TasksState CurrentTasksState = TASKS0;

static constexpr const char* TAG = "TASKS";

static String currentWord = "";
static String currentLine = "";
uint8_t newTaskState = 0;
uint8_t editTaskState = 0;
String newTaskName = "";
String newTaskDueDate = "";
uint8_t selectedTask = 0;

void TASKS_INIT() {
  CurrentAppState = TASKS;
  CurrentTasksState = TASKS0;
  EINK().forceSlowFullUpdate(true);
  newState = true;
}

void sortTasksByDueDate(std::vector<std::vector<String>> &tasks) {
  std::sort(tasks.begin(), tasks.end(), [](const std::vector<String> &a, const std::vector<String> &b) {
    return a[1] < b[1]; // Compare dueDate strings
  });
}

void updateTasksFile() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);
  // Clear the existing tasks file first
  PM_SDAUTO().delFile("/sys/tasks.txt");

  pocketmage::setCpuSpeed(240);
  if (!global_fs->exists("/sys/tasks.txt")) {
    File f = global_fs->open("/sys/tasks.txt", FILE_WRITE);
    if (f) f.close();
  }
  // Iterate through the tasks vector and append each task to the file
  for (size_t i = 0; i < tasks.size(); i++) {
    // Create a string from the task's attributes with "|" delimiter
    String taskInfo = tasks[i][0] + "|" + tasks[i][1] + "|" + tasks[i][2] + "|" + tasks[i][3];
    
    // Append the task info to the file
    PM_SDAUTO().appendToFile("/sys/tasks.txt", taskInfo);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

void addTask(String taskName, String dueDate, String priority, String completed) {
  String taskInfo = taskName+"|"+dueDate+"|"+priority+"|"+completed;
  updateTaskArray();
  tasks.push_back({taskName, dueDate, priority, completed});
  sortTasksByDueDate(tasks);
  updateTasksFile();
}

void updateTaskArray() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);
  File file = global_fs->open("/sys/tasks.txt", "r"); // Open the text file in read mode
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file to read: %s", file.path());
    return;
  }

  tasks.clear(); // Clear the existing vector before loading the new data

  // Loop through the file, line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');  // Read a line from the file
    line.trim();  // Remove any extra spaces or newlines
    
    // Skip empty lines
    if (line.length() == 0) {
      continue;
    }

    // Split the line into individual parts using the delimiter '|'
    uint8_t delimiterPos1 = line.indexOf('|');
    uint8_t delimiterPos2 = line.indexOf('|', delimiterPos1 + 1);
    uint8_t delimiterPos3 = line.indexOf('|', delimiterPos2 + 1);

    // Extract task name, due date, priority, and completed status
    String taskName  = line.substring(0, delimiterPos1);
    String dueDate   = line.substring(delimiterPos1 + 1, delimiterPos2);
    String priority  = line.substring(delimiterPos2 + 1, delimiterPos3);
    String completed = line.substring(delimiterPos3 + 1);

    // Add the task to the vector
    tasks.push_back({taskName, dueDate, priority, completed});
  }

  file.close();  // Close the file

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}


void deleteTask(int index) {
  if (index >= 0 && index < tasks.size()) {
    tasks.erase(tasks.begin() + index);
  }
}

String convertDateFormat(String yyyymmdd) {
  if (yyyymmdd.length() != 8) {
    ESP_LOGE(TAG, "Invalid Date: %s", yyyymmdd.c_str());
    return "Invalid";
  }

  String year = yyyymmdd.substring(2, 4);  // Get last two digits of the year
  String month = yyyymmdd.substring(4, 6);
  String day = yyyymmdd.substring(6, 8);

  return month + "/" + day + "/" + year;
}

void processKB_TASKS() {
  OLED().setPowerSave(false);
  int currentMillis = millis();
  disableTimeout = false;
  char inchar;

  switch (CurrentTasksState) {
    case TASKS0:
      KB().setKeyboardState(FUNC);
      //Make keyboard only updates after cooldown
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = KB().updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          HOME_INIT();
          break;
        }
        // NEW TASK
        else if (inchar == '/' || inchar == 'n' || inchar == 'N') {
          CurrentTasksState = TASKS0_NEWTASK;
          KB().setKeyboardState(NORMAL);
          newTaskState = 0;
          newState = true;
          break;
        }
        // SELECT A TASK
        else if (inchar >= '0' && inchar <= '9') {
          int taskIndex = (inchar == '0') ? 10 : (inchar - '1');  // Adjust for 1-based input

          // SET SELECTED TASK
          if (taskIndex < tasks.size()) {
            selectedTask = taskIndex;
            // GO TO TASKS1
            CurrentTasksState = TASKS1;
            editTaskState = 0;
            newState = true;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledWord(currentWord);
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case TASKS0_NEWTASK:
      if (newTaskState == 1) KB().setKeyboardState(FUNC);

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = KB().updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);                                        
        //SHIFT Recieved
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
        //Space Recieved
        else if (inchar == 32) {                                  
          currentLine += " ";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8 || inchar == 12) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          // ENTER INFORMATION BASED ON STATE
          switch (newTaskState) {
            case 0: // ENTER TASK NAME
              newTaskName = currentLine;
              currentLine = "";
              newTaskState = 1;
              newState = true;
              break;
            case 1: // ENTER DUE DATE
              String testDate = convertDateFormat(currentLine);
              // DATE IS VALID
              if (testDate != "Invalid") {
                newTaskDueDate = currentLine;

                // ADD NEW TASK
                addTask(newTaskName, newTaskDueDate, "0", "0");
                OLED().oledWord("New Task Added");
                delay(1000);

                // RETURN
                currentLine = "";
                newTaskState = 0;
                CurrentTasksState = TASKS0;
                newState = true;
              }
              // DATE IS INVALID
              else {
                OLED().oledWord("Invalid Date");
                delay(1000);
                currentLine = "";
              }
              break;
          }
        } 

        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (KB().getKeyboardState() != NORMAL) {
            KB().setKeyboardState(NORMAL);
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, currentLine.length(), false);
        }
      }
      break;
    case TASKS1:
      disableTimeout = false;

      KB().setKeyboardState(FUNC);
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentTasksState = TASKS0;
          EINK().forceSlowFullUpdate(true);
          newState = true;
          break;
        }
        // SELECT A TASK
        else if (inchar >= '1' && inchar <= '4') {
          if (inchar == '1') {      // RENAME TASK

          }
          else if (inchar == '2') { // CHANGE DUE DATE

          }
          else if (inchar == '3') { // DELETE TASK
            deleteTask(selectedTask);
            updateTasksFile();
            
            CurrentTasksState = TASKS0;
            EINK().forceSlowFullUpdate(true);
            newState = true;
          }
          else if (inchar == '4') { // COPY TASK

          }
          
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledWord(currentWord);
        }
        KBBounceMillis = currentMillis;
      }
      break;
  
  }
}

void einkHandler_TASKS() {
  switch (CurrentTasksState) {
    case TASKS0:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        // DRAW APP
        display.drawBitmap(0, 0, tasksApp0, 320, 218, GxEPD_BLACK);

        // DRAW FILE LIST
        updateTaskArray();
        sortTasksByDueDate(tasks);

        if (!tasks.empty()) {
          ESP_LOGV(TAG, "Printing Tasks");
          EINK().drawStatusBar("Select (0-9),New Task (N)");

          int loopCount = std::min((int)tasks.size(), MAX_FILES);
          for (int i = 0; i < loopCount; i++) {
            display.setFont(&FreeSerif9pt7b);
            // PRINT TASK NAME
            display.setCursor(29, 54 + (17 * i));
            display.print(tasks[i][0].c_str());
            // PRINT TASK DUE DATE
            display.setCursor(231, 54 + (17 * i));
            display.print(convertDateFormat(tasks[i][1]).c_str());

            // Serial.print(tasks[i][0].c_str()); Serial.println(convertDateFormat(tasks[i][1]).c_str());
            ESP_LOGI("TASKS", "%s, %s", tasks[i][0].c_str(), convertDateFormat(tasks[i][1]).c_str()); // TODO: Come up with some tag
          }
        }
        else EINK().drawStatusBar("No Tasks! Add New Task (N)");

        EINK().refresh();
      }
      break;
      case TASKS0_NEWTASK:
        if (newState) {
          newState = false;
          EINK().resetDisplay();

          // DRAW APP
          display.drawBitmap(0, 0, tasksApp0, 320, 218, GxEPD_BLACK);

          // DRAW FILE LIST
          updateTaskArray();
          sortTasksByDueDate(tasks);

          if (!tasks.empty()) {
            ESP_LOGV(TAG, "Printing Tasks");

            int loopCount = std::min((int)tasks.size(), MAX_FILES);
            for (int i = 0; i < loopCount; i++) {
              display.setFont(&FreeSerif9pt7b);
              // PRINT TASK NAME
              display.setCursor(29, 54 + (17 * i));
              display.print(tasks[i][0].c_str());
              // PRINT TASK DUE DATE
              display.setCursor(231, 54 + (17 * i));
              display.print(convertDateFormat(tasks[i][1]).c_str());

              // Serial.print(tasks[i][0].c_str()); Serial.println(convertDateFormat(tasks[i][1]).c_str());
              ESP_LOGI("TASKS", "%s, %s", tasks[i][0].c_str(), convertDateFormat(tasks[i][1]).c_str()); // TODO: Come up with some tag
            }
          }
          switch (newTaskState) {
            case 0:
              EINK().drawStatusBar("Enter Task Name:");
              break;
            case 1:
              EINK().drawStatusBar("Due Date (YYYYMMDD):");
              break;
          }

          EINK().refresh();
        }
        break;
    case TASKS1:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        // DRAW APP
        EINK().drawStatusBar("T:" + tasks[selectedTask][0]);
        display.drawBitmap(0, 0, tasksApp1, 320, 218, GxEPD_BLACK);

        EINK().refresh();
      }
      break;
    
  }
}
#endif

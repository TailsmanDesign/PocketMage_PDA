#include <pocketmage.h> 
#include <Adafruit_MPR121.h>

Adafruit_MPR121 cap =  Adafruit_MPR121(); // Touch slider

// Initialization of capacative touch class
static PocketmageTOUCH pm_touch(cap);

static constexpr const char* TAG = "TOUCH";

// Setup for Touch Class
void setupTouch(){
  // Production Device Setup
  #if POCKETMAGE_HW_VERSION == 2
    if (!cap.begin(MPR121_ADDR, &Wire1)) {
      ESP_LOGE(TAG, "TouchPad Failed");
      OLED().sysMessage("Touchpad Failed",1000);
    }
    cap.setAutoconfig(true);
  #else
    // MPR121 / SLIDER
    if (!cap.begin(MPR121_ADDR)) {
      ESP_LOGE(TAG, "TouchPad Failed");
      OLED().sysMessage("Touchpad Failed",1000);
    }
    cap.setAutoconfig(true);
  #endif
}

// Access for other apps
PocketmageTOUCH& TOUCH() { return pm_touch; }

void PocketmageTOUCH::updateScrollFromTouch() {
  uint16_t touched = cap_.touched();
  int newTouch = -1;

  // Prioritize lowest pad index to cleanly handle physical finger overlap
  for (int i = 0; i < 9; ++i) {
    if (touched & (1 << i)) { 
      newTouch = i; 
      break; 
    }
  }

  unsigned long now = millis();

  if (newTouch != -1) {
    // reset timeout
    CLOCK().setPrevTimeMillis(millis());

    if (lastTouch_ != -1) {
      int d = abs(newTouch - lastTouch_);
      if (d <= 2) {
        int maxScroll = max(0, (int)allLines.size() - EINK().maxLines());
        if (newTouch > lastTouch_) {
          dynamicScroll_ = min((long)(dynamicScroll_ + 1), (long)maxScroll);
        } else if (newTouch < lastTouch_) {
          dynamicScroll_ = max((long)(dynamicScroll_ - 1), 0L);
        }
      }
    }
    lastTouch_ = newTouch;
    lastTouchTime_ = now;
  } else if (lastTouch_ != -1 && (now - lastTouchTime_ > TOUCH_TIMEOUT_MS)) {
    lastTouch_ = -1;
    if (prev_dynamicScroll_ != dynamicScroll_)
      newLineAdded = true;
  }
}

bool PocketmageTOUCH::updateScroll(int maxScroll, ulong& lineScroll, int stepSize) {

  static int lastTouchPos = -1;
  static unsigned long lastTouchTime = 0;
  static int prev_lineScroll = 0;
  bool updateScreen = false;

  uint16_t touched = cap_.touched();  // Read touch state
  int touchPos = -1;

  // Find the first active touch point (lowest index first)
  for (int i = 0; i < 9; i++) {
    if (touched & (1 << i)) {
      touchPos = i;
      break;
    }
  }

  unsigned long currentTime = millis();

  if (touchPos != -1) {  // If a touch is detected
    // reset timeout
    CLOCK().setPrevTimeMillis(millis());

    if (lastTouchPos != -1) {  // Compare with previous touch
      int touchDelta = abs(touchPos - lastTouchPos);
      if (touchDelta <= 2) {  // Ignore large jumps

        // REVERSED SCROLL DIRECTION:
        if (touchPos < lastTouchPos && lineScroll < maxScroll) {
          prev_lineScroll = lineScroll;
          lineScroll = min(lineScroll + stepSize, (ulong)maxScroll);
        } else if (touchPos > lastTouchPos && lineScroll > 0) {
          prev_lineScroll = lineScroll;
          lineScroll = (lineScroll >= (ulong)stepSize) ? lineScroll - stepSize : 0;
        }
      }
    }

    lastTouchPos = touchPos;      // update tracked touch
    lastTouch_ = touchPos;        // <--- update UI flag
    lastTouchTime = currentTime;  // reset timeout
  } else if (lastTouchPos != -1 && (currentTime - lastTouchTime > TOUCH_TIMEOUT_MS)) {
    // Timeout: reset both
    lastTouchPos = -1;
    lastTouch_ = -1;  // <--- reset UI flag

    if (prev_lineScroll != lineScroll) {
      updateScreen = true;
    }

    prev_lineScroll = lineScroll;
  }
  return updateScreen;
}

int PocketmageTOUCH::getScrollVector() {
  static int lastTouchPos = -1;
  static unsigned long lastTouchTime = 0;
  int scrollVector = 0;

  uint16_t touched = cap_.touched();
  int touchPos = -1;

  // Find the first active touch point (lowest index first)
  for (int i = 0; i < 9; i++) {
    if (touched & (1 << i)) {
      touchPos = i;
      break;
    }
  }

  unsigned long currentTime = millis();

  if (touchPos != -1) {  
    // Reset system timeout so the device doesn't sleep while scrolling
    CLOCK().setPrevTimeMillis(currentTime);

    if (lastTouchPos != -1) {  
      int touchDelta = abs(touchPos - lastTouchPos);
      if (touchDelta > 0 && touchDelta <= 2) {  // Ignore large jumps / palm rejection
        
        // Calculate the directional vector. 
        // Example: Moving from pad 4 to pad 3 returns +1. Pad 3 to 4 returns -1.
        scrollVector = lastTouchPos - touchPos;
      }
    }

    lastTouchPos = touchPos;      // update tracked touch
    lastTouch_ = touchPos;        // update UI flag
    lastTouchTime = currentTime;  // reset timeout
    
  } else if (lastTouchPos != -1 && (currentTime - lastTouchTime > TOUCH_TIMEOUT_MS)) {
    // Timeout: reset both tracking variables
    lastTouchPos = -1;
    lastTouch_ = -1; 
  }

  return scrollVector;
}
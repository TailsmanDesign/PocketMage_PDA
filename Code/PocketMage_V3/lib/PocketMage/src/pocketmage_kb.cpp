// dP     dP  88888888b dP    dP  888888ba   .88888.   .d888888   888888ba  888888ba  //
// 88   .d8'  88        Y8.  .8P  88    `8b d8'   `8b d8'    88   88    `8b 88    `8b //
// 88aaa8P'  a88aaaa     Y8aa8P  a88aaaa8P' 88     88 88aaaaa88a a88aaaa8P' 88     88 //
// 88   `8b.  88           88     88   `8b. 88     88 88     88   88   `8b. 88     88 //
// 88     88  88           88     88    .88 Y8.   .8P 88     88   88     88 88    .8P //
// dP     dP  88888888P    dP     88888888P  `8888P'  88     88   dP     dP 8888888P  //
                    
#include <pocketmage.h>

#pragma region usb keyboard
// =========================================== USB Keyboard =========================================== //

#include "driver/gpio.h"
#include "usb/usb_host.h"

#include "hid_host.h"
#include "hid_usage_keyboard.h"
#include "hid_usage_mouse.h"

static constexpr const char* TAG = "KB";
/* GPIO Pin number for quit from example logic */
#define APP_QUIT_PIN                GPIO_NUM_0
#define MAX_USB_KB_CHARS 64

static char usb_kb_chars[MAX_USB_KB_CHARS] = {0};  // unused slots initialized to '\0'

QueueHandle_t hid_host_event_queue;
bool user_shutdown = false;
static bool HIDInitialized = false;
static TaskHandle_t usb_lib_task_handle = NULL;  // MOD: store handle to usb_lib_task
static TaskHandle_t hid_host_task_handle = NULL; // MOD: store handle to hid_host_task

// Adds a character to the first available slot (if not full)
void push_USB_char(char c) {
    for (int i = 0; i < MAX_USB_KB_CHARS; i++) {
        if (usb_kb_chars[i] == '\0') {  // '\0' means unused
            usb_kb_chars[i] = c;
            return;  // stop after adding one char
        }
    }
    // Array full — do nothing
}

// Removes and returns the first character (FIFO)
char pop_USB_char() {
    char c = '\0';  // default return value (nothing to pop)
    if (usb_kb_chars[0] == '\0') return c;  // empty buffer

    c = usb_kb_chars[0];

    // Shift all remaining characters left by one
    for (int i = 1; i < MAX_USB_KB_CHARS; i++) {
        usb_kb_chars[i - 1] = usb_kb_chars[i];
    }

    usb_kb_chars[MAX_USB_KB_CHARS - 1] = '\0';  // mark last slot unused
    return c;
}


/**
 * @brief HID Host event
 *
 * This event is used for delivering the HID Host event from callback to a task.
 */
typedef struct {
  hid_host_device_handle_t hid_device_handle;
  hid_host_driver_event_t event;
  void *arg;
} hid_host_event_queue_t;

/**
 * @brief HID Protocol string names
 */
static const char *hid_proto_name_str[] = {"NONE", "KEYBOARD", "MOUSE"};

/**
 * @brief Key event
 */
typedef struct {
  enum key_state { KEY_STATE_PRESSED = 0x00, KEY_STATE_RELEASED = 0x01 } state;
  uint8_t modifier;
  uint8_t key_code;
} key_event_t;

/* When set to 1 pressing ENTER will be extending with LineFeed during serial
 * debug output */
#define KEYBOARD_ENTER_LF_EXTEND 1

/**
 * @brief Scancode to ascii table
 */
const uint8_t keycode2ascii[57][2] = {
    {0, 0},     /* HID_KEY_NO_PRESS        */
    {0, 0},     /* HID_KEY_ROLLOVER        */
    {0, 0},     /* HID_KEY_POST_FAIL       */
    {0, 0},     /* HID_KEY_ERROR_UNDEFINED */
    {'a', 'A'}, /* HID_KEY_A               */
    {'b', 'B'}, /* HID_KEY_B               */
    {'c', 'C'}, /* HID_KEY_C               */
    {'d', 'D'}, /* HID_KEY_D               */
    {'e', 'E'}, /* HID_KEY_E               */
    {'f', 'F'}, /* HID_KEY_F               */
    {'g', 'G'}, /* HID_KEY_G               */
    {'h', 'H'}, /* HID_KEY_H               */
    {'i', 'I'}, /* HID_KEY_I               */
    {'j', 'J'}, /* HID_KEY_J               */
    {'k', 'K'}, /* HID_KEY_K               */
    {'l', 'L'}, /* HID_KEY_L               */
    {'m', 'M'}, /* HID_KEY_M               */
    {'n', 'N'}, /* HID_KEY_N               */
    {'o', 'O'}, /* HID_KEY_O               */
    {'p', 'P'}, /* HID_KEY_P               */
    {'q', 'Q'}, /* HID_KEY_Q               */
    {'r', 'R'}, /* HID_KEY_R               */
    {'s', 'S'}, /* HID_KEY_S               */
    {'t', 'T'}, /* HID_KEY_T               */
    {'u', 'U'}, /* HID_KEY_U               */
    {'v', 'V'}, /* HID_KEY_V               */
    {'w', 'W'}, /* HID_KEY_W               */
    {'x', 'X'}, /* HID_KEY_X               */
    {'y', 'Y'}, /* HID_KEY_Y               */
    {'z', 'Z'}, /* HID_KEY_Z               */
    {'1', '!'}, /* HID_KEY_1               */
    {'2', '@'}, /* HID_KEY_2               */
    {'3', '#'}, /* HID_KEY_3               */
    {'4', '$'}, /* HID_KEY_4               */
    {'5', '%'}, /* HID_KEY_5               */
    {'6', '^'}, /* HID_KEY_6               */
    {'7', '&'}, /* HID_KEY_7               */
    {'8', '*'}, /* HID_KEY_8               */
    {'9', '('}, /* HID_KEY_9               */
    {'0', ')'}, /* HID_KEY_0               */
    {13 , 13 }, /* HID_KEY_ENTER           */
    {12 , 12 }, /* HID_KEY_ESC             */
    {8  , 8  }, /* HID_KEY_DEL             */
    {28 , 28 }, /* HID_KEY_TAB             */
    {' ', ' '}, /* HID_KEY_SPACE           */
    {'-', 28 }, /* HID_KEY_MINUS          _     */
    {'=', 30 }, /* HID_KEY_EQUAL          +     */
    {'[', '{'}, /* HID_KEY_OPEN_BRACKET    */
    {']', '}'}, /* HID_KEY_CLOSE_BRACKET   */
    {'\\','|'}, /* HID_KEY_BACK_SLASH      */
    {'\\','|'},
    /* HID_KEY_SHARP           */ // HOTFIX: for NonUS Keyboards repeat
                                  // HID_KEY_BACK_SLASH
    {';', ':'},                   /* HID_KEY_COLON           */
    {'\'','"'},                  /* HID_KEY_QUOTE           */
    {'`', '~'},                   /* HID_KEY_TILDE           */
    {',', '<'},                   /* HID_KEY_LESS            */
    {'.', '>'},                   /* HID_KEY_GREATER         */
    {'/', '?'}                    /* HID_KEY_SLASH           */
};

/**
 * @brief Makes new line depending on report output protocol type
 *
 * @param[in] proto Current protocol to output
 */
static void hid_print_new_device_report_header(hid_protocol_t proto) {
  static hid_protocol_t prev_proto_output = HID_PROTOCOL_MAX;

  if (prev_proto_output != proto) {
    prev_proto_output = proto;
    printf("\r\n");
    if (proto == HID_PROTOCOL_MOUSE) {
      printf("Mouse\r\n");
    } else if (proto == HID_PROTOCOL_KEYBOARD) {
      printf("Keyboard\r\n");
    } else {
      printf("Generic\r\n");
    }
    fflush(stdout);
  }
}

/**
 * @brief HID Keyboard modifier verification for capitalization application
 * (right or left shift)
 *
 * @param[in] modifier
 * @return true  Modifier was pressed (left or right shift)
 * @return false Modifier was not pressed (left or right shift)
 *
 */
static inline bool hid_keyboard_is_modifier_shift(uint8_t modifier) {
  if (((modifier & HID_LEFT_SHIFT) == HID_LEFT_SHIFT) ||
      ((modifier & HID_RIGHT_SHIFT) == HID_RIGHT_SHIFT)) {
    return true;
  }
  return false;
}

/**
 * @brief HID Keyboard get char symbol from key code
 *
 * @param[in] modifier  Keyboard modifier data
 * @param[in] key_code  Keyboard key code
 * @param[in] key_char  Pointer to key char data
 *
 * @return true  Key scancode converted successfully
 * @return false Key scancode unknown
 */
static inline bool hid_keyboard_get_char(uint8_t modifier, uint8_t key_code,
                                         unsigned char *key_char) {
  uint8_t mod = (hid_keyboard_is_modifier_shift(modifier)) ? 1 : 0;

  if ((key_code >= HID_KEY_A) && (key_code <= HID_KEY_SLASH)) {
    *key_char = keycode2ascii[key_code][mod];
  } else {
    // All other key pressed
    return false;
  }

  return true;
}

/**
 * @brief HID Keyboard print char symbol
 *
 * @param[in] key_char  Keyboard char to stdout
 */
static inline void hid_keyboard_print_char(unsigned int key_char) {
  if (!!key_char) {
    //putchar(key_char);
    //OLED().oledWord(String(key_char));
    push_USB_char(key_char);
    fflush(stdout);
  }
}

/**
 * @brief Key Event. Key event with the key code, state and modifier.
 *
 * @param[in] key_event Pointer to Key Event structure
 *
 */
static void key_event_callback(key_event_t *key_event) {
  unsigned char key_char;

  hid_print_new_device_report_header(HID_PROTOCOL_KEYBOARD);

  if (key_event->KEY_STATE_PRESSED == key_event->state) {
    if (hid_keyboard_get_char(key_event->modifier, key_event->key_code,
                              &key_char)) {

      hid_keyboard_print_char(key_char);
    }
  }
}

/**
 * @brief Key buffer scan code search.
 *
 * @param[in] src       Pointer to source buffer where to search
 * @param[in] key       Key scancode to search
 * @param[in] length    Size of the source buffer
 */
static inline bool key_found(const uint8_t *const src, uint8_t key,
                             unsigned int length) {
  for (unsigned int i = 0; i < length; i++) {
    if (src[i] == key) {
      return true;
    }
  }
  return false;
}

/**
 * @brief USB HID Host Keyboard Interface report callback handler
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_keyboard_report_callback(const uint8_t *const data,
                                              const int length) {
  hid_keyboard_input_report_boot_t *kb_report =
      (hid_keyboard_input_report_boot_t *)data;

  if (length < sizeof(hid_keyboard_input_report_boot_t)) {
    return;
  }

  static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = {0};
  key_event_t key_event;

  for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {

    // key has been released verification
    if (prev_keys[i] > HID_KEY_ERROR_UNDEFINED &&
        !key_found(kb_report->key, prev_keys[i], HID_KEYBOARD_KEY_MAX)) {
      key_event.key_code = prev_keys[i];
      key_event.modifier = 0;
      key_event.state = key_event.KEY_STATE_RELEASED;
      key_event_callback(&key_event);
    }

    // key has been pressed verification
    if (kb_report->key[i] > HID_KEY_ERROR_UNDEFINED &&
        !key_found(prev_keys, kb_report->key[i], HID_KEYBOARD_KEY_MAX)) {
      key_event.key_code = kb_report->key[i];
      key_event.modifier = kb_report->modifier.val;
      key_event.state = key_event.KEY_STATE_PRESSED;
      key_event_callback(&key_event);
    }
  }

  memcpy(prev_keys, &kb_report->key, HID_KEYBOARD_KEY_MAX);
}

/**
 * @brief USB HID Host Mouse Interface report callback handler
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_mouse_report_callback(const uint8_t *const data,
                                           const int length) {
  hid_mouse_input_report_boot_t *mouse_report =
      (hid_mouse_input_report_boot_t *)data;

  if (length < sizeof(hid_mouse_input_report_boot_t)) {
    return;
  }

  static int x_pos = 0;
  static int y_pos = 0;

  // Calculate absolute position from displacement
  x_pos += mouse_report->x_displacement;
  y_pos += mouse_report->y_displacement;

  hid_print_new_device_report_header(HID_PROTOCOL_MOUSE);

  printf("X: %06d\tY: %06d\t|%c|%c|\r", x_pos, y_pos,
         (mouse_report->buttons.button1 ? 'o' : ' '),
         (mouse_report->buttons.button2 ? 'o' : ' '));
  fflush(stdout);
}

/**
 * @brief USB HID Host Generic Interface report callback handler
 *
 * 'generic' means anything else than mouse or keyboard
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_generic_report_callback(const uint8_t *const data,
                                             const int length) {
  hid_print_new_device_report_header(HID_PROTOCOL_NONE);
  for (int i = 0; i < length; i++) {
    printf("%02X", data[i]);
  }
  putchar('\r');
  putchar('\n');
  fflush(stdout);
}

/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg) {
  uint8_t data[64] = {0};
  size_t data_length = 0;
  hid_host_dev_params_t dev_params;
  ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

  switch (event) {
  case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
    ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(
        hid_device_handle, data, 64, &data_length));

    if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
      if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
        hid_host_keyboard_report_callback(data, data_length);
      } else if (HID_PROTOCOL_MOUSE == dev_params.proto) {
        hid_host_mouse_report_callback(data, data_length);
      }
    } else {
      hid_host_generic_report_callback(data, data_length);
    }

    break;
  case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
             hid_proto_name_str[dev_params.proto]);
    ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
    break;
  case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
    ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
             hid_proto_name_str[dev_params.proto]);
    break;
  default:
    ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event",
             hid_proto_name_str[dev_params.proto]);
    break;
  }
}

/**
 * @brief USB HID Host Device event
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host Device event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event, void *arg) {
  hid_host_dev_params_t dev_params;
  ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));
  const hid_host_device_config_t dev_config = {
      .callback = hid_host_interface_callback, .callback_arg = NULL};


  switch (event) {
  case HID_HOST_DRIVER_EVENT_CONNECTED:
    ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED",
             hid_proto_name_str[dev_params.proto]);

    ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
    if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
      ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle,
                                                     HID_REPORT_PROTOCOL_BOOT));
      if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
        ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
      }
    }
    ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
    break;
  default:
    break;
  }
}

/**
 * @brief Start USB Host install and handle common USB host library events while
 * app pin not low
 *
 * @param[in] arg  Not used
 */
static void usb_lib_task(void *arg) {
  const gpio_config_t input_pin = {
      .pin_bit_mask = BIT64(APP_QUIT_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&input_pin));

  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };

  ESP_ERROR_CHECK(usb_host_install(&host_config));
  xTaskNotifyGive((TaskHandle_t)arg);

  while (gpio_get_level(APP_QUIT_PIN) != 0) {
    uint32_t event_flags;
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

    // Release devices once all clients has deregistered
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      usb_host_device_free_all();
      ESP_LOGI(TAG, "USB Event flags: NO_CLIENTS");
    }
    // All devices were removed
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
      ESP_LOGI(TAG, "USB Event flags: ALL_FREE");
    }
  }
  // App Button was pressed, trigger the flag
  user_shutdown = true;
  ESP_LOGI(TAG, "USB shutdown");
  // Clean up USB Host
  vTaskDelay(10); // Short delay to allow clients clean-up
  ESP_ERROR_CHECK(usb_host_uninstall());
  vTaskDelete(NULL);
}

/**
 * @brief HID Host main task
 *
 * Creates queue and get new event from the queue
 *
 * @param[in] pvParameters Not used
 */
void hid_host_task(void *pvParameters) {
  hid_host_event_queue_t evt_queue;
  // Create queue
  hid_host_event_queue = xQueueCreate(10, sizeof(hid_host_event_queue_t));

  // Wait queue
  while (!user_shutdown) {
    if (xQueueReceive(hid_host_event_queue, &evt_queue, pdMS_TO_TICKS(50))) {
      hid_host_device_event(evt_queue.hid_device_handle, evt_queue.event,
                            evt_queue.arg);
    }
  }

  xQueueReset(hid_host_event_queue);
  vQueueDelete(hid_host_event_queue);
  vTaskDelete(NULL);
}

/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event, void *arg) {
  const hid_host_event_queue_t evt_queue = {
      .hid_device_handle = hid_device_handle, .event = event, .arg = arg};
  xQueueSend(hid_host_event_queue, &evt_queue, 0);
}

void init_USBHID(void) {
  BaseType_t task_created;
  ESP_LOGI(TAG, "Init USB HID");

  /*
   * Create usb_lib_task to:
   * - initialize USB Host library
   * - Handle USB Host events while APP pin is in HIGH state
   */
  task_created = xTaskCreatePinnedToCore(
                    usb_lib_task, 
                    "usb_events", 
                    4096,
                    xTaskGetCurrentTaskHandle(), 
                    2, 
                    &usb_lib_task_handle, 
                    1);
  assert(task_created == pdTRUE);

  // Wait for notification from usb_lib_task to proceed
  ulTaskNotifyTake(false, 1000);

  /*
   * HID host driver configuration
   * - create background task for handling low level event inside the HID driver
   * - provide the device callback to get new HID Device connection event
   */
  const hid_host_driver_config_t hid_host_driver_config = {
      .create_background_task = true,
      .task_priority = 5,
      .stack_size = 4096,
      .core_id = 0,
      .callback = hid_host_device_callback,
      .callback_arg = NULL};

  ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

  // Task is working until the devices are gone (while 'user_shutdown' is false)
  user_shutdown = false;

  /*
   * Create HID Host task process for handle events
   * IMPORTANT: Task is necessary here while there is no possibility to interact
   * with USB device from the callback.
   */
  task_created = xTaskCreate(
                    &hid_host_task, 
                    "hid_task", 
                    4 * 1024, 
                    NULL, 
                    2, 
                    &hid_host_task_handle);
  assert(task_created == pdTRUE);
}

void close_USBHID(void) {
  ESP_LOGI(TAG, "Closing USB HID...");

  // Signal shutdown
  user_shutdown = true;

  // Give tasks a moment to process shutdown
  vTaskDelay(pdMS_TO_TICKS(100));

  // --- Step 1: Request HID Host to stop ---
  esp_err_t err = hid_host_uninstall();
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to uninstall HID host: %s", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "HID host uninstalled");
  }

  // --- Step 2: Notify USB library to finish events ---
  usb_host_lib_handle_events(0, 0);  // Force event loop one more time

  // --- Step 3: Wait for all clients to detach ---
  bool all_clients_gone = false;
  for (int i = 0; i < 50; i++) {  // wait up to ~5s
    uint32_t event_flags;
    if (usb_host_lib_handle_events(1, &event_flags) == ESP_OK) {
      if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
        all_clients_gone = true;
        break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (!all_clients_gone) {
    ESP_LOGW(TAG, "USB clients did not detach in time");
  } else {
    ESP_LOGI(TAG, "All USB clients detached");
  }

  // --- Step 4: Uninstall USB host ---
  err = usb_host_uninstall();
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to uninstall USB host: %s", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "USB host uninstalled");
  }

  // --- Step 5: Cleanup event queue ---
  if (hid_host_event_queue) {
    xQueueReset(hid_host_event_queue);
    vQueueDelete(hid_host_event_queue);
    hid_host_event_queue = NULL;
    ESP_LOGI(TAG, "HID host event queue deleted");
  }

  // --- Step 6: Delete leftover tasks (if still alive) ---
  if (hid_host_task_handle) {
    vTaskDelete(hid_host_task_handle);
    hid_host_task_handle = NULL;
  }
  if (usb_lib_task_handle) {
    vTaskDelete(usb_lib_task_handle);
    usb_lib_task_handle = NULL;
  }

  // Reset flag for next re-init
  user_shutdown = false;

  ESP_LOGI(TAG, "USB HID closed successfully");
}


#pragma region keymaps
// ===================== Keymaps =====================
char currentKB[4][10];            // Current keyboard layout (remove)

char keysArray[4][10] = {
    { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' },
    { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',   8 },  // 8:BKSP
    {   9, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.',  13 },  // 9:TAB, 13:CR
    {   0,  17,  18, ' ', ' ', ' ',  19,  20,  21,   0 }   // 17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};
char keysArraySHFT[4][10] = {
    { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
    { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',   8 },
    {   9, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '\'',  13 },
    {   0,  17,  18, ' ', ' ', ' ',  28,  29,  30,   0 }
};
char keysArrayFN[4][10] = {
    { '1', '2', '3', '4', '5', '6', '7',  '8',  '9', '0' },
    { '#', '!', '$', ':', ';', '(', ')',  '&', '\"',   8 },
    {  14, '%', '_', '+', '-', '*', '/',  '?',  ',',  13 },
    {   0,  17,  18, ' ', ' ', ' ',  12,    7,    6,   0 }
};
char keysArrayFN_SHFT[4][10] = {
    { '!', '@', '#', '$', '%', '^', '&',  '*',  '(', ')' },
    { '~', '`', '|', '[', ']', '{', '}',  '<',  '>',   8 },
    {  14, '%', '=', '&', '+', '-', '\\', '?',  ',',  13 },
    {   0,  17,  18, ' ', ' ', ' ',  24,   25,   26,   0 }
};
#pragma endregion


Adafruit_TCA8418 keypad;
// To Do:
// make currentKBState a member of PocketmageKB and change all references in main apps/libraries

// Initialization of kb class
static PocketmageKB pm_kb(keypad);

void IRAM_ATTR KB_irq_handler() { KB().setTCA8418Event(); }

// Setup for keyboard class
void setupKB(int KB_irq_pin) {
  if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    ESP_LOGE(TAG, "Error Initializing the Keyboard");
    OLED().oledWord("Keyboard INIT Failed");
    delay(500);
    //while (1);
    esp_restart();
  }
  keypad.matrix(4, 10);
  wireKB();
  attachInterrupt(digitalPinToInterrupt(KB_irq_pin), KB_irq_handler, FALLING);
  //keypad.flush();
  KB().setTCA8418Event();
  keypad.enableInterrupts();
}

// Wire function for keyboard class
// add any global references here + add set function to class header file
void wireKB() {
}

// Access for other apps
PocketmageKB& KB() { return pm_kb; }


// ===================== public functions =====================
char PocketmageKB::updateKeypress() {
  // Check for USB char
  char USB_CHAR = pop_USB_char();
  if (USB_CHAR != '\0') {
    return USB_CHAR;
  }

  // Check for keypad char
  if (TCA8418_event_ == true) {
    int k = keypad_.getEvent();
    
    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad_.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad_.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0) TCA8418_event_ = false;

    if (k & 0x80) {   //Key pressed, not released
      k &= 0x7F;
      k--;
      //return currentKB[k/10][k%10];
      if ((k/10) < 4) {
        //Key was pressed, reset timeout counter
        CLOCK().setPrevTimeMillis(millis());

        //Return Key
        switch (kbState_) {
          case 0:
            return keysArray[k/10][k%10];
          case 1:
            return keysArraySHFT[k/10][k%10];
          case 2:
            return keysArrayFN[k/10][k%10];
          case 3:
            return keysArrayFN_SHFT[k/10][k%10];
          default:
            return 0;
        }
      }
    }
  }

  return 0;

}

void PocketmageKB::checkUSBKB() {
  // Check if USB Keyboard has been connected
  bool needBoost;
  PowerSystem.getOTGNeed(needBoost);
  if (needBoost) {
    // Enable boost if not already on
    bool boostOn;
    if (PowerSystem.getBoostState(boostOn) && !boostOn) {
        PowerSystem.setBoost(true);
    }

    // Connect D+/D− to ESP so USB host can enumerate keyboard
    PowerSystem.setUSBControlESP();

    // Initialize USB HID
    if (!HIDInitialized) {
      init_USBHID();
      HIDInitialized = true;
    }
  
    // Set tags
    mscEnabled = true; 
    sinkEnabled = true;

  }
  else {

    #pragma message "TODO: Should probably add shutdown script here but it does not work..."
    // close_USBHID();

    // Disable boost if not already off
    bool boostOn;
    if (PowerSystem.getBoostState(boostOn) && boostOn) {
      PowerSystem.setBoost(false);
      detachInterrupt(digitalPinToInterrupt(PWR_BTN));
      attachInterrupt(digitalPinToInterrupt(PWR_BTN), pocketmage::PWR_BTN_irq, FALLING);
    }

    PowerSystem.setUSBControlBMS();

    // Set tags
    mscEnabled = false; 
    sinkEnabled = false;
  }
}

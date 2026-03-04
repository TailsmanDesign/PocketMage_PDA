//  888888ba  dP     dP d8888888P d8888888P  88888888b  888888ba  //
//  88    `8b 88     88      .d8'      .d8'  88         88    `8b //
// a88aaaa8P' 88     88    .d8'      .d8'   a88aaaa    a88aaaa8P' //
//  88   `8b. 88     88  .d8'      .d8'      88         88   `8b. //
//  88    .88 Y8.   .8P d8'       d8'        88         88     88 //
//  88888888P `Y88888P' Y8888888P Y8888888P  88888888P  dP     dP //

#include <pocketmage.h>


// Initialization of bz class
static PocketmageBZ pm_bz;
/*
uint32_t bzFrequency = 440;

const ledc_timer_config_t ledc_timer{
    .speed_mode       = LEDC_LOW_SPEED_MODE,
    .duty_resolution  = PWM_RESOLUTION,
    .timer_num        = LEDC_TIMER_1,
    .freq_hz          = bzFrequency,
    .clk_cfg          = LEDC_AUTO_CLK,
};




// Configure LEDC channel
const ledc_channel_config_t ledc_channel = {
    .gpio_num = BZ_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_1,
    .timer_sel = LEDC_TIMER_1,
    .duty = 0,
};

*/
PocketmageBZ::PocketmageBZ()
  : buzzer_(BZ_PIN) {}

bool PocketmageBZ::begin(int channel) {
  channel_ = channel;
  buzzer_.begin(channel_);
  begun_ = true;
  return true;
}

void PocketmageBZ::end() {
  if (!begun_) return;
  buzzer_.end(channel_);
  begun_ = false;
}

// Setup for Buzzer Class
void setupBZ() {
  //ledc_timer_config(&ledc_timer);
  //ledc_channel_config(&ledc_channel);
  auto& bz = BZ();
  bz.begin();
}

// Access for other apps
PocketmageBZ& BZ() { return pm_bz; }

// ===================== main functions =====================
void PocketmageBZ::playJingle(const Jingle& jingle) {
  if (jingle.notes == nullptr || jingle.len == 0) {
    return;  // No valid notes to play
  }

  if (!begun_) {
    begin(channel_);
  }

  for (size_t i = 0; i < jingle.len; ++i) {
    buzzer_.sound(jingle.notes[i].key, jingle.notes[i].duration);
  }

  buzzer_.sound(0, 80);  // End the sound
  buzzer_.end(channel_);        // Stop the buzzer
  begun_ = false;
}
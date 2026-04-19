
#ifndef ASSETS_H
#define ASSETS_H

#include <pgmspace.h>

#if !OTA_APP // POCKETMAGE_OS
//
extern const unsigned char backgroundaero [] PROGMEM;
extern const unsigned char backgroundbliss [] PROGMEM;
extern const int backgroundallArray_LEN;
extern const unsigned char* backgroundallArray[2];

//
extern const unsigned char textApp [] PROGMEM;

//
extern const unsigned char _homeIcons2 [] PROGMEM;
extern const unsigned char _homeIcons3 [] PROGMEM;
extern const unsigned char _homeIcons4 [] PROGMEM;
extern const unsigned char _homeIcons5 [] PROGMEM;
extern const unsigned char _homeIcons6 [] PROGMEM;
extern const unsigned char _homeIcons7 [] PROGMEM;
extern const unsigned char _homeIcons8 [] PROGMEM;
extern const unsigned char _homeIcons9 [] PROGMEM;
extern const unsigned char _homeIcons10 [] PROGMEM;
extern const unsigned char _homeIcons11 [] PROGMEM;
extern const unsigned char* homeIconsAllArray[10];

extern const unsigned char _noIconFound [] PROGMEM;

//
extern const unsigned char fileWizardfileWiz0 [] PROGMEM;
extern const unsigned char fileWizardfileWiz1 [] PROGMEM;
extern const unsigned char fileWizardfileWiz2 [] PROGMEM;
extern const unsigned char fileWizardfileWiz3 [] PROGMEM;
extern const int fileWizardallArray_LEN;
extern const unsigned char* fileWizardallArray[4];

//
extern const unsigned char fileWizLitefileWizLite0 [] PROGMEM;
extern const unsigned char fileWizLitefileWizLite1 [] PROGMEM;
extern const unsigned char fileWizLitefileWizLite2 [] PROGMEM;
extern const unsigned char fileWizLitefileWizLite3 [] PROGMEM;
extern const int fileWizLiteallArray_LEN;
extern const unsigned char* fileWizLiteallArray[4];

//
extern const unsigned char nowLaternowAndLater0 [] PROGMEM;
extern const unsigned char nowLaternowAndLater1 [] PROGMEM;
extern const unsigned char nowLaternowAndLater2 [] PROGMEM;
extern const unsigned char nowLaternowAndLater3 [] PROGMEM;
extern const int nowLaterallArray_LEN;
extern const unsigned char* nowLaterallArray[4];

//
extern const unsigned char tasksApp0 [] PROGMEM;
extern const unsigned char tasksApp1 [] PROGMEM;

//
extern const unsigned char taskIconTasks0 [] PROGMEM;

//
extern const unsigned char fontfont0 [] PROGMEM;

//
extern const unsigned char _settings [] PROGMEM;

//
extern const unsigned char _toggle [] PROGMEM;
extern const unsigned char _toggleON [] PROGMEM;
extern const unsigned char _toggleOFF [] PROGMEM;

//
extern const unsigned char _calendar00 [] PROGMEM;
extern const unsigned char _calendar01 [] PROGMEM;
extern const unsigned char _calendar02 [] PROGMEM;
extern const unsigned char _calendar03 [] PROGMEM;
extern const unsigned char _calendar04 [] PROGMEM;
extern const unsigned char _calendar05 [] PROGMEM;
extern const unsigned char _calendar06 [] PROGMEM;
extern const unsigned char _calendar07 [] PROGMEM;
extern const unsigned char _calendar08 [] PROGMEM;
extern const unsigned char _calendar09 [] PROGMEM;
extern const unsigned char _calendar10 [] PROGMEM;
extern const unsigned char* calendar_allArray[11];

//
extern const unsigned char _lex0 [] PROGMEM;
extern const unsigned char _lex1 [] PROGMEM;
extern const unsigned char* lex_allArray[2];

//
extern const unsigned char _usb [] PROGMEM;

//
extern const unsigned char _eventMarker0 [] PROGMEM;
extern const unsigned char _eventMarker1 [] PROGMEM;

//
extern const unsigned char _journal [] PROGMEM;

//
extern const unsigned char _appLoader [] PROGMEM;

// File icons
extern const unsigned char _LFileIcon0 [] PROGMEM;
extern const unsigned char _LFileIcon1 [] PROGMEM;
extern const unsigned char _LFileIcon2 [] PROGMEM;
extern const unsigned char _LFileIcon3 [] PROGMEM;
extern const unsigned char* _LFileIcons[4];

extern const unsigned char _SFileIcon0 [] PROGMEM;
extern const unsigned char _SFileIcon1 [] PROGMEM;
extern const unsigned char _SFileIcon2 [] PROGMEM;
extern const unsigned char _SFileIcon3 [] PROGMEM;
extern const unsigned char* _SFileIcons[4];


// Mage idle left frames
extern const unsigned char _mage_idle_left0[] PROGMEM;
extern const unsigned char _mage_idle_left1[] PROGMEM;
extern const unsigned char _mage_idle_left2[] PROGMEM;
extern const unsigned char _mage_idle_left3[] PROGMEM;
extern const unsigned char _mage_idle_left4[] PROGMEM;
extern const unsigned char _mage_idle_left5[] PROGMEM;
extern const unsigned char _mage_idle_left6[] PROGMEM;
extern const unsigned char* idle_left_allArray[7];

// Mage idle right frames
extern const unsigned char _mage_idle_right0[] PROGMEM;
extern const unsigned char _mage_idle_right1[] PROGMEM;
extern const unsigned char _mage_idle_right2[] PROGMEM;
extern const unsigned char _mage_idle_right3[] PROGMEM;
extern const unsigned char _mage_idle_right4[] PROGMEM;
extern const unsigned char _mage_idle_right5[] PROGMEM;
extern const unsigned char _mage_idle_right6[] PROGMEM;
extern const unsigned char* idle_right_allArray[7];

// Transition left frames
extern const unsigned char _transition_left0[] PROGMEM;
extern const unsigned char _transition_left1[] PROGMEM;
extern const unsigned char _transition_left2[] PROGMEM;
extern const unsigned char _transition_left3[] PROGMEM;
extern const unsigned char _transition_left4[] PROGMEM;
extern const unsigned char* trans_left_allArray[5];

// Transition right frames
extern const unsigned char _transition_right0[] PROGMEM;
extern const unsigned char _transition_right1[] PROGMEM;
extern const unsigned char _transition_right2[] PROGMEM;
extern const unsigned char _transition_right3[] PROGMEM;
extern const unsigned char _transition_right4[] PROGMEM;
extern const unsigned char* trans_right_allArray[5];

// Mage running left frames
extern const unsigned char _mage_run_left0[] PROGMEM;
extern const unsigned char _mage_run_left1[] PROGMEM;
extern const unsigned char _mage_run_left2[] PROGMEM;
extern const unsigned char _mage_run_left3[] PROGMEM;
extern const unsigned char _mage_run_left4[] PROGMEM;
extern const unsigned char _mage_run_left5[] PROGMEM;
extern const unsigned char _mage_run_left6[] PROGMEM;
extern const unsigned char* run_left_allArray[7];

// Mage running right frames
extern const unsigned char _mage_run_right0[] PROGMEM;
extern const unsigned char _mage_run_right1[] PROGMEM;
extern const unsigned char _mage_run_right2[] PROGMEM;
extern const unsigned char _mage_run_right3[] PROGMEM;
extern const unsigned char _mage_run_right4[] PROGMEM;
extern const unsigned char _mage_run_right5[] PROGMEM;
extern const unsigned char _mage_run_right6[] PROGMEM;
extern const unsigned char* run_right_allArray[7];

extern const unsigned char leftRightIndicator0[] PROGMEM;
extern const unsigned char leftRightIndicator1[] PROGMEM;
extern const unsigned char leftRightIndicator2[] PROGMEM;

extern const unsigned char timeInput[] PROGMEM;
extern const unsigned char dateInput[] PROGMEM;

extern const unsigned char noIcon [] PROGMEM;

extern const unsigned char _repeatGUI0 [] PROGMEM;
extern const unsigned char _repeatGUI1 [] PROGMEM;
extern const unsigned char _repeatGUI2 [] PROGMEM;

#endif // POCKETMAGE_OS
#endif
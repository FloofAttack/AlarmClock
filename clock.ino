#include "SevSeg.h"
SevSeg sevseg;  //Instantiate a seven segment controller object

// global timing variable
volatile int fifty_hz = 0;
// these need to go here in order to get the button press delay to work
unsigned long startMillis = 0;
unsigned long currentMillis = 0;
unsigned long startAlarmMillis = 0;
unsigned long currentAlarmMillis = 0;

void setup() {
  byte numDigits = 4;
  byte digitPins[] = { 0, 3, 4, 5 };  //swapped pin2 to pin0 - might fuck with the USB?
  byte segmentPins[] = { 6, 7, 8, 9, 10, 11, 12 };
  bool resistorsOnSegments = true;       // 'true' means resistors are on segment pins
  byte hardwareConfig = COMMON_CATHODE;  // See README.md for options
  bool updateWithDelays = false;         // Default 'false' is Recommended
  bool leadingZeros = true;              // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = true;           // Use 'true' if your decimal point doesn't exist or isn't connected
  // TODO: fix the decimal point, can we make it flash?

  // time and alarm setting buttons
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  // alarm enable switch
  pinMode(A2, INPUT_PULLUP);
  // output to light decimal point when alarm enabled
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(-30);  // -200 to 200

  // take in external 50Hz timing reference
  // can only use pins 2 and 3 as interrupts.
  pinMode(2, INPUT_PULLUP);  // 20kΩ internal pullup enabled
  attachInterrupt(digitalPinToInterrupt(2), mains_ref, FALLING);
}

void loop() {

  static int seconds = 0;
  static int minutes = 0;
  static int hours = 0;
  static int alarm_minutes = 0;
  static int alarm_hours = 0;
  static int set_time_counter = 0;

  // read in what button is being pressed.
  static bool press_0;
  static bool press_1;
  // variables to track time setting mode and alarm mode
  static bool time_setting_mode = false;
  static bool alarm_setting_mode = false;
  static bool alarm_active = false;
  bool alarm_flag = false;
  int snooze_mins = 5;

  // always keep track of starting number of milliseconds
  // create some modes: time setting mode
  // long press (3s) of A0
  press_0 = !digitalRead(A0);
  press_1 = !digitalRead(A1);
  alarm_active = !digitalRead(A2);

  // using the 50Hz signal to toggle the alarm every 0.2 seconds
  if ((minutes == alarm_minutes) && (hours == alarm_hours) && (alarm_active)) {
    if (fifty_hz <= 20) {
      alarm_flag = true;
    } else {
      alarm_flag = false;
    }
  }

  // A4 goes high if alarm_flag = true
  if (alarm_active) {
    digitalWrite(A4, HIGH);
  } else {
    digitalWrite(A4, LOW);
  }
  // snooze statement
  if ((minutes == alarm_minutes) && (hours == alarm_hours) && (alarm_active) && (press_0 || press_1)) {
    if (alarm_minutes + snooze_mins > 59) {
      alarm_minutes = alarm_minutes + snooze_mins - 60;
      if (alarm_hours == 23) {
        alarm_hours = 0;
      } else {
        alarm_hours++;
      }
    } else {
      alarm_minutes = alarm_minutes + snooze_mins;
    }
  }

  // if statement to sound alarm.
  if (alarm_flag) {
    tone(A5, 400);
  } else {
    noTone(A5);
    digitalWrite(A5, LOW);
  }


  // set mode detection variables
  // be explicit about what buttons are and are NOT being pressed for each mode
  if ((press_0 || press_1) && !(press_0 && press_1) && !alarm_setting_mode && !time_setting_mode) {
    if (press_0 && !press_1) {
      currentMillis = millis();
      if ((currentMillis - startMillis) > 3000) {  // button held for 3 seconds
        time_setting_mode = true;
      }
    }
    if (press_1 && !press_0 && !time_setting_mode && !alarm_setting_mode) {
      currentMillis = millis();
      if ((currentMillis - startMillis) > 3000) {  // button held for 3 seconds
        alarm_setting_mode = true;
      }
    }
  } else {
    startMillis = millis();
  }

  // double press both to quit modes
  if (press_0 && press_1) {
    time_setting_mode = false;
    alarm_setting_mode = false;
  }

  if (!alarm_setting_mode && time_setting_mode && press_0 && !press_1) {
    if (set_time_counter == 1000) {
      set_time_counter = 0;
      if (minutes == 59) {
        minutes = 0;
      } else {
        minutes++;
      }
    } else {
      set_time_counter++;
    }
  }
  if (!alarm_setting_mode && time_setting_mode && press_1 && !press_0) {
    if (set_time_counter == 1000) {
      set_time_counter = 0;
      if (hours == 23) {
        hours = 0;
      } else {
        hours++;
      }
    } else {
      set_time_counter++;
    }
  }

  if (!alarm_active && !time_setting_mode && alarm_setting_mode && press_0 && !press_1) {
    if (set_time_counter == 1000) {
      set_time_counter = 0;
      if (alarm_minutes == 59) {
        alarm_minutes = 0;
      } else {
        alarm_minutes++;
      }
    } else {
      set_time_counter++;
    }
  }
  if (!alarm_active && !time_setting_mode && alarm_setting_mode && press_1 && !press_0) {
    if (set_time_counter == 1000) {
      set_time_counter = 0;
      if (alarm_hours == 23) {
        alarm_hours = 0;
      } else {
        alarm_hours++;
      }
    } else {
      set_time_counter++;
    }
  }

  // tell the time as normal
  if (fifty_hz == 50) {
    fifty_hz = 0;
    if (seconds == 59) {
      seconds = 0;
      if (minutes == 59) {
        minutes = 0;
        if (hours == 23) {
          hours = 0;
        } else {
          hours++;
        }
      } else {
        minutes++;
      }
    } else {
      seconds++;
    }
  }
  if (alarm_setting_mode) {
    sevseg.setNumber(100 * alarm_hours + alarm_minutes, 1);
  } else {
    sevseg.setNumber(100 * hours + minutes, 1);
  }

  sevseg.refreshDisplay();  // Must run repeatedly
}

void mains_ref() {
  fifty_hz++;
}
/// END ///

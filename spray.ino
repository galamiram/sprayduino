

#include "EEPROMAnything.h"
#include <LiquidCrystal_I2C.h>
#include <ClickEncoder.h>
#include "Print.h"
#include "Spray.h"

#define ENCODER_STEPS_PER_NOTCH    4   // Change this depending on which encoder is used
#define ENCODER_PINA               3
#define ENCODER_PINB               2
#define ENCODER_BTN                4
#define LED_PIN                    13
#define RELAY_A_PIN                6
#define RELAY_B_PIN                5

const int numOfScreens = 7;
const int goToSleepAfter = 10000;
ClickEncoder      *encoder;
LiquidCrystal_I2C *lcd;

String onOff[2]     = {"Off","On"};
String trueFalse[2] = {"False","True"};

unsigned long backLightTimer = 0;

int currentScreen = 0;

int storageFlagAddr = 0;
bool storageEnabled = false;

String screens[numOfScreens][2] = {{"Spray 1 Enabled",""}, {"Spray 2 Enabled",""},
                                   {"Spray 1 Interval","Seconds"}, {"Spray 2 Interval","Seconds"},
                                   {"Spray 1 Duration","Seconds"}, {"Spray 2 Duration","Seconds"},
                                   {"Factory Reset", "Reset"}
};

bool inMenu       = false;
bool showMenu     = false;
bool inputFlag    = false;
bool screenChange = true;
bool btnClk       = false;
bool btnDblClk    = false;
int  rotary       = 0;

Spray *spray1;
Spray *spray2;

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);

  encoder = new ClickEncoder(ENCODER_PINA, ENCODER_PINB, ENCODER_BTN, ENCODER_STEPS_PER_NOTCH);
  encoder->setAccelerationEnabled(false);
  encoder->setButtonHeldEnabled(true);
  encoder->setDoubleClickEnabled(true);
  encoder->setAccelerationEnabled(false);

  attachInterrupt(ENCODER_BTN,printMenu,RISING);
  
  lcd = new LiquidCrystal_I2C(0x27,20,4);
  lcd->init();                      // initialize the lcd 
  lcd->backlight();
  
  storageEnabled = checkStorageEnabled();
  
  spray1 = new Spray(RELAY_A_PIN, 2, 8, true, storageEnabled, 100);
  spray2 = new Spray(RELAY_B_PIN, 1, 10, true, storageEnabled, 200);
  
  if (!storageEnabled) {
    EEPROM_writeAnything(storageFlagAddr, 1);
  }
}

void(* resetFunc) (void) = 0;

void loop() {
  if (inputFlag || screenChange) {
    if (showMenu) {
      printMenu();
    } else {
      printMainScreen();
    }
  }
 
  if (spray1->check() && !showMenu) screenChange = true;
  if (spray2->check() && !showMenu) screenChange = true;
 
  readEncoderState();

  if (millis() > (backLightTimer + goToSleepAfter)) {
    screenOff();
  }
}

void printMainScreen() {
  screenChange = false;
  
  lcd->clear();
  lcd->print("SPRAY 1.0");
  lcd->setCursor(0,1);
  lcd->print("S1:");
  lcd->print(onOff[spray1->getState()]);
  lcd->print(" | ");
  lcd->print("S2:");
  lcd->print(onOff[spray2->getState()]);
  if (btnClk) {
    showMenu = true;
    screenChange = true;
  }
}

void printMenu() {
  screenChange = false;
  unsigned int val, oldVal, minVal, maxVal;
  String txt;
  switch (currentScreen) {
    case 0:
      val = spray1->getConf().sprayEnabled;
      minVal = 0;
      maxVal = 1;
      txt = trueFalse[val];
      break;
    case 1:
      val = spray2->getConf().sprayEnabled;
      minVal = 0;
      maxVal = 1;
      txt = trueFalse[val];
      break;
    case 2:
      val = spray1->getConf().timeOff;
      minVal = 1;
      maxVal = 120;
      txt = String(val);
      break;
    case 3:
      val = spray2->getConf().timeOff;
      minVal = 1;
      maxVal = 120;
      txt = String(val);
      break;
    case 4:
      val = spray1->getConf().timeOn;
      minVal = 1;
      maxVal = 10;
      txt = String(val);
      break;
    case 5:
      val = spray2->getConf().timeOn;
      minVal = 1;
      maxVal = 10;
      txt = String(val);
      break;
  }
  oldVal = val;
  
  lcd->clear();
  lcd->print(screens[currentScreen][0]);
  lcd->setCursor(0,1);
  if (inMenu) lcd->print(">");
  lcd->print(txt);
  lcd->print(" ");
  lcd->print(screens[currentScreen][1]);
  
  if (btnDblClk) {
     showMenu = false;
     screenChange = true;
  }
  
  if (btnClk) {
    if (inMenu) {
      inMenu = false;
    } else {
      if (currentScreen == 6) {
        Serial.println("reset");
        clearStorage();
      }
      inMenu = true;
    }
    screenChange = true;
  }
  
  if (inMenu) {
     if(rotary < 0) {
      if (val == minVal) {
        val = maxVal;
      }else{
        val--;
      }
      screenChange = true;
    } else {
      if(rotary > 0) {
        if (val == maxVal) {
          val = minVal;
        }else{
          val++;
        }
        screenChange = true;
      }
    }
  } else {
    if(rotary < 0) {
      if (currentScreen == 0) {
        currentScreen = numOfScreens-1;
      }else{
        currentScreen--;
      }
      screenChange = true;
    } else {
      if(rotary > 0) {
        if (currentScreen == numOfScreens-1) {
          currentScreen = 0;
        }else{
          currentScreen++;
        }
        screenChange = true;
      }
    }
  }

  if (val != oldVal) {
    switch (currentScreen) {
      case 0:
        spray1->updateEnabled(val,storageEnabled);
        break;
      case 1:
        spray2->updateEnabled(val,storageEnabled);
        break;
      case 2:
        spray1->updateTimeOff(val,storageEnabled);
        break;
      case 3:
        spray2->updateTimeOff(val,storageEnabled);
        break;
      case 4:
        spray1->updateTimeOn(val,storageEnabled);
        break;
      case 5:
        spray2->updateTimeOn(val,storageEnabled);
        break;
    }
  }
}

void screenOff() {
  lcd->noBacklight();
}

void clearStorage() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  resetFunc();
}

void readEncoderState() {
  resetInput();
  //Call Service in loop becasue using timer interrupts may affect ESP8266 WIFI
  //however call no more than 1 time per millisecond to reduce encoder bounce
  static uint32_t lastService = 0;
  if (lastService + 1000 < micros()) {
    lastService = micros();                
    encoder->service();  
  }

  static int16_t value;
  value = encoder->getValue();
  if (value != 0) {
    inputDetected();
    rotary = value;
  }

  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Clicked:
        Serial.println("click ");
        inputDetected();
        btnClk = true;
        break;
       case ClickEncoder::DoubleClicked:
        Serial.println("dblclick");
        inputDetected();
        btnDblClk = true;
        break;
    }
  }
}

void resetInput() {
  inputFlag = false;
  btnClk    = false;
  btnDblClk = false;
  rotary    = 0;
}

bool checkStorageEnabled() {
  int enabled;
  EEPROM_readAnything(storageFlagAddr, enabled);
  Serial.println(enabled);
  if (enabled == 1) return true;
  return false;
}

void inputDetected() {
  inputFlag = true;
  backLightTimer = millis();
  lcd->backlight();
}

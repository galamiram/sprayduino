struct config_spray
{
    bool sprayEnabled;
    unsigned int timeOn;
    unsigned int timeOff;
};

class Spray {
  private:
    byte pin;
    bool sprayState = false;
    config_spray conf;
    int storageAddr;
    unsigned long nextChangeTime = 0;

  public:
    Spray(byte pin, unsigned int timeOn, unsigned int timeOff, bool enabled, bool storageEnabled, int storageAddr) {
      this->pin = pin;
      this->storageAddr = storageAddr;
      if (storageEnabled) {
        Serial.println("read from storage");
        EEPROM_readAnything(storageAddr, this->conf);
      } else {
        Serial.println("no storage");
        this->conf.timeOn = timeOn;
        this->conf.timeOff = timeOff;
        this->conf.sprayEnabled = enabled;
        EEPROM_writeAnything(storageAddr, this->conf);
      }
      pinMode(pin, OUTPUT);
    }

    void updateEnabled(bool enabled, bool storageEnabled) {
      this->conf.sprayEnabled = enabled;
      if (storageEnabled) {
        EEPROM_writeAnything(storageAddr, this->conf);
      }
    }

    void updateTimeOn(unsigned int timeOn, bool storageEnabled) {
      this->conf.timeOn = timeOn;
      if (storageEnabled) {
        EEPROM_writeAnything(storageAddr, this->conf);
      }
    }

    void updateTimeOff(unsigned int timeOff, bool storageEnabled) {
      this->conf.timeOff = timeOff;
      if (storageEnabled) {
        EEPROM_writeAnything(storageAddr, this->conf);
      }
    }

    // Checks whether it is time to turn on or off the LED.
    bool check() {
      if (conf.sprayEnabled == false) {
        return false;
      }
      unsigned long currentTime = millis();
      if(currentTime >= nextChangeTime) {
        if(sprayState) {
          // LED is currently turned On. Turn Off LED.
          sprayState = LOW;
          nextChangeTime = currentTime + conf.timeOff*1000;
        }
        else{
          // LED is currently turned Off. Turn On LED.
          sprayState = HIGH;
          nextChangeTime = currentTime + conf.timeOn*1000;
        }

        digitalWrite(pin, sprayState);
        return true;
      }
      return false;
    }

    bool getState() {
      return this-> sprayState;
    }

    config_spray getConf() {
      return this->conf;
    }
};

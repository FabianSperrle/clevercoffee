/**
 * @file StatusLED.h
 *
 * @brief Implementation of LED behaviour
 */

#pragma once

unsigned long brightnessLED;

void statusLEDSetup() {
    if (STATUS_LED == 1) {
        pinMode(PIN_STATUSLED, OUTPUT);
    }

    if (STATUS_LED == 2) {
        FastLED.addLeds<RGB_LED_TYPE, PIN_STATUSLED>(leds, NUM_LEDS);
    }
}

void loopLED() {
#if STATUS_LED == 1
    //analog LED
    if ((machineState == kPidNormal && (fabs(temperature  - setpoint) < 0.3)) || (temperature > 115 && fabs(temperature - setpoint) < 5)) {
        digitalWrite(PIN_STATUSLED, HIGH);
    }
    else {
        digitalWrite(PIN_STATUSLED, LOW);
    }
#endif

#if STATUS_LED == 2
    unsigned long currentMillisLED = millis();

    if (currentMillisLED - previousMillisLED >= intervalLED) {
        previousMillisLED = currentMillisLED;

        // Heating to setpoint or Standby -> Switch LEDs off, but overwrite with other states
        // fill_solid(leds, NUM_LEDS, CRGB::White); 
        fill_solid(leds, NUM_LEDS, CHSV(0, 0, 140)); 

        if (machineState == kStandby) {
            fill_solid(leds, NUM_LEDS, CRGB::Black); 
        }

        // [For LED_BEHAVIOUR 2]:
        if (LED_BEHAVIOUR == 2) {
            // Brew coffee -> Blue
            if (machineState == kBrew) {
                fill_solid(leds, NUM_LEDS, CRGB::White); 
            }

            // Set higher temperature for Steam -> dark yellow (if not at correct temperature, then its green)
            if (machineState == kSteam && !(temperature > 115 && fabs(temperature - setpoint) < 5) ) {
                fill_solid(leds, NUM_LEDS, CHSV(45,255,100)); //45 yellow, 100 is 40% brighteness
            }

            // Backflush -> White
            if (machineState == kBackflush) {
                fill_solid(leds, NUM_LEDS, CRGB::Black); 
            }        
         }

        // After coffee is brewed -> White light to show ready coffee for ~15s
        if (BrewFinishedLEDon > 0) {
            BrewFinishedLEDon--;
            // for the last 10% of the time, smoothly dim down white light on cup
            brightnessLED = (BrewFinishedLEDon > (BrewFinishedLEDonDuration / 10) ? 255 : 255*BrewFinishedLEDon*10/BrewFinishedLEDonDuration);
            fill_solid(leds, NUM_LEDS, CHSV(0, 0, brightnessLED)); 
        }
        
        // Error message: Red (Water empty, sensor error, etc) - overrides all other states in LED color
        if (!waterFull || machineState == kSensorError || machineState ==  kEepromError || machineState ==  kEmergencyStop) {
            if (!waterFull) {
                // [For LED_BEHAVIOUR 2]: Red heartbeat
                fill_solid(leds, NUM_LEDS, CRGB::Blue);
            }
            else {
                // [For LED_BEHAVIOUR 1]: Solid red color
                fill_solid(leds, NUM_LEDS, CRGB::Red); 
            }
        }
        
        FastLED.show(); 
    }
#endif

}
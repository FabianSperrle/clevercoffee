/**
 * @file scalevoid.h
 *
 * @brief Implementation of scale initialization and weight measurement
 */

#pragma once

#if (BREWMODE == 2 || ONLYPIDSCALE == 1)

void calibrate(HX711_ADC loadCell, int pin, sto_item_id_t name, float *calibration) {
    loadCell.setCalFactor(1.0);
    u8g2.clearBuffer();
    u8g2.drawStr(0, 22, "Calibration coming up");
    u8g2.drawStr(0, 32, "Empty scale ");
    u8g2.print(pin, 0);
    u8g2.sendBuffer();
    debugPrintf("Taking scale %d to zero point\n", pin);
    loadCell.update();
    loadCell.tare();
    delay(1000);
    debugPrintf("Put load on scale %d within the next 10 seconds\n", pin);
    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration in progress.");
    u8g2.drawStr(2, 12, "Place known weight");
    u8g2.drawStr(2, 22, "on scale in next");
    u8g2.drawStr(2, 32," 10 seconds");
    u8g2.drawStr(2, 42, number2string(scaleKnownWeight));
    u8g2.sendBuffer();
    delay(10000);
    debugPrintf("Taking scale load point\n");
    loadCell.refreshDataSet();  
    float cali = loadCell.getNewCalibration(scaleKnownWeight);
    debugPrintf("New calibration: %f\n", cali);
    u8g2.sendBuffer();
    storageSet(name, cali, true);
    *calibration = cali;
    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration done!");
    u8g2.drawStr(2, 12, "New calibration:");
    u8g2.drawStr(2, 22, number2string(cali));
    u8g2.sendBuffer();
    delay(2000);
}

float w1 = 0.0;
float w2 = 0.0;

/**
 * @brief Check measured weight
 */
void checkWeight() {
    unsigned long currentMillisScale = millis();

    if (scaleFailure) {   // abort if scale is not working
        return;
    }

    static boolean newDataReady = 0;
    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;

    if (newDataReady) {
        if (currentMillisScale - previousMillisScale >= intervalWeight * 10) {
        previousMillisScale = currentMillisScale;
            w1 = LoadCell.getData();
            Serial.println(w1);
        }
    }

    #if SINGLE_HX711 == 0
    unsigned long currentMillisScale2 = millis();
    if (LoadCell2.update()) {
        if (currentMillisScale - previousMillisScale2 >= intervalWeight) {
        previousMillisScale2 = currentMillisScale2;
            w2 = LoadCell2.getData();
            debugPrintf("Got new weight W2: %f.2\n", w2);
            debugPrintf("Current weights: %f.2 and %f.2\n", w1, w2);
            debugPrintf("Current total: %f.2\n", w1 + w2);
        }
    }
    weight = w1 + w2;
    #else 
    weight = w1;
    #endif

    if (calibrationON) {
        #if SINGLE_HX711 == 1
        while (!LoadCell.update());
        calibrate(LoadCell, PIN_HXDAT, STO_ITEM_SCALE_CALIBRATION_FACTOR, &scaleCalibration);
        #else 
        while (!LoadCell.update());
        calibrate(LoadCell, PIN_HXDAT, STO_ITEM_SCALE_CALIBRATION_FACTOR, &scaleCalibration);
        while (!LoadCell2.update());
        calibrate(LoadCell2, PIN_HXDAT2, STO_ITEM_SCALE2_CALIBRATION_FACTOR, &scale2Calibration);
        #endif
        calibrationON = 0;
    }

    if (tareON) {
        tareON = 0;
        u8g2.clearBuffer();
        u8g2.drawStr(0, 2, "Taring scale,");
        u8g2.drawStr(0, 12, "remove any load!");
        u8g2.drawStr(0, 22, "....");
        delay(2000);
        u8g2.sendBuffer();
        LoadCell.tare();
        #if SINGLE_HX711 == 0
        LoadCell2.tare();
        #endif
        u8g2.drawStr(0, 32, "done");
        u8g2.sendBuffer();
        delay(2000);
    }
    
}

boolean initScale(HX711_ADC loadCell, int pin, bool calibrate, sto_item_id_t name, float* scaleCalibration);

void initScale() {
    boolean shouldCalibrate = calibrationON;
    if(!initScale(LoadCell, 0, shouldCalibrate, STO_ITEM_SCALE_CALIBRATION_FACTOR, &scaleCalibration)) {
        scaleFailure = true;
    }
    #if SINGLE_HX711 == 0
    if(!initScale(LoadCell2, 0, shouldCalibrate, STO_ITEM_SCALE2_CALIBRATION_FACTOR, &scale2Calibration)) {
        scaleFailure = true;
    }
    #endif

    calibrationON = 0;
}

/**
 * @brief Initialize scale
 * @return true iff the calibration fails, else false.
 */
boolean initScale(HX711_ADC loadCell, int pin, bool shouldCalibrate, sto_item_id_t name, float* calibrationFactor) {
    loadCell.begin();
    long stabilizingtime = 5000; // tare preciscion can be improved by adding a few seconds of stabilizing time
    boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
    u8g2.clearBuffer();
    u8g2.drawStr(0, 2, "Taring scale @PIN: "); // TODO add pin to display output
    u8g2.print(pin, 0); 
    u8g2.drawStr(0, 12, "remove any load!");
    u8g2.drawStr(0, 22, "....");
    u8g2.sendBuffer();
    delay(2000);
    loadCell.start(stabilizingtime, _tare);
    delay(5000);

    if (loadCell.getTareTimeoutFlag() || loadCell.getSignalTimeoutFlag() ) {
        debugPrintf("Timeout, check MCU>HX711 wiring and pin designations");
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
        u8g2.sendBuffer();
        delay(5000);
        return false;
    }
    else {
        if (shouldCalibrate) {
            loadCell.setCalFactor(1.0);
            while (!loadCell.update());
            calibrate(loadCell, pin, name, calibrationFactor);
            // loadCell.setSamplesInUse(SCALE_SAMPLES);
            u8g2.drawStr(0, 52, "done.");
            delay(2000);
        }
        else {
            loadCell.setCalFactor(scaleCalibration); // set calibration factor (float)
            // loadCell.setSamplesInUse(SCALE_SAMPLES);
            u8g2.drawStr(0, 42, number2string(scaleCalibration));
            u8g2.drawStr(0, 52, "done.");
            u8g2.sendBuffer();
            delay(2000);
        }
    }

    return true;
}


/**
 * @brief Scale with shot timer
 */
void shottimerscale() {
    switch (shottimercounter)  {
        case 10:    // waiting step for brew switch turning on
        if (preinfusionpause == 0 || preinfusion == 0) {
            if (timeBrewed > 0) {
                weightPreBrew = weight;
                shottimercounter = 20;
            }
        } else {
            if (timeBrewed > preinfusion*1000) {
                weightPreBrew = weight;
                shottimercounter = 20;
            }
        }

            break;

        case 20:
            weightBrew = weight - weightPreBrew;

            if (timeBrewed == 0) {
                shottimercounter = 10;
            }

            break;
    }
}
#endif

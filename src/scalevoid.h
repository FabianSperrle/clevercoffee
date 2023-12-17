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
    debugPrintf("Put load on scale %d within the next 10 seconds\n", pin);
    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration in progress.");
    u8g2.drawStr(2, 12, "Place known weight");
    u8g2.drawStr(2, 22, "on scale in next");
    u8g2.drawStr(2, 32,"10 seconds");
    u8g2.drawStr(2, 42, number2string(scaleKnownWeight));
    u8g2.sendBuffer();
    delay(10000);
    debugPrintf("Taking scale load point\n");
    // increase scale samples temporarily to ensure a stable reading
    loadCell.setSamplesInUse(128);
    loadCell.refreshDataSet();  
    *calibration = loadCell.getNewCalibration(scaleKnownWeight);
    loadCell.setSamplesInUse(SCALE_SAMPLES);
    debugPrintf("New calibration: %f\n", *calibration);
    u8g2.sendBuffer();
    storageSet(name, *calibration, true);
    u8g2.clearBuffer();
    u8g2.drawStr(2, 2, "Calibration done!");
    u8g2.drawStr(2, 12, "New calibration:");
    u8g2.drawStr(2, 22, number2string(*calibration));
    u8g2.sendBuffer();
    delay(2000);
}

float w1 = 0.0;
float w2 = 0.0;

/**
 * @brief Check measured weight
 */
void checkWeight() {
    static boolean newDataReady = 0;
    unsigned long currentMillisScale = millis();

    if (scaleFailure) {   // abort if scale is not working
        return;
    }

    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;
    #if SINGLE_HX711 == 0
    // weirdly, the library examples do not check for updates on the second cell before getting the values...
    LoadCell2.update();
    #endif

    if (newDataReady) {
        if (currentMillisScale - previousMillisScale >= intervalWeight) {
            previousMillisScale = currentMillisScale;
            newDataReady = false;
            w1 = LoadCell.getData();

            #if SINGLE_HX711 == 0
            w2 = LoadCell2.getData();
            debugPrintf("Current weight: %.2f from %.2f and %.2f\n", weight, w1, w2);
            #endif
        }
    }

    #if SINGLE_HX711 == 0
    weight = w1 + w2;
    #else 
    weight = w1;
    #endif

    if (calibrationON) {
        calibrate(LoadCell, PIN_HXDAT, STO_ITEM_SCALE_CALIBRATION_FACTOR, &scaleCalibration);
        #if SINGLE_HX711 == 0
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
        LoadCell.setCalFactor(scaleCalibration);
        #if SINGLE_HX711 == 0
        LoadCell2.setCalFactor(scale2Calibration);
        LoadCell2.tare();
        #endif
        u8g2.drawStr(0, 32, "done");
        u8g2.sendBuffer();
        delay(2000);
    }
    
}

void initScale() {
    boolean shouldCalibrate = calibrationON;

    LoadCell.begin();
    #if SINGLE_HX711 == 0
    LoadCell2.begin();
    #endif 
    // u8g2.clearBuffer();
    // u8g2.drawStr(0, 2, "Taring scale"); 
    // u8g2.drawStr(0, 12, "remove any load!");
    // u8g2.drawStr(0, 22, "....");
    // u8g2.sendBuffer();
    // delay(2000);
    #if SINGLE_HX711 == 1
    while(!LoadCell.startMultiple(stabilizingtime, _tare));
    #else 
    byte loadCellReady = 0;
    byte loadCell2Ready = 0;
    // run startup, stabilization and tare, both modules simultaniously
    // this parallel start seems to be the most important part to get accurate readings with two HX711s connected
    while ((loadCellReady + loadCell2Ready) < 2) { 
        if (!loadCellReady) loadCellReady = LoadCell.startMultiple(5000, true);
        if (!loadCell2Ready) loadCell2Ready = LoadCell2.startMultiple(5000, true);
    }
    #endif

    if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag() ) {
        debugPrintf("Timeout, check MCU>HX711 wiring for scale");
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
        u8g2.sendBuffer();
        delay(5000);
        scaleFailure = true;
        return;
    }

    #if SINGLE_HX711 == 0
    if (LoadCell2.getTareTimeoutFlag() || LoadCell2.getSignalTimeoutFlag() ) {
        debugPrintf("Timeout, check MCU>HX711 wiring for scale 2");
        u8g2.drawStr(0, 32, "failed!");
        u8g2.drawStr(0, 42, "Scale not working...");    // scale timeout will most likely trigger after OTA update, but will still work after boot
        u8g2.sendBuffer();
        delay(5000);
        scaleFailure = true;
        return;
    }
    #endif 

    LoadCell.setCalFactor(scaleCalibration); 
    LoadCell.setSamplesInUse(SCALE_SAMPLES);

    #if SINGLE_HX711 == 0
    LoadCell2.setCalFactor(scale2Calibration); 
    LoadCell2.setSamplesInUse(SCALE_SAMPLES);
    #endif 

    calibrationON = 0;
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

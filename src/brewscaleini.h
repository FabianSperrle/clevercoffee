/**
 * @file brewscaleini.h
 *
 * @brief TODO
 *
 */

#pragma once
#include <pinmapping.h>


enum BrewState {
    kBrewIdle = 10,
    kPreinfusion = 20,
    kWaitPreinfusion = 21,
    kPreinfusionPause = 30,
    kWaitPreinfusionPause = 31,
    kBrewRunning = 40,
    kWaitBrew = 41,
    kBrewFinished = 42,
    kBrewWaitTrickle = 43,
    kWaitBrewOff = 44
};

// Normal Brew
BrewState brewcounter = kBrewIdle;
int brewswitch = 0;
int brewswitchTrigger = LOW;
int buttonStateBrewTrigger;                     // the current reading from the input pin
unsigned long lastDebounceTimeBrewTrigger = 0;  // the last time the output pin was toggled
unsigned long debounceDelayBrewTrigger = 50;
unsigned long brewswitchTriggermillis = 0;
int brewswitchTriggerCase = 10;
boolean brewswitchWasOFF = false;
double totalBrewTime = 0;                           // total brewtime set in software
double timeBrewed = 0;                              // total brewed time
double lastbrewTimeMillis = 0;                      // for shottimer delay after disarmed button
double lastbrewTime = 0 ;
unsigned long startingTime = 0;                     // start time of brew
boolean brewPIDdisabled = false;                    // is PID disabled for delay after brew has started?
const unsigned long analogreadingtimeinterval = 10; // ms
unsigned long previousMillistempanalogreading;      // ms for analogreading

// Shot timer with or without scale
#if (ONLYPIDSCALE == 1 || BREWMODE == 2)
    boolean calibrationON = 0;
    boolean tareON = 0;
    int shottimercounter = 10 ;
    float weight = 0;                                   // value from HX711
    float weightPreBrew = 0;                            // value of scale before wrew started
    float weightBrew = 0;                               // weight value of brew
    float scaleDelayValue = 2.5;                        // value in gramm that takes still flows onto the scale after brew is stopped
    bool scaleFailure = false;
    const unsigned long intervalWeight = 50;           // weight scale
    unsigned long previousMillisScale;                  // initialisation at the end of init()
    unsigned long timeTrickleStarted = 0;
    unsigned long timeTrickling = 0;
    HX711_ADC LoadCell(PIN_HXDAT, PIN_HXCLK);

    #if SINGLE_HX711 == 0 
    HX711_ADC LoadCell2(PIN_HXDAT2, PIN_HXCLK);
    #endif

    // flow rate calculation
    unsigned long prevFlowRateTime = 0;
    float prevFlowRateWeight = 0.0;
    float flowRate = 0.0;
    float flowRateEmaAlpha = 0.05;
#endif

#include <Arduino.h>
#include "config.h"
#include<avr/wdt.h>


Config config;

typedef void (*Func)();

void on_pressure_sensor_trigger() {
    const auto now = millis();
    config.outputs.dispenser.trigger(now);
    config.outputs.solenoid.trigger(now);
    config.outputs.lighter.trigger(now);
}


void setup() {
// write your initialization code here
    Serial.begin(115200);
    const auto now = millis();
    wdt_enable(WDTO_8S);  /* Enable the watchdog with a timeout of 2 seconds */
    info(now, "[ROAR] Starting...");
    info(now, "[ROAR] Configuring...");
    config.inputs.setup();
    config.outputs.setup();
    config.inputs.dump();
    config.outputs.dump();
    config.inputs.pressure.subscribe_high(on_pressure_sensor_trigger);
    info(now, "[ROAR] Configured");
    info(now, "[ROAR] Started");
}

void loop() {
// write your code here
    const auto now = millis();
    config.inputs.tick(now);
    config.outputs.tick(now);
    wdt_reset();
}
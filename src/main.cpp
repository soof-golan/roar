#include <Arduino.h>
#include "config.h"
#include<avr/wdt.h>


Config config;

typedef void (*Func)();

void on_pressure_sensor_trigger() {
    config.outputs.dispenser.trigger();
    config.outputs.solenoid.trigger();
    config.outputs.lighter.trigger();
}


void setup() {
// write your initialization code here
    Serial.begin(115200);
    wdt_enable(WDTO_8S);  /* Enable the watchdog with a timeout of 2 seconds */
    Serial.println("[ROAR] Starting...");
    Serial.println("[ROAR] Configuring...");
    config.inputs.setup();
    config.outputs.setup();
    config.inputs.dump();
    config.outputs.dump();
    config.inputs.pressure.subscribe_high(on_pressure_sensor_trigger);
    Serial.println("[ROAR] Configured");
    Serial.println("[ROAR] Started");
}

void loop() {
// write your code here
    config.inputs.tick();
    config.outputs.tick();
    wdt_reset();
}
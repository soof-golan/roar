#include <Arduino.h>
#include "config.h"
#include<avr/wdt.h>


Config config;

typedef void (*Func)();


struct InputDebounce {
    typedef struct Config {
        Pin pin;
        bool pull_up;
        bool inverted;
        Milliseconds debounce;
        String name;
    } Config;

    const Config config;

    explicit InputDebounce(const Config& config) : config(config), stable_state(config.inverted ? HIGH : LOW), last_state(stable_state),
                                   func(nullptr), last_change(millis()) {
        last_state = stable_state;
        last_change = millis();
    }

    int stable_state;
    int last_state;
    Func func;
    Milliseconds last_change;

    void setup() {
        pinMode(config.pin, config.pull_up ? INPUT_PULLUP : INPUT);
        last_state = stable_state = digitalRead(config.pin);
        last_change = millis();
    }

    void dump() const {
        Serial.println("[InputDebounce." + String(config.name) + "]");
        Serial.println("  - Pin: " + String(config.pin));
        Serial.println("  - Inverted: " + String(config.inverted));
        Serial.println("  - Debounce: " + String(config.debounce) + "ms");
    }

    void tick() {
        const auto now = millis();
        const auto reading = digitalRead(config.pin);
        const auto changed = reading != last_state;
        if (changed) {
            last_change = now;
        }
        const auto dt = now - last_change;
        if (dt >= config.debounce && stable_state != reading) {
            stable_state = reading;

            const auto up_edge = reading == config.inverted ? LOW : HIGH;
            if (up_edge) {
                Serial.println("[InputDebounce." + String(config.name) + "] Triggered");
                if (func) {
                    func();
                }
            }
        }
    }

    void subscribe_high(Func &&func_) {
        func = func_;
    }
};

InputDebounce trig(InputDebounce::Config{
        .pin=7,
        .pull_up=true,
        .inverted=true,
        .debounce=100,
        .name="trig"
});

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
    trig.setup();
    config.inputs.dump();
    config.outputs.dump();
    trig.dump();
//    config.inputs.pressure.subscribe_high(on_pressure_sensor_trigger);
    trig.subscribe_high(on_pressure_sensor_trigger);
    Serial.println("[ROAR] Configured");
    Serial.println("[ROAR] Started");
}

void loop() {
// write your code here
    config.inputs.tick();
    config.outputs.tick();
    trig.tick();
    wdt_reset();
}
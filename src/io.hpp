//
// Created by Soof Golan on 28/05/2023.
//

#ifndef ROAR_IO_HPP
#define ROAR_IO_HPP

#include <Arduino.h>
#include <EasyButton.h>
#include <AsyncDelay.h>

typedef long int Degrees;
typedef unsigned long Milliseconds;
typedef uint8_t Pin;

typedef struct Activation {
    typedef struct Config {
        Pin pin;
        Milliseconds delay;
        Milliseconds duration;
        bool inverted;
        String name;
    } Config;
    const Config config;

    AsyncDelay on_timer;
    AsyncDelay off_timer;

    explicit Activation(const Config &config) : config(config) {
        on_timer = AsyncDelay(config.delay, AsyncDelay::units_t::MILLIS);
        off_timer = AsyncDelay(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
    }

    void setup() const {
        pinMode(config.pin, OUTPUT);
        digitalWrite(config.pin, config.inverted ? HIGH : LOW);
    }

    void dump() const {
        Serial.println("[Output." + String(config.name) + "]");
        Serial.println("  - Pin: " + String(config.pin));
        Serial.println("  - Inverted: " + String(config.inverted));
        Serial.println("  - Delay: " + String(config.delay) + "ms");
        Serial.println("  - Duration: " + String(config.duration) + "ms");
    }

    void trigger() {
        if (on_timer.isExpired()) {
            Serial.println("[Output." + String(config.name) + "] Triggered");
            on_timer.start(config.delay, AsyncDelay::units_t::MILLIS);
            off_timer.start(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
        }
    }

    void tick() const {
        if (off_timer.isExpired()) {
//            Serial.println("[Output." + String(config.name) + "] Off");
            digitalWrite(config.pin, config.inverted ? HIGH : LOW);
        } else if (on_timer.isExpired()) {
            Serial.println("[Output." + String(config.name) + "] On");
            digitalWrite(config.pin, config.inverted ? LOW : HIGH);
        }
    }

} Activation;

typedef struct Servo {
    typedef struct Config {
        Pin pin;
        Milliseconds delay;
        Milliseconds duration;
        Degrees angleWhenOn;
        Degrees angleWhenOff;
        String name;
    } Config;

    // Represents the current state of the servo
    typedef struct State {
        Degrees angle;
        int pwm;
    } State;

    const Config config;
    State state;

    AsyncDelay on_timer;
    AsyncDelay off_timer;

    explicit Servo(const Config &config) : config(config), state({config.angleWhenOff, angele_to_pwm(config.angleWhenOff)}) {
        on_timer = AsyncDelay(config.delay, AsyncDelay::units_t::MILLIS);
        off_timer = AsyncDelay(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
    }

    static int angele_to_pwm(Degrees angle) {
        return (int) map(angle, 0, 180, 0, 255);
    }

    static Degrees pwm_to_angle(int pwm) {
        return (Degrees) map(pwm, 0, 255, 0, 180);
    }

    void update_state(Degrees angle) {
        state.angle = angle;
        state.pwm = angele_to_pwm(angle);
        analogWrite(config.pin, state.pwm);
    }

    void setup() {
        pinMode(config.pin, OUTPUT);
        update_state(config.angleWhenOff);
    }

    void dump() const {
        Serial.println("[Servo." + String(config.name) + "]");
        Serial.println("  - Pin: " + String(config.pin));
        Serial.println("  - Delay: " + String(config.delay) + "ms");
        Serial.println("  - Duration: " + String(config.duration) + "ms");
        Serial.println("  - Angle When On: " + String(config.angleWhenOn) + "° (PWM: " + String(angele_to_pwm(config.angleWhenOn)) + ")");
        Serial.println("  - Angle When Off: " + String(config.angleWhenOff) + "° (PWM: " + String(angele_to_pwm(config.angleWhenOff)) + ")");
    }

    void trigger() {
        if (on_timer.isExpired()) {
            Serial.println("[Servo." + String(config.name) + "] Triggered");
            on_timer.start(config.delay, AsyncDelay::units_t::MILLIS);
            off_timer.start(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
        }
    }

    void tick() {
        if (off_timer.isExpired()) {
            update_state(config.angleWhenOff);
        } else if (on_timer.isExpired()) {
            update_state(config.angleWhenOn);
            Serial.println("[Servo." + String(config.name) + "] On");
        }
    }
} Servo;

typedef struct Sensor {
    typedef struct Config {
        Pin pin;
        Milliseconds debounce;
        bool pull_up;
        bool inverted;
        String name;
    } Config;
    const Config config;


    explicit Sensor(const Config &config) : config(config), debouncer(EasyButton(
            /*pin=*/config.pin,
            /*debounce_time=*/config.debounce,
            /*pullup_enable=*/config.pull_up,
            /*active_low==*/config.inverted
    )) {}

    EasyButton debouncer;

    void setup() {
        pinMode(config.pin, config.pull_up ? INPUT_PULLUP : INPUT);
        debouncer.begin();
    }

    void dump() const {
        Serial.println("[Sensor." + String(config.name) + "]");
        Serial.println("  - Pin: " + String(config.pin));
        Serial.println("  - Inverted: " + String(config.inverted));
        Serial.println("  - Debounce: " + String(config.debounce) + "ms");
    }

    template<typename Func>
    void subscribe_high(Func &&func) {
        return debouncer.onPressedFor(config.debounce, func);
    }

    void tick() {
        debouncer.read();
        if (debouncer.wasPressed()) {
            Serial.println("[Sensor." + String(config.name) + "] Rising Edge");
        } else if (debouncer.wasReleased()) {
            Serial.println("[Sensor." + String(config.name) + "] Falling Edge");
        }
    }

} Sensor;

typedef struct Inputs {
    Sensor pressure;


    void setup() {
        pressure.setup();
    }

    void dump() const {
        Serial.println("[Inputs]");
        pressure.dump();
    }

    void tick() {
        pressure.tick();
    }
} Inputs;

typedef struct Outputs {
    Activation dispenser;
    Activation solenoid;
    Servo lighter;

    void setup() {
        dispenser.setup();
        solenoid.setup();
        lighter.setup();
    }

    void dump() const {
        Serial.println("[Outputs]");
        dispenser.dump();
        solenoid.dump();
        lighter.dump();
    }

    void tick() {
        dispenser.tick();
        solenoid.tick();
        lighter.tick();
    }
} Outputs;

#endif //ROAR_IO_HPP

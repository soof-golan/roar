//
// Created by Soof Golan on 28/05/2023.
//

#ifndef ROAR_IO_HPP
#define ROAR_IO_HPP

#include <Arduino.h>
#include <EasyButton.h>
#include <AsyncDelay.h>
#include <Servo.h>

typedef int Degrees;
typedef unsigned long Milliseconds;
typedef uint8_t Pin;

#define info(now, message) (Serial.println("[clk:" + String(now) + "] | " + String(message)))

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
    int state;

    explicit Activation(const Config &config) : config(config), state(config.inverted ? HIGH : LOW) {
        on_timer = AsyncDelay(config.delay, AsyncDelay::units_t::MILLIS);
        off_timer = AsyncDelay(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
    }

    void setup() {
        pinMode(config.pin, OUTPUT);
        digitalWrite(config.pin, state);
        on_timer.expire();
        off_timer.expire();
    }

    void dump(const Milliseconds now) const {
        info(now, "[Output." + String(config.name) + "]");
        info(now, "  - Pin: " + String(config.pin));
        info(now, "  - Inverted: " + String(config.inverted));
        info(now, "  - Delay: " + String(config.delay) + "ms");
        info(now, "  - Duration: " + String(config.duration) + "ms");
    }

    void trigger(Milliseconds now) {
        if (on_timer.isExpired()) {
            info(now, "[Output." + String(config.name) + "] Triggered");
            on_timer.start(config.delay, AsyncDelay::units_t::MILLIS);
            off_timer.start(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
        }
    }

    void tick(const Milliseconds now) {
        if (off_timer.isExpired()) {
            const auto new_state = config.inverted ? LOW : HIGH;
            if (state != new_state) {
                info(now, "[Output." + String(config.name) + "] Off | " + (config.inverted ? "LOW -> HIGH" : "HIGH -> LOW"));
            }
            state = new_state;
        } else if (on_timer.isExpired()) {
            const auto new_state = config.inverted ? HIGH : LOW;
            if (state != new_state) {
                info(now, "[Output." + String(config.name) + "] On | " + (config.inverted ? "HIGH -> LOW" : "LOW -> HIGH"));
            }
            state = new_state;
        }
        digitalWrite(config.pin, state);
    }


} Activation;

typedef struct ServoActivation {
    typedef struct Config {
        Pin pin;
        Milliseconds delay;
        Milliseconds on_duration;
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
    Servo servo;

    AsyncDelay on_timer;
    AsyncDelay off_timer;

    explicit ServoActivation(const Config &config) : config(config), state({config.angleWhenOff, angele_to_pwm(config.angleWhenOff)}) {
        on_timer = AsyncDelay(config.delay, AsyncDelay::units_t::MILLIS);
        off_timer = AsyncDelay(config.delay + config.on_duration, AsyncDelay::units_t::MILLIS);
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
    }

    void setup() {
        servo.attach(config.pin);
        state = {config.angleWhenOff, angele_to_pwm(config.angleWhenOff)};
        update_state(config.angleWhenOff);
        drive_outputs();
        on_timer.expire();
        off_timer.expire();
    }

    void dump(const Milliseconds now) const {
        info(now, "[Servo." + String(config.name) + "]");
        info(now, "  - Pin: " + String(config.pin));
        info(now, "  - Delay: " + String(config.delay) + "ms");
        info(now, "  - Duration: " + String(config.on_duration) + "ms");
        info(now, "  - Angle When On: " + String(config.angleWhenOn) + "° (PWM: " + String(angele_to_pwm(config.angleWhenOn)) + ")");
        info(now, "  - Angle When Off: " + String(config.angleWhenOff) + "° (PWM: " + String(angele_to_pwm(config.angleWhenOff)) + ")");
    }

    void trigger(Milliseconds now) {
        if (on_timer.isExpired()) {
            info(now, "[ServoActivation." + String(config.name) + "] Triggered");
            on_timer.start(config.delay, AsyncDelay::units_t::MILLIS);
            off_timer.start(config.delay + config.on_duration, AsyncDelay::units_t::MILLIS);
        }
    }

    void tick(const Milliseconds now) {
        if (off_timer.isExpired()) {
            const auto new_state = State{
                    .angle = config.angleWhenOff,
                    .pwm = angele_to_pwm(config.angleWhenOff),
            };
            if (state.angle != new_state.angle) {
                info(now, "[ServoActivation." + String(config.name) + "] Off | " + String(state.angle) + "° -> " + String(new_state.angle) + "°");
                drive_outputs();
            }
            state = new_state;
        } else if (on_timer.isExpired()) {
            const auto new_state = State{
                    .angle = config.angleWhenOn,
                    .pwm = angele_to_pwm(config.angleWhenOn),
            };
            if (state.angle != new_state.angle) {
                info(now, "[ServoActivation." + String(config.name) + "] On | " + String(state.angle) + "° -> " + String(new_state.angle) + "°");
                drive_outputs();
            }
            state = new_state;
        }
    }

    void drive_outputs() {
        servo.write(state.angle);
    }

} ServoActivation;

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
        const auto now = millis();
        info(now, "[Sensor." + String(config.name) + "]");
        info(now, "  - Pin: " + String(config.pin));
        info(now, "  - Inverted: " + String(config.inverted));
        info(now, "  - Debounce: " + String(config.debounce) + "ms");
    }

    template<typename Func>
    void subscribe_high(Func &&func) {
        return debouncer.onPressedFor(config.debounce, func);
    }

    void tick(const Milliseconds now) {
        debouncer.read();
        if (debouncer.wasPressed()) {
            info(now, "[Sensor." + String(config.name) + "] Rising Edge");
        } else if (debouncer.wasReleased()) {
            info(now, "[Sensor." + String(config.name) + "] Falling Edge");
        }
    }

} Sensor;

typedef struct Inputs {
    Sensor pressure;


    void setup() {
        pressure.setup();
    }

    void dump() const {
        const auto now = millis();
        info(now, "[Inputs]");
        pressure.dump();
    }

    void tick(const Milliseconds now) {
        pressure.tick(now);
    }
} Inputs;

typedef struct Outputs {
    Activation dispenser;
    Activation solenoid;
    ServoActivation lighter;

    void setup() {
        dispenser.setup();
        solenoid.setup();
        lighter.setup();
    }

    void dump() const {
        const auto now = millis();
        info(now, "[Outputs]");
        dispenser.dump(now);
        solenoid.dump(now);
        lighter.dump(now);
    }

    void tick(Milliseconds now) {
        dispenser.tick(now);
        solenoid.tick(now);
        lighter.tick(now);
    }
} Outputs;

#endif //ROAR_IO_HPP

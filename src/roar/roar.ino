
#include <Arduino.h>

// External Libraries
// EasyButton @ 2.0.1
#include <EasyButton.h>
// AsyncDelay @ 1.1.2
#include <AsyncDelay.h>
// Servo @ 1.2.0
#include <Servo.h>
// Watchdog
#include<avr/wdt.h>


/*
 * Configuration
 * */

// Input Configuration
#define PRESSURE_SENSOR_PIN 2
#define PRESSURE_SENSOR_DEBOUNCE_MS 50
#define PRESSURE_SENSOR_PULL_UP true
#define PRESSURE_SENSOR_INVERTED true

// Output Configuration
#define DISPENSER_PIN 4
#define DISPENSER_DELAY_MS 1000
#define DISPENSER_DURATION_MS 1000
#define DISPENSER_INVERTED false

#define PIN_SOLENOID 5
#define SOLENOID_DELAY_MS 50
#define SOLENOID_DURATION_MS 700
#define SOLENOID_INVERTED false

#define LIGHTER_SERVO_PIN 3
#define LIGHTER_DELAY_MS 50
#define LIGHTER_ARM_DURATION_MS 300
#define LIGHTER_ON_DURATION_MS 1000
#define LIGHTER_ANGLE_WHEN_ARMED 90
#define LIGHTER_ANGLE_WHEN_ON 135
#define LIGHTER_ANGLE_WHEN_OFF 15

// Time between triggers
#define TIME_BETWEEN_TRIGGERS_MS 15000

// System Configuration
#define HEARTBEAT_INTERVAL_MS 5000
#define SERIAL_BAUD_RATE 115200


/*
 * Logging
 *
 * */
#define info(message) (Serial.println("[clk:" + String(millis()) + "] | " + String(message)))
// This prints: [clk:123456] | <your message>

typedef int Degrees;
typedef unsigned long Milliseconds;
typedef uint8_t Pin;




typedef struct Activation {
    typedef struct Config {
        Pin pin;
        Milliseconds delay;
        Milliseconds duration;
        Milliseconds time_between_triggers;
        bool inverted;
        String name;
    } Config;
    const Config config;

    AsyncDelay on_timer;
    AsyncDelay off_timer;
    AsyncDelay trigger_gate_timer;
    int state;

    explicit Activation(const Config &config) : config(config), state(config.inverted ? HIGH : LOW) {
        on_timer = AsyncDelay(config.delay, AsyncDelay::units_t::MILLIS);
        off_timer = AsyncDelay(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
        trigger_gate_timer = AsyncDelay(config.time_between_triggers, AsyncDelay::units_t::MILLIS);
    }

    void setup() {
        pinMode(config.pin, OUTPUT);
        digitalWrite(config.pin, state);
        on_timer.expire();
        off_timer.expire();
        trigger_gate_timer.expire();
    }

    void dump() const {
        info("[Output." + String(config.name) + "]");
        info("  - Pin: " + String(config.pin));
        info("  - Inverted: " + String(config.inverted));
        info("  - Delay: " + String(config.delay) + "ms");
        info("  - Duration: " + String(config.duration) + "ms");
        info("  - Time between triggers: " + String(config.time_between_triggers) + "ms");
    }

    void trigger() {
        if (on_timer.isExpired() && off_timer.isExpired() && trigger_gate_timer.isExpired()) {
            info("[Output." + String(config.name) + "] Triggered");
            on_timer.start(config.delay, AsyncDelay::units_t::MILLIS);
            off_timer.start(config.delay + config.duration, AsyncDelay::units_t::MILLIS);
            trigger_gate_timer.start(config.time_between_triggers, AsyncDelay::units_t::MILLIS);
        }
    }

    void tick() {
        if (off_timer.isExpired()) {
            const auto new_state = config.inverted ? LOW : HIGH;
            if (state != new_state) {
                info("[Output." + String(config.name) + "] Off | " + (config.inverted ? "LOW -> HIGH" : "HIGH -> LOW"));
            }
            state = new_state;
        } else if (on_timer.isExpired()) {
            const auto new_state = config.inverted ? HIGH : LOW;
            if (state != new_state) {
                info("[Output." + String(config.name) + "] On | " + (config.inverted ? "HIGH -> LOW" : "LOW -> HIGH"));
            }
            state = new_state;
        }
        digitalWrite(config.pin, state);
    }


} Activation;

typedef struct ServoActivation {
    // TODO: Maybe just support an array of angles and delays
    typedef struct Config {
        Pin pin;
        Milliseconds delay;
        Milliseconds arm_duration;
        Milliseconds on_duration;
        Milliseconds time_between_triggers;
        Degrees angleWhenArmed;
        Degrees angleWhenOn;
        Degrees angleWhenOff;
        String name;
    } Config;

    // Represents the current state of the servo
    typedef struct State {
        Degrees angle;
    } State;

    const Config config;
    State state;
    Servo servo;

    AsyncDelay on_timer;
    AsyncDelay arm_timer;
    AsyncDelay off_timer;
    AsyncDelay trigger_gate_timer;

    explicit ServoActivation(const Config &config) : config(config), state({config.angleWhenOff}) {
        arm_timer = AsyncDelay(config.delay, AsyncDelay::units_t::MILLIS);
        on_timer = AsyncDelay(config.delay + config.arm_duration, AsyncDelay::units_t::MILLIS);
        off_timer = AsyncDelay(config.delay + config.arm_duration + config.on_duration, AsyncDelay::units_t::MILLIS);
        trigger_gate_timer = AsyncDelay(config.time_between_triggers, AsyncDelay::units_t::MILLIS);
    }

    void update_state(Degrees angle) {
        state.angle = angle;
    }

    void setup() {
        servo.attach(config.pin);
        state = {
                .angle=config.angleWhenOff,
        };
        drive_outputs();
        arm_timer.expire();
        on_timer.expire();
        off_timer.expire();
        trigger_gate_timer.expire();
    }

    void dump() const {
        info("[Servo." + String(config.name) + "]");
        info("  - Pin: " + String(config.pin));
        info("  - Delay: " + String(config.delay) + "ms");
        info("  - Duration: " + String(config.on_duration) + "ms");
        info("  - Angle When Armed: " + String(config.angleWhenArmed) + "°");
        info("  - Angle When On: " + String(config.angleWhenOn) + "°");
        info("  - Angle When Off: " + String(config.angleWhenOff) + "°");
        info("  - Time between triggers: " + String(config.time_between_triggers) + "ms");
    }

    void trigger() {
        if (on_timer.isExpired() && off_timer.isExpired() && trigger_gate_timer.isExpired()) {
            info("[ServoActivation." + String(config.name) + "] Triggered");
            arm_timer.start(config.delay, AsyncDelay::units_t::MILLIS);
            on_timer.start(config.delay + config.arm_duration, AsyncDelay::units_t::MILLIS);
            off_timer.start(config.delay + config.arm_duration + config.on_duration, AsyncDelay::units_t::MILLIS);
            trigger_gate_timer.start(config.time_between_triggers, AsyncDelay::units_t::MILLIS);
        }
    }

    void tick() {
        if (off_timer.isExpired()) {
            const auto new_state = State{
                    .angle = config.angleWhenOff,
            };
            const auto should_drive = state.angle != new_state.angle;
            state = new_state;
            if (should_drive) {
                info("[ServoActivation." + String(config.name) + "] Off | " + String(state.angle) + "° -> " + String(new_state.angle) + "°");
                drive_outputs();
            }
        } else if (on_timer.isExpired()) {
            const auto new_state = State{
                    .angle = config.angleWhenOn,
            };
            const auto should_drive = state.angle != new_state.angle;
            state = new_state;
            if (should_drive) {
                info("[ServoActivation." + String(config.name) + "] On | " + String(state.angle) + "° -> " + String(new_state.angle) + "°");
                drive_outputs();
            }
        } else if (arm_timer.isExpired()) {
            const auto new_state = State{
                    .angle = config.angleWhenArmed,
            };
            const auto should_drive = state.angle != new_state.angle;
            state = new_state;
            if (should_drive) {
                info("[ServoActivation." + String(config.name) + "] Armed | " + String(state.angle) + "° -> " + String(new_state.angle) + "°");
                drive_outputs();
            }
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
        info("[Sensor." + String(config.name) + "]");
        info("  - Pin: " + String(config.pin));
        info("  - Inverted: " + String(config.inverted));
        info("  - Debounce: " + String(config.debounce) + "ms");
    }

    template<typename Func>
    void subscribe_high(Func &&func) {
        return debouncer.onPressedFor(config.debounce, func);
    }

    void tick() {
        debouncer.read();
        if (debouncer.wasPressed()) {
            info("[Sensor." + String(config.name) + "] Rising Edge");
        } else if (debouncer.wasReleased()) {
            info("[Sensor." + String(config.name) + "] Falling Edge");
        }
    }

} Sensor;

typedef struct Inputs {
    Sensor pressure;


    void setup() {
        pressure.setup();
    }

    void dump() const {
        info("[Inputs]");
        pressure.dump();
    }

    void tick() {
        pressure.tick();
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
        info("[Outputs]");
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

typedef struct Heartbeat {
    struct Config {
        Milliseconds interval;
    } config;
    AsyncDelay timer;

    void setup() {
        timer.expire();
    }

    void dump() {
        info("[Heartbeat]");
        info("  - Interval: " + String(config.interval) + "ms");
    }

    void tick() {
        if (timer.isExpired()) {
            info("[Heartbeat] 💜");
            timer.start(config.interval, AsyncDelay::units_t::MILLIS);
        }
    }

} Heartbeat;


// Global config object - this is the only place where you should be updating the config
typedef struct Config {
    Inputs inputs = {
            .pressure=Sensor(
                    Sensor::Config{
                            .pin = PRESSURE_SENSOR_PIN,
                            .debounce = PRESSURE_SENSOR_DEBOUNCE_MS,
                            .pull_up = PRESSURE_SENSOR_PULL_UP,
                            .inverted = PRESSURE_SENSOR_INVERTED,
                            .name = "Pressure"
                    }
            )
    };
    Outputs outputs = {
            .dispenser = Activation(
                    Activation::Config{
                            .pin = DISPENSER_PIN,
                            .delay = DISPENSER_DELAY_MS,
                            .duration = DISPENSER_DURATION_MS,
                            .time_between_triggers = TIME_BETWEEN_TRIGGERS_MS,
                            .inverted = DISPENSER_INVERTED,
                            .name = "Dispenser"
                    }
            ),
            .solenoid=Activation(
                    Activation::Config{
                            .pin = PIN_SOLENOID,
                            .delay = SOLENOID_DELAY_MS,
                            .duration = SOLENOID_DURATION_MS,
                            .time_between_triggers = TIME_BETWEEN_TRIGGERS_MS,
                            .inverted = SOLENOID_INVERTED,
                            .name = "Solenoid"
                    }
            ),

            .lighter=ServoActivation(
                    ServoActivation::Config{
                            .pin = LIGHTER_SERVO_PIN,
                            .delay = LIGHTER_DELAY_MS,
                            .arm_duration = LIGHTER_ARM_DURATION_MS,
                            .on_duration = LIGHTER_ON_DURATION_MS,
                            .time_between_triggers = TIME_BETWEEN_TRIGGERS_MS,
                            .angleWhenArmed = LIGHTER_ANGLE_WHEN_ARMED,
                            .angleWhenOn = LIGHTER_ANGLE_WHEN_ON,
                            .angleWhenOff = LIGHTER_ANGLE_WHEN_OFF,
                            .name = "Lighter"
                    }),
    };
    Heartbeat heartbeat = {
            .config = {
                    .interval = HEARTBEAT_INTERVAL_MS
            }
    };
} Config;


Config config;

void on_pressure_sensor_trigger() {
    config.outputs.dispenser.trigger();
    config.outputs.solenoid.trigger();
    config.outputs.lighter.trigger();
}


void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    info("[ROAR] Booting...");
    info("[ROAR.Watchdog][8s timeout]");
    wdt_enable(WDTO_8S);  /* Enable the watchdog with a timeout of 8 seconds */
    info("[ROAR] Starting...");
    info("[ROAR] Configuring...");
    config.inputs.setup();
    config.outputs.setup();
    config.heartbeat.setup();
    config.inputs.dump();
    config.outputs.dump();
    config.heartbeat.dump();
    config.inputs.pressure.subscribe_high(on_pressure_sensor_trigger);
    info("[ROAR] Configured");
    info("[ROAR] Started");
}

void loop() {
    config.inputs.tick();
    config.outputs.tick();
    config.heartbeat.tick();
    wdt_reset();
}

//
// Created by Soof Golan on 28/05/2023.
//

#include <Arduino.h>
#include <unity.h>
#include "config.h"


#define ASSERT_WITH_CLOCK(condition, message) do {char buffer[sizeof (message) + 40]; snprintf(buffer, sizeof buffer, "clk %lu | %s", millis(), message); TEST_ASSERT_MESSAGE(condition, buffer);} while (0)
#define ASSERT_EQ_WITH_CLOCK(expected, actual, message) do { char buffer[sizeof (message) + 40]; snprintf(buffer, sizeof buffer, "clk %lu | %s", millis(), message); TEST_ASSERT_EQUAL_MESSAGE(expected, actual, buffer);} while (0)

Sensor sensor = Sensor(
        Sensor::Config{
                .pin = 4,
                .debounce = 50,
                .pull_up = true,
                .inverted = true,
                .name = "Pressure"
        }
);

Activation activation = Activation(
        Activation::Config{
                .pin = 11,
                .delay = 200,
                .duration = 500,
                .inverted = false,
                .name = "Dispenser"
        }
);

Servo servo = Servo(
        Servo::Config{
                .pin = 13,
                .delay = 200,
                .duration = 500,
                .angleWhenOn = 30,
                .angleWhenOff = 150,
                .name = "Lighter"
        }
);



bool called = false;
void test_sensor_debounce() {
    sensor.setup();
    sensor.dump();
    sensor.subscribe_high([]() {
        called = true;
    });
    pinMode(sensor.config.pin, OUTPUT);  // Override pin mode for testing
    const auto ACTIVE = sensor.config.inverted ? LOW : HIGH;
    const auto INACTIVE = sensor.config.inverted ? HIGH : LOW;
    digitalWrite(sensor.config.pin, INACTIVE);
    TEST_ASSERT_EQUAL_MESSAGE(INACTIVE, digitalRead(sensor.config.pin), "Sanity check failed");
    delay(10);
    sensor.tick();
    digitalWrite(sensor.config.pin, ACTIVE);
    ASSERT_WITH_CLOCK(!called, "Should not be called yet");
    delay(10);
    sensor.tick();
    ASSERT_WITH_CLOCK(!called, "Should not be called yet");
    digitalWrite(sensor.config.pin, INACTIVE);
    ASSERT_WITH_CLOCK(!called, "Should not be called yet");
    delay(10);
    sensor.tick();
    ASSERT_WITH_CLOCK(!called, "Should not be called yet");
    digitalWrite(sensor.config.pin, ACTIVE);
    sensor.tick();
    delay(10);
    ASSERT_WITH_CLOCK(!called, "Should not be called yet");
    sensor.tick();
    delay(sensor.config.debounce - 10);
    sensor.tick();
    delay(20);
    sensor.tick();
    delay(20);
    sensor.tick();
    delay(20);
    sensor.tick();
    ASSERT_WITH_CLOCK(called, "Should have been called");

}


void test_activation() {
    activation.setup();
    activation.dump();
    delay(100);
    ASSERT_WITH_CLOCK(activation.on_timer.isExpired(), "On timer should start expired");
    ASSERT_WITH_CLOCK(activation.off_timer.isExpired(), "Off timer should start expired");
    ASSERT_WITH_CLOCK(!digitalRead(activation.config.pin), "Pin should be LOW");
    activation.trigger();
    activation.tick();
    ASSERT_WITH_CLOCK(!activation.on_timer.isExpired(), "On timer should start");
    ASSERT_WITH_CLOCK(!activation.off_timer.isExpired(), "Off timer should start");
    ASSERT_WITH_CLOCK(!digitalRead(activation.config.pin), "Pin should be LOW");
    delay(activation.config.delay - 50);
    activation.tick();
    ASSERT_WITH_CLOCK(!activation.on_timer.isExpired(), "On timer should not expire");
    ASSERT_WITH_CLOCK(!activation.off_timer.isExpired(), "Off timer should not expire");
    ASSERT_WITH_CLOCK(!digitalRead(activation.config.pin), "Pin should be LOW");
    delay(100);
    activation.tick();
    ASSERT_WITH_CLOCK(activation.on_timer.isExpired(), "On timer should expire");
    ASSERT_WITH_CLOCK(!activation.off_timer.isExpired(), "Off timer should not expire");
    ASSERT_WITH_CLOCK(digitalRead(activation.config.pin), "Pin should be HIGH");
    delay(activation.config.duration - 100);
    activation.tick();
    ASSERT_WITH_CLOCK(activation.on_timer.isExpired(), "On timer should stay expired");
    ASSERT_WITH_CLOCK(!activation.off_timer.isExpired(), "Off timer should not expire");
    ASSERT_WITH_CLOCK(digitalRead(activation.config.pin), "Pin should be HIGH");
    delay(100);
    activation.tick();
    ASSERT_WITH_CLOCK(activation.on_timer.isExpired(), "On timer should stay expired");
    ASSERT_WITH_CLOCK(activation.off_timer.isExpired(), "Off timer should expire");
    ASSERT_WITH_CLOCK(!digitalRead(activation.config.pin), "Pin should be LOW");

}

void test_servo_timing() {
    servo.setup();
    servo.dump();
    servo.tick();
    delay(100);
    servo.tick();
    ASSERT_WITH_CLOCK(servo.on_timer.isExpired(), "On timer should start expired");
    ASSERT_WITH_CLOCK(servo.off_timer.isExpired(), "Off timer should start expired");
    ASSERT_EQ_WITH_CLOCK(servo.state.angle, servo.config.angleWhenOff, "Angle should be at angleWhenOff");
    ASSERT_EQ_WITH_CLOCK(servo.state.pwm, Servo::angele_to_pwm(servo.config.angleWhenOff), "PWM should be at angleWhenOff");
    servo.trigger();
    servo.tick();
    ASSERT_WITH_CLOCK(!servo.on_timer.isExpired(), "On timer should start");
    ASSERT_WITH_CLOCK(!servo.off_timer.isExpired(), "Off timer should start");
    ASSERT_EQ_WITH_CLOCK(servo.state.angle, servo.config.angleWhenOff, "Angle should be at angleWhenOff");
    ASSERT_EQ_WITH_CLOCK(servo.state.pwm, Servo::angele_to_pwm(servo.config.angleWhenOff), "PWM should be at angleWhenOff");
    delay(servo.config.delay - 50);
    servo.tick();
    ASSERT_WITH_CLOCK(!servo.on_timer.isExpired(), "On timer should not expire");
    ASSERT_WITH_CLOCK(!servo.off_timer.isExpired(), "Off timer should not expire");
    ASSERT_EQ_WITH_CLOCK(servo.state.angle, servo.config.angleWhenOff, "Angle should be at angleWhenOff");
    ASSERT_EQ_WITH_CLOCK(servo.state.pwm, Servo::angele_to_pwm(servo.config.angleWhenOff), "PWM should be at angleWhenOff");
    delay(100);
    servo.tick();
    ASSERT_WITH_CLOCK(servo.on_timer.isExpired(), "On timer should expire");
    ASSERT_WITH_CLOCK(!servo.off_timer.isExpired(), "Off timer should not expire");
    ASSERT_EQ_WITH_CLOCK(servo.state.angle, servo.config.angleWhenOn, "Angle should be at angleWhenOn");
    ASSERT_EQ_WITH_CLOCK(servo.state.pwm, Servo::angele_to_pwm(servo.config.angleWhenOn), "PWM should be at angleWhenOn");
    delay(servo.config.on_duration - 100);
    servo.tick();
    ASSERT_WITH_CLOCK(servo.on_timer.isExpired(), "On timer should stay expired");
    ASSERT_WITH_CLOCK(!servo.off_timer.isExpired(), "Off timer should not expire");
    ASSERT_EQ_WITH_CLOCK(servo.state.angle, servo.config.angleWhenOn, "Angle should be at angleWhenOn");
    ASSERT_EQ_WITH_CLOCK(servo.state.pwm, Servo::angele_to_pwm(servo.config.angleWhenOn), "PWM should be at angleWhenOn");
    delay(100);
    servo.tick();
    ASSERT_WITH_CLOCK(servo.on_timer.isExpired(), "On timer should stay expired");
    ASSERT_WITH_CLOCK(servo.off_timer.isExpired(), "Off timer should expire");
    ASSERT_EQ_WITH_CLOCK(servo.state.angle, servo.config.angleWhenOff, "Angle should be at angleWhenOff");
    ASSERT_EQ_WITH_CLOCK(servo.state.pwm, Servo::angele_to_pwm(servo.config.angleWhenOff), "PWM should be at angleWhenOff");
}

void setup() {
    delay(2000);
    UNITY_BEGIN(); // IMPORTANT LINE!
    RUN_TEST(test_sensor_debounce);
    RUN_TEST(test_activation);
    RUN_TEST(test_servo_timing);
}

void loop() {
    // Do nothing
    UNITY_END(); // Stops unit testing
}
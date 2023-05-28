//
// Created by Soof Golan on 28/05/2023.
//

#ifndef ROAR_CONFIG_H
#define ROAR_CONFIG_H

#include "io.hpp"
#include <Arduino.h>

// Global config object - this is the only place where you should be updating the config
typedef struct Config {
    Inputs inputs = {
            .pressure=Sensor(
                    Sensor::Config{
                            .pin = 4,
                            .debounce = 50,
                            .pull_up = true,
                            .inverted = false,
                            .name = "Pressure"
                    }
            )
    };
    Outputs outputs = {
            .dispenser = Activation(
                    Activation::Config{
                            .pin = 11,
                            .delay = 1000,
                            .duration = 1000,
                            .inverted = false,
                            .name = "Dispenser"
                    }
            ),
            .solenoid=Activation(
                    Activation::Config{
                            .pin = 12,
                            .delay = 1000,
                            .duration = 1000,
                            .inverted = false,
                            .name = "Solenoid"
                    }
            ),

            .lighter=Servo(
                    Servo::Config{
                            .pin = 13,
                            .delay = 1000,
                            .duration = 1000,
                            .angleWhenOn = 0,
                            .angleWhenOff = 180,
                            .name = "Lighter"
                    }),
    };
} Config;

#endif //ROAR_CONFIG_H

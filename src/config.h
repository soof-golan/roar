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
                            .pin = 2,
                            .debounce = 50,
                            .pull_up = true,
                            .inverted = true,
                            .name = "Pressure"
                    }
            )
    };
    Outputs outputs = {
            .dispenser = Activation(
                    Activation::Config{
                            .pin = 5,
                            .delay = 1000,
                            .duration = 1000,
                            .inverted = false,
                            .name = "Dispenser"
                    }
            ),
            .solenoid=Activation(
                    Activation::Config{
                            .pin = 4,
                            .delay = 50,
                            .duration = 700,
                            .inverted = false,
                            .name = "Solenoid"
                    }
            ),

            .lighter=ServoActivation(
                    ServoActivation::Config{
                            .pin = 3,
                            .delay = 50,
                            .on_duration = 1000,
                            .angleWhenOn = 135,
                            .angleWhenOff = 0,
                            .name = "Lighter"
                    }),
    };
} Config;

#endif //ROAR_CONFIG_H

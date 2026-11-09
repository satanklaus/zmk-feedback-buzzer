/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>

/**
 * Play a melody on a zmk,buzzer device.
 *
 * Returns immediately; the melody runs on the system work queue. Starting a
 * melody while another one plays cuts the old one off.
 *
 * @param dev    The buzzer device.
 * @param melody Melody number (child node order in devicetree).
 * @return 0 on success, -EINVAL for an unknown melody, -ENODEV if the buzzer
 *         is not ready.
 */
int zmk_buzzer_play(const struct device *dev, uint8_t melody);

/** Number of melodies defined for a buzzer. */
uint8_t zmk_buzzer_melody_count(const struct device *dev);

/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_buzzer_test_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int test_pwm_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
                               uint32_t pulse_cycles, pwm_flags_t flags) {
    LOG_DBG("channel %d period %d us pulse %d us", channel, period_cycles, pulse_cycles);
    return 0;
}

static int test_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
                                       uint64_t *cycles) {
    *cycles = USEC_PER_SEC;
    return 0;
}

static DEVICE_API(pwm, test_pwm_api) = {
    .set_cycles = test_pwm_set_cycles,
    .get_cycles_per_sec = test_pwm_get_cycles_per_sec,
};

#define TEST_PWM_INST(n)                                                                           \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,        \
                          &test_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(TEST_PWM_INST)

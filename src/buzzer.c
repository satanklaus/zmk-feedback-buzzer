/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_buzzer

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <zmk_buzzer/buzzer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct buzzer_melody {
    // Frequency (Hz) and duration (ms) pairs.
    const uint32_t *notes;
    size_t len;
};

struct buzzer_config {
    struct pwm_dt_spec pwm;
    uint8_t duty_percent;
    const struct buzzer_melody *melodies;
    uint8_t melodies_len;
};

struct buzzer_data {
    const struct device *dev;
    struct k_work_delayable step_work;
    // Melody number + 1 asked for by zmk_buzzer_play(), 0 when none. The
    // work handler picks it up, so the PWM is only ever set from there.
    atomic_t requested;
    // Melody being played, NULL when idle.
    const struct buzzer_melody *melody;
    size_t note;
};

static int buzzer_silence(const struct buzzer_config *config) {
    return pwm_set_dt(&config->pwm, config->pwm.period, 0);
}

static void buzzer_step(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct buzzer_data *data = CONTAINER_OF(dwork, struct buzzer_data, step_work);
    const struct buzzer_config *config = data->dev->config;

    atomic_val_t requested = atomic_clear(&data->requested);
    if (requested) {
        // A new melody cuts off the one playing.
        data->melody = &config->melodies[requested - 1];
        data->note = 0;
        LOG_DBG("melody %ld start", requested - 1);
    } else if (data->melody) {
        data->note++;
    } else {
        return;
    }

    if (data->note >= data->melody->len / 2) {
        buzzer_silence(config);
        data->melody = NULL;
        LOG_DBG("melody done");
        return;
    }

    uint32_t freq_hz = data->melody->notes[2 * data->note];
    uint32_t duration_ms = data->melody->notes[2 * data->note + 1];

    int ret;
    if (freq_hz) {
        uint32_t period_ns = NSEC_PER_SEC / freq_hz;
        ret = pwm_set_dt(&config->pwm, period_ns, period_ns / 100 * config->duty_percent);
        LOG_DBG("note %d Hz for %d ms", freq_hz, duration_ms);
    } else {
        ret = buzzer_silence(config);
        LOG_DBG("rest for %d ms", duration_ms);
    }

    if (ret < 0) {
        LOG_ERR("Failed to set PWM: %d", ret);
        buzzer_silence(config);
        data->melody = NULL;
        return;
    }

    k_work_reschedule(&data->step_work, K_MSEC(duration_ms));
}

int zmk_buzzer_play(const struct device *dev, uint8_t melody) {
    const struct buzzer_config *config = dev->config;
    struct buzzer_data *data = dev->data;

    if (!device_is_ready(dev)) {
        return -ENODEV;
    }

    if (melody >= config->melodies_len) {
        LOG_ERR("Unknown buzzer melody %d (have %d)", melody, config->melodies_len);
        return -EINVAL;
    }

    atomic_set(&data->requested, melody + 1);
    k_work_reschedule(&data->step_work, K_NO_WAIT);

    return 0;
}

uint8_t zmk_buzzer_melody_count(const struct device *dev) {
    const struct buzzer_config *config = dev->config;

    return config->melodies_len;
}

static int buzzer_init(const struct device *dev) {
    const struct buzzer_config *config = dev->config;
    struct buzzer_data *data = dev->data;

    if (!pwm_is_ready_dt(&config->pwm)) {
        LOG_ERR("PWM device %s is not ready", config->pwm.dev->name);
        return -ENODEV;
    }

    int ret = buzzer_silence(config);
    if (ret < 0) {
        LOG_ERR("Failed to silence buzzer: %d", ret);
        return ret;
    }

    data->dev = dev;
    k_work_init_delayable(&data->step_work, buzzer_step);

    return 0;
}

#define MELODY_NOTES(node_id)                                                                      \
    BUILD_ASSERT(DT_PROP_LEN(node_id, notes) > 0 && DT_PROP_LEN(node_id, notes) % 2 == 0,          \
                 "buzzer melody needs frequency/duration pairs in notes");                         \
    static const uint32_t DT_CAT(node_id, _buzzer_notes)[] = DT_PROP(node_id, notes);

#define MELODY_ENTRY(node_id)                                                                      \
    {                                                                                              \
        .notes = DT_CAT(node_id, _buzzer_notes),                                                   \
        .len = DT_PROP_LEN(node_id, notes),                                                        \
    },

#define BUZZER_INST(n)                                                                             \
    BUILD_ASSERT(DT_INST_PROP(n, duty_percent) <= 100, "duty-percent must be 0-100");              \
    DT_INST_FOREACH_CHILD(n, MELODY_NOTES)                                                         \
    static const struct buzzer_melody buzzer_melodies_##n[] = {                                    \
        DT_INST_FOREACH_CHILD(n, MELODY_ENTRY)};                                                   \
    static struct buzzer_data buzzer_data_##n;                                                     \
    static const struct buzzer_config buzzer_config_##n = {                                        \
        .pwm = PWM_DT_SPEC_INST_GET(n),                                                            \
        .duty_percent = DT_INST_PROP(n, duty_percent),                                             \
        .melodies = buzzer_melodies_##n,                                                           \
        .melodies_len = ARRAY_SIZE(buzzer_melodies_##n),                                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, buzzer_init, NULL, &buzzer_data_##n, &buzzer_config_##n, POST_KERNEL, \
                          CONFIG_ZMK_BUZZER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(BUZZER_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

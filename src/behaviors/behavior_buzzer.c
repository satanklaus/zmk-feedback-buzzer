/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_buzzer

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk_buzzer/buzzer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_buzzer_config {
    const struct device *buzzer;
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    const struct behavior_parameter_metadata *metadata;
#endif
};

static int on_buzzer_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_buzzer_config *config = dev->config;

    int ret = zmk_buzzer_play(config->buzzer, binding->param1);
    if (ret < 0) {
        LOG_ERR("Failed to play buzzer melody %d: %d", binding->param1, ret);
        return ret;
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_buzzer_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    // A melody always plays to the end, however long the key is held.
    return ZMK_BEHAVIOR_OPAQUE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static int buzzer_get_parameter_metadata(const struct device *dev,
                                         struct behavior_parameter_metadata *param_metadata) {
    const struct behavior_buzzer_config *config = dev->config;

    *param_metadata = *config->metadata;
    return 0;
}
#endif

static const struct behavior_driver_api behavior_buzzer_driver_api = {
    .binding_pressed = on_buzzer_binding_pressed,
    .binding_released = on_buzzer_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = buzzer_get_parameter_metadata,
#endif
};

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

// ZMK Studio lists the buzzer's melodies by node name.
#define MELODY_VALUE(node_id)                                                                      \
    {                                                                                              \
        .display_name = DT_NODE_FULL_NAME(node_id),                                                \
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,                                               \
        .value = DT_NODE_CHILD_IDX(node_id),                                                       \
    },

#define BUZZER_METADATA(n)                                                                         \
    static const struct behavior_parameter_value_metadata behavior_buzzer_values_##n[] = {         \
        DT_FOREACH_CHILD(DT_INST_PHANDLE(n, buzzer), MELODY_VALUE)};                               \
    static const struct behavior_parameter_metadata_set behavior_buzzer_set_##n = {                \
        .param1_values = behavior_buzzer_values_##n,                                               \
        .param1_values_len = ARRAY_SIZE(behavior_buzzer_values_##n),                               \
    };                                                                                             \
    static const struct behavior_parameter_metadata behavior_buzzer_metadata_##n = {               \
        .sets_len = 1,                                                                             \
        .sets = &behavior_buzzer_set_##n,                                                          \
    };
#define BUZZER_METADATA_REF(n) .metadata = &behavior_buzzer_metadata_##n,

#else

#define BUZZER_METADATA(n)
#define BUZZER_METADATA_REF(n)

#endif

#define BUZZER_INST(n)                                                                             \
    BUZZER_METADATA(n)                                                                             \
    static const struct behavior_buzzer_config behavior_buzzer_config_##n = {                      \
        .buzzer = DEVICE_DT_GET(DT_INST_PHANDLE(n, buzzer)), BUZZER_METADATA_REF(n)};              \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_buzzer_config_##n, POST_KERNEL,         \
                            CONFIG_ZMK_BEHAVIOR_BUZZER_INIT_PRIORITY,                              \
                            &behavior_buzzer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BUZZER_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

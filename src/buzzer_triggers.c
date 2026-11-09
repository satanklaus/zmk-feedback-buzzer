/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_buzzer_triggers

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk_buzzer/buzzer.h>

#if IS_ENABLED(CONFIG_ZMK_USB)
#include <zmk/events/usb_conn_state_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Only one zmk,buzzer-triggers allowed");

#define TRIGGERS DT_DRV_INST(0)

// A melody node: its parent is the buzzer, its child index the melody number.
// dev is NULL when the trigger has no sound.
struct buzzer_sound {
    const struct device *dev;
    uint8_t melody;
};

#define SOUND(node_id, prop)                                                                       \
    COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),                                                   \
                ({                                                                                 \
                    .dev = DEVICE_DT_GET(DT_PARENT(DT_PHANDLE(node_id, prop))),                    \
                    .melody = DT_NODE_CHILD_IDX(DT_PHANDLE(node_id, prop)),                        \
                }),                                                                                \
                ({0}))

struct layer_sounds {
    uint8_t layer;
    struct buzzer_sound on;
    struct buzzer_sound off;
};

#define LAYER_SOUNDS(node_id)                                                                      \
    {                                                                                              \
        .layer = DT_PROP(node_id, layer),                                                          \
        .on = SOUND(node_id, on),                                                                  \
        .off = SOUND(node_id, off),                                                                \
    },

static const struct layer_sounds layer_sounds[] = {
    DT_FOREACH_CHILD_STATUS_OKAY(TRIGGERS, LAYER_SOUNDS)};

static const struct buzzer_sound startup_sound = SOUND(TRIGGERS, startup);

static void play(const struct buzzer_sound *sound) {
    if (!sound->dev) {
        return;
    }

    int ret = zmk_buzzer_play(sound->dev, sound->melody);
    if (ret < 0) {
        LOG_ERR("Failed to play buzzer melody %d: %d", sound->melody, ret);
    }
}

static int on_layer_state_changed(const struct zmk_layer_state_changed *ev) {
    for (size_t i = 0; i < ARRAY_SIZE(layer_sounds); i++) {
        if (layer_sounds[i].layer == ev->layer) {
            play(ev->state ? &layer_sounds[i].on : &layer_sounds[i].off);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

#if IS_ENABLED(CONFIG_ZMK_USB)

static const struct buzzer_sound usb_connected_sound = SOUND(TRIGGERS, usb_connected);
static const struct buzzer_sound usb_disconnected_sound = SOUND(TRIGGERS, usb_disconnected);

static int on_usb_conn_state_changed(const struct zmk_usb_conn_state_changed *ev) {
    static bool was_connected;
    // The first connection after boot is covered by the startup sound.
    static bool skip_connect = DT_NODE_HAS_PROP(TRIGGERS, startup);

    // ZMK reports a suspended host as still connected, so a sleeping host
    // stays quiet.
    bool connected = ev->conn_state == ZMK_USB_CONN_HID;
    if (connected == was_connected) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    was_connected = connected;

    if (connected && skip_connect) {
        skip_connect = false;
    } else {
        play(connected ? &usb_connected_sound : &usb_disconnected_sound);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

#endif

static int buzzer_triggers_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *layer_ev = as_zmk_layer_state_changed(eh);
    if (layer_ev) {
        return on_layer_state_changed(layer_ev);
    }

#if IS_ENABLED(CONFIG_ZMK_USB)
    const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
    if (usb_ev) {
        return on_usb_conn_state_changed(usb_ev);
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(buzzer_triggers, buzzer_triggers_listener);
ZMK_SUBSCRIPTION(buzzer_triggers, zmk_layer_state_changed);
#if IS_ENABLED(CONFIG_ZMK_USB)
ZMK_SUBSCRIPTION(buzzer_triggers, zmk_usb_conn_state_changed);
#endif

static int buzzer_triggers_init(void) {
    play(&startup_sound);
    return 0;
}

SYS_INIT(buzzer_triggers_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

/* SPDX-License-Identifier: MIT */

#define DT_DRV_COMPAT linea40_behavior_profile_key

#include <errno.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>
#include <zmk/events/keycode_state_changed.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct profile_key_config {
    uint32_t mac_keycode;
    uint32_t windows_keycode;
    uint8_t windows_profile;
};

struct profile_key_data {
    uint32_t keycode;
    uint32_t position;
    bool pressed;
};

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }

    const struct profile_key_config *config = dev->config;
    struct profile_key_data *data = dev->data;
    if (data->pressed) {
        return -EBUSY;
    }

    // Read ZMK's restored profile directly: no separate OS state to save or synchronize.
    data->keycode = zmk_ble_active_profile_index() == config->windows_profile
                        ? config->windows_keycode
                        : config->mac_keycode;
    data->position = event.position;
    data->pressed = true;
    return raise_zmk_keycode_state_changed_from_encoded(data->keycode, true, event.timestamp);
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }

    struct profile_key_data *data = dev->data;
    if (!data->pressed || data->position != event.position) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    // A profile can change while a key is held. Release the key that was actually pressed.
    uint32_t keycode = data->keycode;
    data->pressed = false;
    return raise_zmk_keycode_state_changed_from_encoded(keycode, false, event.timestamp);
}

static const struct behavior_driver_api profile_key_driver_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define PROFILE_KEY_INST(n)                                                                        \
    BUILD_ASSERT(DT_INST_PROP(n, windows_profile) >= 0 &&                                          \
                     DT_INST_PROP(n, windows_profile) < ZMK_BLE_PROFILE_COUNT,                     \
                 "Windows profile must be an available Bluetooth profile");                        \
    static const struct profile_key_config config_##n = {                                          \
        .mac_keycode = DT_INST_PROP(n, mac_keycode),                                               \
        .windows_keycode = DT_INST_PROP(n, windows_keycode),                                       \
        .windows_profile = DT_INST_PROP(n, windows_profile),                                       \
    };                                                                                             \
    static struct profile_key_data data_##n;                                                       \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &data_##n, &config_##n, POST_KERNEL,                    \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &profile_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PROFILE_KEY_INST)

#endif

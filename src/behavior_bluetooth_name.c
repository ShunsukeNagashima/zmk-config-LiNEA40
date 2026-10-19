/* SPDX-License-Identifier: MIT */

#define DT_DRV_COMPAT linea40_behavior_bluetooth_name

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC), "Bluetooth names must be writable");
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_SETTINGS), "Bluetooth names must survive a restart");

struct bluetooth_name_config {
    char *name;
};

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }

    const struct bluetooth_name_config *config = dev->config;
    // ZMK refreshes advertising; Zephyr saves the name without clearing bonds or Studio settings.
    int err = zmk_ble_set_device_name(config->name);
    if (err) {
        LOG_ERR("Failed to change Bluetooth name to %s (%d)", config->name, err);
        return err;
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api bluetooth_name_driver_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define BLUETOOTH_NAME_INST(n)                                                                     \
    BUILD_ASSERT(sizeof(DT_INST_PROP(n, device_name)) > 1, "Bluetooth name must not be empty");    \
    BUILD_ASSERT(sizeof(DT_INST_PROP(n, device_name)) - 1 <= CONFIG_BT_DEVICE_NAME_MAX,            \
                 "Bluetooth name is too long");                                                    \
    static const struct bluetooth_name_config config_##n = {                                       \
        .name = DT_INST_PROP(n, device_name),                                                      \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &config_##n, POST_KERNEL,                         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bluetooth_name_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BLUETOOTH_NAME_INST)

#endif

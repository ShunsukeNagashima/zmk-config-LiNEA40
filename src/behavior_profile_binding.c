/* SPDX-License-Identifier: MIT */

#define DT_DRV_COMPAT linea40_behavior_profile_binding

#include <errno.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/ble.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct profile_binding_config {
    struct zmk_behavior_binding bindings[2];
    uint8_t windows_profile;
};

struct profile_binding_data {
    const struct zmk_behavior_binding *pressed_binding;
    uint32_t position;
};

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }
    const struct profile_binding_config *config = dev->config;
    struct profile_binding_data *data = dev->data;
    if (data->pressed_binding != NULL) {
        return -EBUSY;
    }

    int index = zmk_ble_active_profile_index() == config->windows_profile ? 1 : 0;
    data->pressed_binding = &config->bindings[index];
    data->position = event.position;
    int err = zmk_behavior_invoke_binding(data->pressed_binding, event, true);
    if (err < 0) {
        data->pressed_binding = NULL;
    }
    return err;
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }
    struct profile_binding_data *data = dev->data;
    if (data->pressed_binding == NULL || data->position != event.position) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    // Release the original action even if the selected Bluetooth profile changed.
    const struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
    data->pressed_binding = NULL;
    return zmk_behavior_invoke_binding(pressed_binding, event, false);
}

static const struct behavior_driver_api profile_binding_driver_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define PROFILE_BINDING_ENTRY(n, idx)                                                              \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(n, bindings, idx)),                  \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(n, bindings, idx, param1), (0),          \
                              (DT_INST_PHA_BY_IDX(n, bindings, idx, param1))),                     \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(n, bindings, idx, param2), (0),          \
                              (DT_INST_PHA_BY_IDX(n, bindings, idx, param2))),                     \
    }

#define PROFILE_BINDING_INST(n)                                                                    \
    BUILD_ASSERT(DT_INST_PROP(n, windows_profile) >= 0 &&                                          \
                     DT_INST_PROP(n, windows_profile) < ZMK_BLE_PROFILE_COUNT,                     \
                 "Windows profile must be an available Bluetooth profile");                        \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, bindings) == 2, "Provide Mac and Windows bindings");          \
    static const struct profile_binding_config config_##n = {                                      \
        .bindings = {PROFILE_BINDING_ENTRY(n, 0), PROFILE_BINDING_ENTRY(n, 1)},                    \
        .windows_profile = DT_INST_PROP(n, windows_profile),                                       \
    };                                                                                             \
    static struct profile_binding_data data_##n;                                                   \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &data_##n, &config_##n, POST_KERNEL,                    \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &profile_binding_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PROFILE_BINDING_INST)

#endif

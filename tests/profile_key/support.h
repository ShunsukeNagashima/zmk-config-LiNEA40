/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Minimal host substitutes. The test compiles the production driver unchanged.
#define IS_ENABLED(value) (value)
#define DT_HAS_COMPAT_STATUS_OKAY(compat) 1
#define BUILD_ASSERT(condition, message) _Static_assert(condition, message)
#define ZMK_BLE_PROFILE_COUNT 5
#define ZMK_BEHAVIOR_OPAQUE 0
#define CONFIG_KERNEL_INIT_PRIORITY_DEFAULT 50

struct device {
    const void *config;
    void *data;
};

struct zmk_behavior_binding {
    const char *behavior_dev;
    uint32_t param1;
    uint32_t param2;
};

struct zmk_behavior_binding_event {
    int layer;
    uint32_t position;
    int64_t timestamp;
    uint8_t source;
};

struct behavior_parameter_metadata;
struct behavior_driver_api {
    enum { BEHAVIOR_LOCALITY_CENTRAL } locality;
    int (*binding_pressed)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    int (*binding_released)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    int (*get_parameter_metadata)(const struct device *, struct behavior_parameter_metadata *);
#endif
};

const struct device *zmk_behavior_get_binding(const char *name);
int zmk_ble_active_profile_index(void);
int raise_zmk_keycode_state_changed_from_encoded(uint32_t code, bool pressed, int64_t timestamp);
int zmk_behavior_get_empty_param_metadata(const struct device *dev,
                                          struct behavior_parameter_metadata *metadata);

// Real HID usage values; modifier bits use ZMK's encoded-keycode format.
#define TEST_GUI 0x000700e3u
#define TEST_CTRL 0x000700e0u
#define TEST_MAC_INPUT 0x0807002cu
#define TEST_WINDOWS_INPUT 0x04070035u

#define TEST_PROP_0_mac_keycode TEST_GUI
#define TEST_PROP_0_windows_keycode TEST_CTRL
#define TEST_PROP_0_windows_profile 1
#define TEST_PROP_1_mac_keycode TEST_CTRL
#define TEST_PROP_1_windows_keycode TEST_GUI
#define TEST_PROP_1_windows_profile 1
#define TEST_PROP_2_mac_keycode TEST_MAC_INPUT
#define TEST_PROP_2_windows_keycode TEST_WINDOWS_INPUT
#define TEST_PROP_2_windows_profile 1
#define DT_INST_PROP(n, prop) TEST_PROP_##n##_##prop
#define DT_INST_FOREACH_STATUS_OKAY(fn) fn(0) fn(1) fn(2)
#define BEHAVIOR_DT_INST_DEFINE(n, init, pm, data, config, level, priority, api)                   \
    static const struct device device_##n = {config, data}

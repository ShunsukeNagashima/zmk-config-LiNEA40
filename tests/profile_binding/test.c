/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "support.h"
#include "../../src/behavior_profile_binding.c"

static int active_profile;
static int invoke_result;
static struct {
    struct zmk_behavior_binding binding;
    struct zmk_behavior_binding_event event;
    bool pressed;
} output[16];
static size_t output_count;

const struct device *zmk_behavior_get_binding(const char *name) {
    return strcmp(name, "middle") == 0 ? &device_0 : NULL;
}

int zmk_ble_active_profile_index(void) { return active_profile; }

int zmk_behavior_invoke_binding(const struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event, bool pressed) {
    assert(output_count < sizeof(output) / sizeof(output[0]));
    output[output_count].binding = *binding;
    output[output_count].event = event;
    output[output_count++].pressed = pressed;
    return invoke_result;
}

int zmk_behavior_get_empty_param_metadata(const struct device *dev,
                                          struct behavior_parameter_metadata *metadata) {
    (void)dev;
    (void)metadata;
    return 0;
}

int main(void) {
    struct zmk_behavior_binding middle = {.behavior_dev = "middle"};
    struct zmk_behavior_binding missing = {.behavior_dev = "missing"};
    struct zmk_behavior_binding_event event = {
        .position = UINT32_MAX, .timestamp = 100, .layer = 2, .source = 1};

    // Every profile routes both edges to only its intended mouse/keyboard behavior.
    for (int profile = 0; profile < 5; profile++) {
        active_profile = profile;
        output_count = 0;
        event.timestamp = 100;
        assert(profile_binding_driver_api.binding_pressed(&middle, event) == 0);
        // Switching profiles while held must not leave a mouse button or Win key stuck.
        active_profile = profile == 1 ? 0 : 1;
        event.timestamp = 200;
        assert(profile_binding_driver_api.binding_released(&middle, event) == 0);
        assert(output_count == 2);
        for (size_t i = 0; i < output_count; i++) {
            assert(strcmp(output[i].binding.behavior_dev, profile == 1 ? "kp" : "mkp") == 0);
            assert(output[i].binding.param1 ==
                   (profile == 1 ? TEST_TASK_VIEW : TEST_MIDDLE_BUTTON));
            assert(output[i].binding.param2 == 0);
            assert(output[i].event.position == UINT32_MAX);
            assert(output[i].event.layer == 2 && output[i].event.source == 1);
            assert(output[i].event.timestamp == (i == 0 ? 100 : 200));
            assert(output[i].pressed == (i == 0));
        }
    }

    // Duplicate presses and unmatched releases cannot overwrite the active action.
    output_count = 0;
    assert(on_binding_pressed(&middle, event) == 0);
    assert(on_binding_pressed(&middle, event) == -EBUSY);
    event.position = 6;
    assert(on_binding_released(&middle, event) == 0);
    assert(output_count == 1);
    event.position = UINT32_MAX;
    assert(on_binding_released(&middle, event) == 0);
    assert(on_binding_released(&middle, event) == 0);
    assert(output_count == 2);

    // A failed press must allow a subsequent attempt, without a spurious release.
    output_count = 0;
    invoke_result = -ENODEV;
    assert(on_binding_pressed(&middle, event) == -ENODEV);
    assert(on_binding_released(&middle, event) == 0);
    assert(output_count == 1);
    invoke_result = 0;
    assert(on_binding_pressed(&middle, event) == 0);
    assert(on_binding_released(&middle, event) == 0);
    assert(output_count == 3);

    assert(on_binding_pressed(&missing, event) == -ENODEV);
    assert(on_binding_released(&missing, event) == -ENODEV);
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    assert(profile_binding_driver_api.get_parameter_metadata ==
           zmk_behavior_get_empty_param_metadata);
#endif
    puts("Profile binding tests passed");
    return 0;
}

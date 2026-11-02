/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "support.h"
#include "../../src/behavior_profile_key.c"

static int active_profile;
static struct {
    uint32_t code;
    bool pressed;
    int64_t timestamp;
    uint8_t implicit_modifiers;
    uint8_t explicit_modifiers;
    uint8_t reported_modifiers;
} output[64];
static size_t output_count;
static int modifier_counts[8];

static uint8_t held_modifiers(void) {
    uint8_t mask = 0;
    for (int i = 0; i < 8; i++) {
        if (modifier_counts[i] > 0) {
            mask |= 1u << i;
        }
    }
    return mask;
}

const struct device *zmk_behavior_get_binding(const char *name) {
    if (strcmp(name, "command") == 0) {
        return &device_0;
    }
    if (strcmp(name, "control") == 0) {
        return &device_1;
    }
    if (strcmp(name, "input") == 0) {
        return &device_2;
    }
    return NULL;
}

int zmk_ble_active_profile_index(void) { return active_profile; }

// Same encoded-key decoding as ZMK v0.2.1's keycode_state_changed.h.
struct zmk_keycode_state_changed zmk_keycode_state_changed_from_encoded(uint32_t code, bool pressed,
                                                                        int64_t timestamp) {
    uint16_t page = (code >> 16) & 0xff;
    uint16_t id = code & 0xffff;
    bool modifier = page == 7 && id >= 0xe0 && id <= 0xe7;
    return (struct zmk_keycode_state_changed){
        .usage_page = page ? page : 7,
        .keycode = id,
        .implicit_modifiers = modifier ? 0 : code >> 24,
        .explicit_modifiers = modifier ? code >> 24 : 0,
        .state = pressed,
        .timestamp = timestamp,
    };
}

int raise_zmk_keycode_state_changed(struct zmk_keycode_state_changed event) {
    assert(output_count < sizeof(output) / sizeof(output[0]));
    // Model ZMK HID's explicit modifier reference counts to check ownership/release order.
    for (int i = 0; i < 8; i++) {
        int delta = event.state ? 1 : -1;
        if (event.usage_page == 7 && event.keycode == (uint32_t)(0xe0 + i)) {
            modifier_counts[i] += delta;
        }
        if (event.explicit_modifiers & (1u << i)) {
            modifier_counts[i] += delta;
        }
        assert(modifier_counts[i] >= 0);
    }
    output[output_count].code = ((uint32_t)event.implicit_modifiers << 24) |
                                ((uint32_t)event.usage_page << 16) | event.keycode;
    output[output_count].pressed = event.state;
    output[output_count].timestamp = event.timestamp;
    output[output_count].implicit_modifiers = event.implicit_modifiers;
    output[output_count].explicit_modifiers = event.explicit_modifiers;
    output[output_count].reported_modifiers =
        held_modifiers() | (event.state ? event.implicit_modifiers : 0);
    output_count++;
    return 0;
}

int raise_zmk_keycode_state_changed_from_encoded(uint32_t code, bool pressed, int64_t timestamp) {
    return raise_zmk_keycode_state_changed(
        zmk_keycode_state_changed_from_encoded(code, pressed, timestamp));
}

int zmk_behavior_get_empty_param_metadata(const struct device *dev,
                                          struct behavior_parameter_metadata *metadata) {
    (void)dev;
    (void)metadata;
    return 0;
}

static void expect_tap(struct zmk_behavior_binding *binding, uint32_t expected) {
    struct zmk_behavior_binding_event event = {.position = 30, .timestamp = 100};
    output_count = 0;
    assert(profile_key_driver_api.binding_pressed(binding, event) == 0);
    event.timestamp = 200;
    assert(profile_key_driver_api.binding_released(binding, event) == 0);
    assert(output_count == 2);
    assert(output[0].code == expected && output[0].pressed && output[0].timestamp == 100);
    assert(output[1].code == expected && !output[1].pressed && output[1].timestamp == 200);
    // The ordinary Mac/Windows key path must not acquire explicit shortcut modifiers.
    assert(output[0].explicit_modifiers == 0 && output[1].explicit_modifiers == 0);
}

static void expect_windows_input(struct zmk_behavior_binding *binding, int next_profile) {
    struct zmk_behavior_binding_event event = {.position = UINT32_MAX, .timestamp = 100};
    output_count = 0;
    uint8_t existing_modifiers = held_modifiers();
    active_profile = 1;
    assert(on_binding_pressed(binding, event) == 0);
    assert(on_binding_pressed(binding, event) == -EBUSY);
    active_profile = next_profile;
    event.timestamp = 200;
    assert(on_binding_released(binding, event) == 0);
    assert(on_binding_released(binding, event) == 0);
    assert(output_count == 3);
    // One combined Alt+Grave press; then Grave up with Alt held, followed by Alt up.
    assert(output[0].code == TEST_GRAVE && output[0].pressed);
    assert(output[0].explicit_modifiers == 4 && output[0].implicit_modifiers == 0);
    assert(output[0].timestamp == 100);
    assert(output[1].code == TEST_GRAVE && !output[1].pressed);
    assert(output[1].explicit_modifiers == 0 && output[1].implicit_modifiers == 0);
    assert(output[2].code == TEST_ALT && !output[2].pressed);
    assert(output[2].explicit_modifiers == 0 && output[2].implicit_modifiers == 0);
    assert(output[1].timestamp == 200 && output[2].timestamp == 200);
    assert(output[0].reported_modifiers == (existing_modifiers | 4));
    assert(output[1].reported_modifiers == (existing_modifiers | 4));
    assert(output[2].reported_modifiers == existing_modifiers);
}

int main(void) {
    struct zmk_behavior_binding command = {.behavior_dev = "command"};
    struct zmk_behavior_binding control = {.behavior_dev = "control"};
    struct zmk_behavior_binding input = {.behavior_dev = "input"};
    struct zmk_behavior_binding missing = {.behavior_dev = "missing"};
    struct zmk_behavior_binding_event event = {.position = 30, .timestamp = 300};

    // Boot directly into the restored Windows profile without a profile-change event.
    active_profile = 1;
    expect_tap(&command, TEST_CTRL);
    expect_tap(&control, TEST_GUI);
    expect_windows_input(&input, 1);

    for (active_profile = 0; active_profile < 5; active_profile++) {
        if (active_profile == 1) {
            continue;
        }
        expect_tap(&command, TEST_GUI);
        expect_tap(&control, TEST_CTRL);
        expect_tap(&input, TEST_MAC_INPUT);
    }

    // A held modifier keeps its original release code when switching in either direction.
    for (int from = 0; from <= 1; from++) {
        active_profile = from;
        output_count = 0;
        assert(on_binding_pressed(&command, event) == 0);
        active_profile = 1 - from;
        assert(on_binding_released(&command, event) == 0);
        assert(output_count == 2);
        assert(output[0].code == output[1].code);
        assert(output[0].pressed && !output[1].pressed);
        expect_tap(&command, active_profile == 1 ? TEST_CTRL : TEST_GUI);
    }

    // Both modifiers can be held together; one instance never overwrites the other's state.
    active_profile = 1;
    output_count = 0;
    assert(on_binding_pressed(&command, event) == 0);
    assert(on_binding_pressed(&control, event) == 0);
    active_profile = 0;
    assert(on_binding_released(&control, event) == 0);
    assert(on_binding_released(&command, event) == 0);
    assert(output_count == 4);
    assert(output[0].code == TEST_CTRL && output[3].code == TEST_CTRL);
    assert(output[1].code == TEST_GUI && output[2].code == TEST_GUI);

    // The Windows release sequence remains paired even after switching to any Mac profile.
    for (int profile = 0; profile < 5; profile++) {
        expect_windows_input(&input, profile);
    }

    // A Mac shortcut pressed before a switch to Windows must keep the original release.
    event.position = UINT32_MAX;
    active_profile = 0;
    output_count = 0;
    assert(on_binding_pressed(&input, event) == 0);
    active_profile = 1;
    assert(on_binding_released(&input, event) == 0);
    assert(output_count == 2 && output[0].code == TEST_MAC_INPUT);
    assert(output[1].code == TEST_MAC_INPUT && !output[1].pressed);
    assert(output[0].explicit_modifiers == 0 && output[1].explicit_modifiers == 0);
    assert(output[0].reported_modifiers == 8 && output[1].reported_modifiers == 0);

    // Releasing the shortcut must not release an independently held physical Alt key.
    assert(raise_zmk_keycode_state_changed_from_encoded(TEST_ALT, true, 300) == 0);
    expect_windows_input(&input, 1);
    assert(held_modifiers() == 4);
    assert(raise_zmk_keycode_state_changed_from_encoded(TEST_ALT, false, 400) == 0);
    assert(held_modifiers() == 0);

    // Duplicate use cannot replace a held key or release it from a different position.
    event.position = 30;
    output_count = 0;
    assert(on_binding_pressed(&command, event) == 0);
    assert(on_binding_pressed(&command, event) == -EBUSY);
    event.position = 31;
    assert(on_binding_released(&command, event) == 0);
    assert(output_count == 1);
    event.position = 30;
    assert(on_binding_released(&command, event) == 0);
    assert(on_binding_released(&command, event) == 0);
    assert(output_count == 2);

    assert(on_binding_pressed(&missing, event) == -ENODEV);
    assert(on_binding_released(&missing, event) == -ENODEV);
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    assert(profile_key_driver_api.get_parameter_metadata == zmk_behavior_get_empty_param_metadata);
#endif
    puts("Profile key tests passed");
    return 0;
}

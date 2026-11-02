/* SPDX-License-Identifier: MIT */
#pragma once
#include "../profile_key/support.h"

int zmk_behavior_invoke_binding(const struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event, bool pressed);

// Match the preset's &mkp MB3 and &kp LG(TAB), including absent param2 cells.
#define TEST_MIDDLE_BUTTON 4u
#define TEST_TASK_VIEW 0x0807002bu
#undef DT_INST_FOREACH_STATUS_OKAY
#define DT_INST_FOREACH_STATUS_OKAY(fn) fn(0)
#define DT_INST_PROP_LEN(n, prop) 2
#define DEVICE_DT_NAME(node) node
#define TEST_DEVICE_0 "mkp"
#define TEST_DEVICE_1 "kp"
#define DT_INST_PHANDLE_BY_IDX(n, prop, idx) TEST_DEVICE_##idx
#define TEST_HAS_param1 1
#define TEST_HAS_param2 0
#define DT_INST_PHA_HAS_CELL_AT_IDX(n, prop, idx, cell) TEST_HAS_##cell
#define TEST_CELL_0_param1 TEST_MIDDLE_BUTTON
#define TEST_CELL_1_param1 TEST_TASK_VIEW
#define DT_INST_PHA_BY_IDX(n, prop, idx, cell) TEST_CELL_##idx##_##cell
#define TEST_UNPAREN(...) __VA_ARGS__
#define TEST_COND_0(zero, nonzero) TEST_UNPAREN zero
#define TEST_COND_1(zero, nonzero) TEST_UNPAREN nonzero
#define TEST_SELECT(flag) TEST_COND_##flag
#define COND_CODE_0(flag, zero, nonzero) TEST_SELECT(flag)(zero, nonzero)

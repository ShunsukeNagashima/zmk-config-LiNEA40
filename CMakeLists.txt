if(CONFIG_ZMK_BLE AND (NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
  target_sources(app PRIVATE src/behavior_bluetooth_name.c)
endif()

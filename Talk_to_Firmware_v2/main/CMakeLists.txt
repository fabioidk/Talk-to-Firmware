idf_component_register(
    SRCS "talk_to_firmware.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_driver_gpio esp_hw_support esp_partition led_strip
)

set(state_image "${CMAKE_CURRENT_BINARY_DIR}/state.bin")
file(WRITE "${state_image}" "LOCKED")
esptool_py_flash_target_image(flash state 0x100000 "${state_image}" ALWAYS_PLAINTEXT)

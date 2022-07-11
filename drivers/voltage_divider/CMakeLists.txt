zephyr_include_directories(.)
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_VOLTAGE_DIVIDER voltage_divider.c)

add_library(app_wifi STATIC wifi.c)
target_include_directories(app_wifi PUBLIC include)
target_link_libraries(app_wifi PUBLIC zephyr_interface)
# kernel.h includes generated headers that must exist before compiling wifi.c.
add_dependencies(app_wifi zephyr_generated_headers)
target_link_libraries(app PRIVATE app_wifi)

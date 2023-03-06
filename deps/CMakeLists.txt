# gpio-cpp
add_subdirectory("gpio-cpp")

# Broadcom Host
add_library(broadcom_host INTERFACE)
find_file(BCM_HOST_PATH NAMES "bcm_host.h")
if (BCM_HOST_PATH)
    message("BCM host header: ${BCM_HOST_PATH}")
    target_link_libraries(broadcom_host INTERFACE bcm_host)
else()
    message("BCM host header: Not found! Setting compile def SIMULATE_PI_HARDWARE...")
    target_compile_definitions(broadcom_host INTERFACE "SIMULATE_PI_HARDWARE")
endif()

# User configuration section starts here

# Default output data pin for the LED strip
set(OUTPUT_DATA_PIN 2)

# Default output data pin for the LED strip (SPI leds only, not for sk6812/ws2812b)
set(OUTPUT_CLOCK_PIN 3)

# Use multi-segment, starting index of second led strip or OFF to disable
set(SECOND_SEGMENT_INDEX OFF)

# If multi-segment is used and it's reversed, set this option to ON to enable reversing
set(SECOND_SEGMENT_REVERSED OFF)

# User configuration section ends here
# Usually you don't need to change anything belowe this section

cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/sdk/pico)
set(FREERTOS_KERNEL_PATH ${CMAKE_SOURCE_DIR}/sdk/freertos)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(HyperSerialPico  C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

IF(CMAKE_COMPILER_IS_GNUCC)
    string(REGEX REPLACE "(\-O[011123456789])" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REGEX REPLACE "(\-O[011123456789])" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")    
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Og")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Og")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# generic HyperSerialPico settings
set(HyperSerialPicoCompanionLibs FreeRTOS-Kernel FreeRTOS-Kernel-Heap1 pico_stdlib pico_multicore hardware_pio hardware_dma hardware_spi)
set(HyperSerialPicoCompanionIncludes ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/sdk/config)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/generated)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/firmwares)
 
macro(HyperSerialPicoTarget HyperSerialPicoTargetName)
    add_executable(${HyperSerialPicoTargetName} ${CMAKE_SOURCE_DIR}/source/main.cpp)
    target_include_directories(${HyperSerialPicoTargetName} PRIVATE ${HyperSerialPicoCompanionIncludes})
    target_link_libraries(${HyperSerialPicoTargetName} ${HyperSerialPicoCompanionLibs})
    pico_add_extra_outputs(${HyperSerialPicoTargetName})
    pico_enable_stdio_usb(${HyperSerialPicoTargetName} 1)
    pico_enable_stdio_uart(${HyperSerialPicoTargetName} 0)
    pico_generate_pio_header(${HyperSerialPicoTargetName} ${CMAKE_SOURCE_DIR}/pio/neopixel.pio OUTPUT_DIR ${CMAKE_SOURCE_DIR}/generated)    
    add_custom_command(TARGET ${HyperSerialPicoTargetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${HyperSerialPicoTargetName}.uf2 ${CMAKE_SOURCE_DIR}/firmwares)
endmacro()

# targets for different LED strips
IF(NOT SECOND_SEGMENT_INDEX)
    HyperSerialPicoTarget("HyperSerialPico_Spi")
    target_compile_definitions(HyperSerialPico_Spi PRIVATE -DSPILED_APA102 -DDATA_PIN=${OUTPUT_DATA_PIN} -DCLOCK_PIN=${OUTPUT_CLOCK_PIN})
    HyperSerialPicoTarget("HyperSerialPico_sk6812Cold")
    target_compile_definitions(HyperSerialPico_sk6812Cold PRIVATE -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=${OUTPUT_DATA_PIN})
    HyperSerialPicoTarget("HyperSerialPico_sk6812Neutral")
    target_compile_definitions(HyperSerialPico_sk6812Neutral PRIVATE -DNEOPIXEL_RGBW -DDATA_PIN=${OUTPUT_DATA_PIN})
    HyperSerialPicoTarget("HyperSerialPico_ws2812")
    target_compile_definitions(HyperSerialPico_ws2812 PRIVATE -DNEOPIXEL_RGB -DDATA_PIN=${OUTPUT_DATA_PIN})
ELSE()
    IF(NOT SECOND_SEGMENT_REVERSED)
        HyperSerialPicoTarget("HyperSerialPico_sk6812Cold_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("HyperSerialPico_sk6812Cold_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX})
        HyperSerialPicoTarget("HyperSerialPico_sk6812Neutral_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("HyperSerialPico_sk6812Neutral_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX})
        HyperSerialPicoTarget("HyperSerialPico_ws2812_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("HyperSerialPico_ws2812_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGB -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX})
    ELSE()
        HyperSerialPicoTarget("HyperSerialPico_sk6812Cold_rev_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("HyperSerialPico_sk6812Cold_rev_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX} -DSECOND_SEGMENT_REVERSED)
        HyperSerialPicoTarget("HyperSerialPico_sk6812Neutral_rev_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("HyperSerialPico_sk6812Neutral_rev_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX} -DSECOND_SEGMENT_REVERSED)
        HyperSerialPicoTarget("HyperSerialPico_ws2812_rev_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("HyperSerialPico_ws2812_rev_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGB -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX} -DSECOND_SEGMENT_REVERSED)
    ENDIF()        
ENDIF()

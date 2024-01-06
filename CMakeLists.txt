if (NOT PICO_PROGRAM_MAIN_ENTRY)
	# User configuration for HyperSerialPico section starts here

	# Some boards, such as the first Adafruit revisions, may have trouble booting properly
	# due to bad componets used in the design.
	# Turn this setting to ON if your rp2040 is not detected after firmware upload and reset
	set(BOOT_WORKAROUND OFF)

	# Default output data pin for the non-SPI LED strips (only for sk6812/ws2812b)
	set(OUTPUT_DATA_PIN 2)

	# Default output data/clocks pin for the SPI LED strips (only for apa102, not for sk6812/ws2812b)
	# only certain pairs of pins are allowed for selected SPI interface (refer to "readme.md")
	set(OUTPUT_SPI_DATA_PIN 3)
	set(OUTPUT_SPI_CLOCK_PIN 2)
	set(OUTPUT_SPI_INTERFACE spi0)

	# Use multi-segment, starting index of second led strip or OFF to disable
	set(SECOND_SEGMENT_INDEX OFF)

	# If multi-segment is used and it's reversed, set this option to ON to enable reversing
	set(SECOND_SEGMENT_REVERSED OFF)

	# User configuration section ends here
	# Usually you don't need to change anything below this section
endif()

cmake_minimum_required(VERSION 3.13)

# Disable Pico 'Reset device'
add_definitions ( -DPICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0 )

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sdk/pico)
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sdk/freertos)

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
set(HyperSerialPicoCompanionIncludes ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/sdk/config)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generated)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/firmware)

if(NOT CMAKE_HOST_WIN32)
	string(ASCII 27 EscChar)
	set(ColorReset  "${EscChar}[m")
	set(GreenColor  "${EscChar}[32m")
	set(YellowColor "${EscChar}[33m")
endif()

if (OVERRIDE_DATA_PIN)
	set(OUTPUT_DATA_PIN ${OVERRIDE_DATA_PIN})
	message( STATUS "${YellowColor}Overriding Neopixel Data GPIO: ${OUTPUT_DATA_PIN}${ColorReset}")
endif()

if (OVERRIDE_SPI_DATA_PIN)
	set(OUTPUT_SPI_DATA_PIN ${OVERRIDE_SPI_DATA_PIN})
	message( STATUS "${YellowColor}Overriding SPI Data GPIO: ${OUTPUT_SPI_DATA_PIN}${ColorReset}")
endif()

if (OVERRIDE_SPI_CLOCK_PIN)
	set(OUTPUT_SPI_CLOCK_PIN ${OVERRIDE_SPI_CLOCK_PIN})
	message( STATUS "${YellowColor}Overriding SPI Clock GPIO: ${OUTPUT_SPI_CLOCK_PIN}${ColorReset}")
endif()

if (OVERRIDE_SPI_INTERFACE)
	set(OUTPUT_SPI_INTERFACE ${OVERRIDE_SPI_INTERFACE})
	message( STATUS "${YellowColor}Overriding SPI Interface: ${OUTPUT_SPI_INTERFACE}${ColorReset}")
endif()

if (OVERRIDE_BOOT_WORKAROUND)
	set(BOOT_WORKAROUND ${OVERRIDE_BOOT_WORKAROUND})
	message( STATUS "${YellowColor}Overriding boot workaround: ${BOOT_WORKAROUND}${ColorReset}")
endif()

message( STATUS "---------------------------")
message( STATUS "Neopixel Data GPIO: ${GreenColor}${OUTPUT_DATA_PIN}${ColorReset}")
message( STATUS "SPI Data GPIO: ${GreenColor}${OUTPUT_SPI_DATA_PIN}${ColorReset}")
message( STATUS "SPI Clock GPIO: ${GreenColor}${OUTPUT_SPI_CLOCK_PIN}${ColorReset}")

if (OUTPUT_SPI_CHIP_SELECT)
	message( STATUS "SPI Select GPIO: ${GreenColor}${OUTPUT_SPI_CHIP_SELECT}${ColorReset}")
endif()

message( STATUS "SPI Interface: ${GreenColor}${OUTPUT_SPI_INTERFACE}${ColorReset}")
message( STATUS "Boot workaround: ${GreenColor}${BOOT_WORKAROUND}${ColorReset}")
message( STATUS "---------------------------")

add_compile_options(-ftrack-macro-expansion=0 -fno-diagnostics-show-caret -fdiagnostics-color=auto)

macro(HyperSerialPicoTarget HyperSerialPicoTargetName)
    if (PICO_PROGRAM_MAIN_ENTRY)
        add_executable(${HyperSerialPicoTargetName} ${PICO_PROGRAM_MAIN_ENTRY})
    else()
        add_executable(${HyperSerialPicoTargetName} ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp)
    endif()
    if (BOOT_WORKAROUND)
        target_compile_definitions(${HyperSerialPicoTargetName} PUBLIC -DBOOT_WORKAROUND -DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)
    endif()
    target_include_directories(${HyperSerialPicoTargetName} PRIVATE ${HyperSerialPicoCompanionIncludes})
    target_link_libraries(${HyperSerialPicoTargetName} ${HyperSerialPicoCompanionLibs})
    pico_add_extra_outputs(${HyperSerialPicoTargetName})
    pico_enable_stdio_usb(${HyperSerialPicoTargetName} 1)
    pico_enable_stdio_uart(${HyperSerialPicoTargetName} 0)
    pico_generate_pio_header(${HyperSerialPicoTargetName} ${CMAKE_CURRENT_SOURCE_DIR}/pio/neopixel.pio OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
    pico_generate_pio_header(${HyperSerialPicoTargetName} ${CMAKE_CURRENT_SOURCE_DIR}/pio/neopixel_ws2812b.pio OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
    add_custom_command(TARGET ${HyperSerialPicoTargetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${HyperSerialPicoTargetName}.uf2 ${CMAKE_CURRENT_SOURCE_DIR}/firmware)
endmacro()

# targets for different LED strips
IF(NOT SECOND_SEGMENT_INDEX)
    IF(NOT DISABLE_SPI_LEDS)
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_Spi")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_Spi" PRIVATE -DSPILED_APA102 -DSPI_INTERFACE=${OUTPUT_SPI_INTERFACE} -DDATA_PIN=${OUTPUT_SPI_DATA_PIN} -DCLOCK_PIN=${OUTPUT_SPI_CLOCK_PIN})
    endif()
    HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_sk6812Cold")
    target_compile_definitions("${CMAKE_PROJECT_NAME}_sk6812Cold" PRIVATE -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=${OUTPUT_DATA_PIN})
    HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_sk6812Neutral")
    target_compile_definitions("${CMAKE_PROJECT_NAME}_sk6812Neutral" PRIVATE -DNEOPIXEL_RGBW -DDATA_PIN=${OUTPUT_DATA_PIN})
    HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_ws2812")
    target_compile_definitions("${CMAKE_PROJECT_NAME}_ws2812" PRIVATE -DNEOPIXEL_RGB -DDATA_PIN=${OUTPUT_DATA_PIN})
ELSE()
    IF(NOT SECOND_SEGMENT_REVERSED)
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_sk6812Cold_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_sk6812Cold_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX})
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_sk6812Neutral_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_sk6812Neutral_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX})
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_ws2812_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_ws2812_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGB -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX})
    ELSE()
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_sk6812Cold_rev_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_sk6812Cold_rev_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DCOLD_WHITE -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX} -DSECOND_SEGMENT_REVERSED)
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_sk6812Neutral_rev_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_sk6812Neutral_rev_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGBW -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX} -DSECOND_SEGMENT_REVERSED)
        HyperSerialPicoTarget("${CMAKE_PROJECT_NAME}_ws2812_rev_multisegment_at_${SECOND_SEGMENT_INDEX}")
        target_compile_definitions("${CMAKE_PROJECT_NAME}_ws2812_rev_multisegment_at_${SECOND_SEGMENT_INDEX}" PRIVATE -DNEOPIXEL_RGB -DDATA_PIN=${OUTPUT_DATA_PIN} -DSECOND_SEGMENT_START_INDEX=${SECOND_SEGMENT_INDEX} -DSECOND_SEGMENT_REVERSED)
    ENDIF()
ENDIF()

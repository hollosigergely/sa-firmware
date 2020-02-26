set(CMAKE_C_COMPILER_WORKS 1)
project(uwb2020 C ASM)
cmake_minimum_required(VERSION 2.8)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# Please create a 'CMakePaths.cmake' file to specify the variables:
#   - NRF5_SDK_ROOT
#   - NRFJPROG_ROOT
#   - ARM_GCC_ROOT
include(CMakePaths.cmake)

SET(CMAKE_C_COMPILER   ${ARM_GCC_ROOT}/arm-none-eabi-gcc)
SET(CMAKE_ASM_COMPILER ${ARM_GCC_ROOT}/arm-none-eabi-gcc)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -DBOARD_PCA10040 \
    -DBSP_DEFINES_ONLY \
    -DCONFIG_GPIO_AS_PINRESET \
    -DFLOAT_ABI_HARD \
    -DNRF52 \
    -DNRF52832_XXAA \
    -DNRF52_PAN_74 \
    -mcpu=cortex-m4 \
    -mthumb -mabi=aapcs \
    -Wall \
    -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
    -ffunction-sections -fdata-sections -fno-strict-aliasing \
    -fno-builtin -fshort-enums \
    -std=c11 \
    -O3 -g3")

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -g3 -O3 \
    -mthumb -mabi=aapcs \
    -T${PROJECT_SOURCE_DIR}/ld.script/uart_gcc_nrf52.ld -L${NRF5_SDK_ROOT}/modules/nrfx/mdk \
    -mcpu=cortex-m4 \
    -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
    -Wl,--gc-sections")

add_definitions(-D__HEAP_SIZE=8192 -D__STACK_SIZE=8192)


set(SRCS
      ${PROJECT_SOURCE_DIR}/main.c
      ${PROJECT_SOURCE_DIR}/log.c
      ${PROJECT_SOURCE_DIR}/address_handler.c
      ${PROJECT_SOURCE_DIR}/dwm_utils.c
      ${PROJECT_SOURCE_DIR}/impl_observer.c
      ${PROJECT_SOURCE_DIR}/impl_anchor.c
      ${PROJECT_SOURCE_DIR}/impl_tag.c
      ${PROJECT_SOURCE_DIR}/decadriver/deca_device.c
      ${PROJECT_SOURCE_DIR}/decadriver/deca_params_init.c
      ${PROJECT_SOURCE_DIR}/utils.c
      ${PROJECT_SOURCE_DIR}/timing.c
      ${PROJECT_SOURCE_DIR}/messages.c

      ${NRF5_SDK_ROOT}/modules/nrfx/mdk/gcc_startup_nrf52.S
      ${NRF5_SDK_ROOT}/components/libraries/timer/app_timer.c
      ${NRF5_SDK_ROOT}/components/boards/boards.c
      ${NRF5_SDK_ROOT}/components/libraries/util/app_error.c
      ${NRF5_SDK_ROOT}/components/libraries/util/app_error_handler_gcc.c
      ${NRF5_SDK_ROOT}/components/libraries/util/app_error_weak.c
      ${NRF5_SDK_ROOT}/components/libraries/fifo/app_fifo.c
      ${NRF5_SDK_ROOT}/components/libraries/uart/app_uart_fifo.c
      ${NRF5_SDK_ROOT}/components/libraries/util/app_util_platform.c
      ${NRF5_SDK_ROOT}/components/libraries/util/nrf_assert.c
      ${NRF5_SDK_ROOT}/components/libraries/gpiote/app_gpiote.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_gpiote.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_twi.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_spi.c
      ${NRF5_SDK_ROOT}/components/libraries/strerror/nrf_strerror.c
      ${NRF5_SDK_ROOT}/components/libraries/uart/retarget.c
      ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_uart.c
      ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_spi.c
      ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_twi.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/prs/nrfx_prs.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_uart.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_uarte.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_timer.c
      ${NRF5_SDK_ROOT}/modules/nrfx/mdk/system_nrf52.c
      ${NRF5_SDK_ROOT}/external/segger_rtt/SEGGER_RTT.c
      ${NRF5_SDK_ROOT}/components/drivers_ext/lis2dh12/lis2dh12.c
      ${NRF5_SDK_ROOT}/components/libraries/twi_sensor/nrf_twi_sensor.c
      ${NRF5_SDK_ROOT}/components/libraries/twi_mngr/nrf_twi_mngr.c
      ${NRF5_SDK_ROOT}/components/libraries/queue/nrf_queue.c
      ${NRF5_SDK_ROOT}/components/libraries/balloc/nrf_balloc.c
      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_saadc.c
      ${NRF5_SDK_ROOT}/components/libraries/scheduler/app_scheduler.c
)

include_directories(
    ${NRF5_SDK_ROOT}/components
    ${NRF5_SDK_ROOT}/modules/nrfx/mdk
    ${NRF5_SDK_ROOT}/components/libraries/experimental_log
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/ld.script
    ${PROJECT_SOURCE_DIR}/decadriver
    ${NRF5_SDK_ROOT}/components/libraries/scheduler
    ${NRF5_SDK_ROOT}/components/libraries/fifo
    ${NRF5_SDK_ROOT}/components/libraries/strerror
    ${NRF5_SDK_ROOT}/components/toolchain/cmsis/include
    ${NRF5_SDK_ROOT}/components/libraries/util
    ${NRF5_SDK_ROOT}/components/libraries/balloc
    ${NRF5_SDK_ROOT}/modules/nrfx/hal
    ${NRF5_SDK_ROOT}/components/libraries/bsp
    ${NRF5_SDK_ROOT}/components/libraries/uart
    ${NRF5_SDK_ROOT}/components/libraries/gpiote
    ${NRF5_SDK_ROOT}/modules/nrfx
    ${NRF5_SDK_ROOT}/components/libraries/experimental_section_vars
    ${NRF5_SDK_ROOT}/integration/nrfx/legacy
    ${NRF5_SDK_ROOT}/components/libraries/experimental_log/src
    ${NRF5_SDK_ROOT}/components/libraries/delay
    ${NRF5_SDK_ROOT}/integration/nrfx
    ${NRF5_SDK_ROOT}/components/drivers_nrf/nrf_soc_nosd
    ${NRF5_SDK_ROOT}/components/boards
    ${NRF5_SDK_ROOT}/components/libraries/experimental_memobj
    ${NRF5_SDK_ROOT}/modules/nrfx/drivers/include
    ${NRF5_SDK_ROOT}/components/libraries/timer
    ${NRF5_SDK_ROOT}/external/segger_rtt
    ${NRF5_SDK_ROOT}/components/drivers_ext/lis2dh12
    ${NRF5_SDK_ROOT}/components/libraries/twi_sensor/
    ${NRF5_SDK_ROOT}/components/libraries/twi_mngr/
    ${NRF5_SDK_ROOT}/components/libraries/queue/
    ${NRF5_SDK_ROOT}/components/libraries/balloc/
    ${NRF5_SDK_ROOT}/components/libraries/pwr_mgmt/
)


add_executable(${PROJECT_NAME}  ${SRCS})
target_link_libraries(${PROJECT_NAME} c nosys m)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)

add_custom_target(flash
    COMMAND ${NRFJPROG_ROOT}/nrfjprog -f nrf52 --program ${PROJECT_NAME}.hex --sectorerase
    DEPENDS ${PROJECT_NAME})

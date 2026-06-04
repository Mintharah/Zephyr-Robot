cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(TestBench)

target_sources(app PRIVATE src/main.c
src/adc_thread.c
src/cmd_parser.c
src/dio_thread.c
src/pwm_thread.c
src/spi_thread.c
src/uart_passthru_thread.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
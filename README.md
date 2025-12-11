# RTOS Systems Comparison (Master Thesis)

Project collecting two reference implementations for STM32F439ZI used in the thesis comparison: a FreeRTOS firmware and a Zephyr-based temperature sensing demo with DS18B20.

## Repository Layout

- [freertos](freertos): STM32CubeMX/CubeIDE project targeting STM32F439ZI with FreeRTOS.
- [zephyr](zephyr): Zephyr samples, including DS18B20 over 1-Wire with UART6 bridge.
- [scripts](scripts): Helper scripts (e.g., zephyr build helpers).
- [logs](logs): Collected measurement or run logs.

## FreeRTOS Firmware (STM32F439ZI)

- **Kernel tick**: 1000 Hz default tick rate defined in [freertos/Core/Inc/FreeRTOSConfig.h](freertos/Core/Inc/FreeRTOSConfig.h).
- **Communication**: UART3 used for host communication (pins per Nucleo-F439ZI default mapping).
- **Timing resources**: 3 hardware timers reserved/available for optional timing calculations or profiling; core scheduling relies on the SysTick configured at 1 kHz.
- **Project files**: Entry point in [freertos/Core/Src/main.c](freertos/Core/Src/main.c) with FreeRTOS tasks configured in [freertos/Core/Src/freertos.c](freertos/Core/Src/freertos.c).
- **Build/flash**:
  - Open [freertos/FreeRTOS_project.ioc](freertos/FreeRTOS_project.ioc) in STM32CubeIDE (or regenerate code with STM32CubeMX if needed).
  - Build for the STM32F439ZI target; linker scripts provided for FLASH ([freertos/STM32F439ZITX_FLASH.ld](freertos/STM32F439ZITX_FLASH.ld)) and RAM ([freertos/STM32F439ZITX_RAM.ld](freertos/STM32F439ZITX_RAM.ld)).
  - Flash via ST-LINK from CubeIDE or `st-flash write` using the generated binary in `freertos/Debug/`.

## Zephyr DS18B20 Demo (STM32F439ZI)

- **Board**: Nucleo-F439ZI (Zephyr board target `nucleo_f439zi`).
- **Sensor**: DS18B20 temperature sensor over 1-Wire using the provided sample in [zephyr/ds18b20](zephyr/ds18b20).
- **UART wiring (critical)**: Use USART6 with TX -> PG14 and RX -> PG9 for the UART-to-1-Wire bridge; confirm jumpers match these pins.
- **Key sources/config**:
  - Application logic: [zephyr/ds18b20/src/main.c](zephyr/ds18b20/src/main.c)
  - Board overlay: [zephyr/ds18b20/nucleo_f439zi.overlay](zephyr/ds18b20/nucleo_f439zi.overlay)
  - Zephyr configuration: [zephyr/ds18b20/prj.conf](zephyr/ds18b20/prj.conf)
- **Build/flash (Zephyr toolchain)**:
  ```bash
  west build -b nucleo_f439zi zephyr/ds18b20 -t run
  # or flash after build
  west flash
  ```
  Ensure Zephyr SDK or Zephyr RTOS toolchain is installed and `west` is initialized in this workspace.

## Notes

- Both projects target the same MCU to enable timing and behavior comparison between FreeRTOS and Zephyr.
- FreeRTOS timing measurements can leverage the 3 hardware timers while keeping the 1 kHz scheduler tick intact.
- The Zephyr demo focuses on reliable UART6 pin assignment for the DS18B20 bridge; incorrect wiring will prevent sensor responses.

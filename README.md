# Zephyr-Robot

A Robot Framework-driven hardware test bench built on Zephyr RTOS for automated validation of embedded peripherals.

The project provides a UART-based command protocol that allows a host PC running Robot Framework to control and verify peripheral functionality on an STM32 Blackpill board.

## Features

- Automated hardware validation using Robot Framework
- UART command protocol over serial
- Multi-threaded Zephyr RTOS architecture
- Verification of:
  - Digital I/O (DIO)
  - ADC
  - PWM
  - UART
  - SPI
- Command and result queues for thread-safe communication
- Single-board self-loopback validation (no second board required)

---

## System Overview

The testbench consists of:

- Host PC running Robot Framework
- A single STM32 Blackpill running Zephyr RTOS

Every peripheral is verified by **self-loopback on the same board** — each
output is jumpered back to a corresponding input. No passive components and no
second board are required.

Communication is performed through UART at **115200 baud**.

```text
Robot Framework
       │
       ▼  USART1 (command channel)
+-----------------------------+
| STM32 Blackpill (Zephyr)    |
|                             |
|  PB3 ──┐ DIO     ┌── PB4    |
|  PB5 ──┘ pairs   └── PB6    |
|  PA7 MOSI ─────────  PA6 MISO
|  PA2 TX ───────────  PA3 RX |
|  PB0 PWM ──────────  PA8 cap|
|  PA1 GPIO ─────────  PA0 ADC|
+-----------------------------+
   (jumpers close every loop on-board)
```

---

## Architecture

### Thread Model

| Thread | Priority | Purpose |
|----------|----------|----------|
| cmd_rx | 3 | Receives UART commands |
| cmd_dispatch | 3 | Routes commands to peripherals |
| result_tx | 3 | Sends responses back to host |
| dio_thread | 4 | Handles GPIO operations |
| adc_thread | 4 | Handles ADC reads |
| pwm_thread | 4 | Handles PWM configuration |
| uart_passthru | 4 | UART loopback send/receive |
| spi_thread | 4 | SPI transfers |

### Queues

| Queue | Depth |
|---------|---------|
| cmd_q | 16 |
| result_q | 16 |

### Stack Size

All worker threads use:

```text
1024 bytes
```

---

## Supported Peripherals

### Digital I/O

- GPIOB PB3 → PB6
- Set and read GPIO levels
- Loopback validation

### ADC

- ADC1 IN0 (PA0)
- 12-bit resolution
- Range: 0–4095

### PWM

- TIM3 Channel 3
- Output on PB0
- Configurable period and pulse width

### UART

- USART1: Host communication
- USART2: Loopback UART (TX jumpered to RX)

### SPI

- SPI1 Master
- 1 MHz clock
- Full duplex transfers

---

## ASCII Command Protocol

All commands are newline terminated.

### DIO

#### Set Output

```text
CMD:DIO_SET:<pin>:<value>
```

Example:

```text
CMD:DIO_SET:0:1
```

Response:

```text
OK:DIO_SET
```

#### Read Input

```text
CMD:DIO_GET:<pin>
```

Example:

```text
CMD:DIO_GET:1
```

Response:

```text
OK:DIO_GET:1:1
```

---

### ADC

#### Read (ADC in)

```text
CMD:ADC_READ
```

Response (millivolts):

```text
OK:ADC_READ:2048
```

> **ADC out** uses a spare GPIO as a 1-bit "DAC": the board has no DAC and we
> use no passives, so PA1 (exposed as DIO4) is jumpered to the ADC pin PA0.
> Driving it high/low gives ~3300 mV / ~0 mV. The `Set ADC Source` keyword
> wraps this; pair it with `Read ADC Millivolts` to verify the loop.

---

### PWM

#### Set Output (PWM out)

```text
CMD:PWM_SET:<period_ns>:<pulse_ns>
```

Example:

```text
CMD:PWM_SET:1000000:500000
```

Response:

```text
OK:PWM_SET:1000000:500000
```

#### Capture Input (PWM in)

Measures an incoming PWM via timer input capture. Requires
`CONFIG_PWM_CAPTURE=y` and a jumper into the capture pin (PA8 / TIM1 CH1).

```text
CMD:PWM_GET
```

Response (`<period_ns>:<pulse_ns>`):

```text
OK:PWM_GET:1000000:500000
```

If no capture channel is wired, the firmware replies `ERR:PWM_GET:NO_CAPTURE`.

---

### UART

#### Send

```text
CMD:UART_SEND:HELLO
```

Response:

```text
OK:UART_SEND
```

#### Receive

```text
CMD:UART_RECV
```

Response:

```text
OK:UART_RECV:HELLO
```

---

### SPI

```text
CMD:SPI_XFER:DE AD BE EF
```

Response:

```text
OK:SPI_XFER:DEADBEEF
```

---

### Error Responses

Unknown command:

```text
ERR:UNKNOWN
```

General format:

```text
ERR:<reason>
```

---

## Pin Mapping

### Host UART

| Pin | Function |
|-------|----------|
| PA9 | USART1 TX |
| PA10 | USART1 RX |

### Loopback UART

| Pin | Function |
|-------|----------|
| PA2 | USART2 TX |
| PA3 | USART2 RX |

### SPI

| Pin | Function |
|-------|----------|
| PA5 | SPI1 SCK |
| PA6 | SPI1 MISO |
| PA7 | SPI1 MOSI |
| PA4 | SPI1 CS |

### ADC

| Pin | Function |
|-------|----------|
| PA0 | ADC1 IN0 |

### PWM

| Pin | Function |
|-------|----------|
| PB0 | TIM3 CH3 (PWM out) |
| PA8 | TIM1 CH1 (PWM in / capture) |

### ADC drive

| Pin | Function |
|-------|----------|
| PA1 | GPIO output (DIO4), jumpered to PA0 to drive the ADC |

### Digital I/O

| Pin | Channel |
|-------|----------|
| PB3 | DIO0 |
| PB4 | DIO1 |
| PB5 | DIO2 |
| PB6 | DIO3 |

---

## Loopback Wiring (single board)

All loops are closed with jumper wires between the board's own pins. With these
in place the full suite runs against one Blackpill — no DUT.

| Peripheral | Jumper / connection | Verifies |
|------------|--------------------|----------|
| UART | PA2 (USART2 TX) → PA3 (USART2 RX) | UART send/receive |
| DIO pair 0–1 | PB3 (DIO0) ↔ PB4 (DIO1) | drive out / read back |
| DIO pair 2–3 | PB5 (DIO2) ↔ PB6 (DIO3) | drive out / read back |
| SPI | PA7 (MOSI) ↔ PA6 (MISO) | master transfer echo |
| PWM in/out | PB0 (PWM out) → PA8 (capture) | generate + capture |
| ADC in/out | PA1 (GPIO/DIO4) → PA0 (ADC) | drive rail level + read back |

Six jumper wires, no passive components. Each is a single wire between two
header pins on the same board.

**ADC note:** with no DAC and no RC filter, the ADC "output" is a 1-bit GPIO
drive — PA1 high gives ~3300 mV at PA0, PA1 low gives ~0 mV. That covers the two
rail levels; intermediate voltages would need an RC filter or a DAC.

> Note: PA4 (SPI CS) is still driven by the master but goes nowhere in the
> MOSI↔MISO loopback — that's harmless.

---

## Robot Framework Test Suites

### UART

- Loopback returns the sent text
- Empty-response timeout handling

### SPI

- Single-byte transfer
- Multi-byte transfer
- Zero-byte transfer

### ADC

- ADC in: reading within valid range
- ADC in: stability across reads
- ADC out: drive PA1 high/low and read it back on PA0
- ADC out: clear high-vs-low separation

### PWM

- PWM out: 50% / 25% / full / off duty cycles
- PWM in: capture period and frequency
- PWM in: duty-cycle readback and sweep tracking

### DIO

- High/low verification
- Loopback verification
- Continuous toggling tests

---

## Build Configuration

### Required Zephyr Options

```conf
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_ADC_STM32=y
CONFIG_PWM_STM32=y
CONFIG_PWM_CAPTURE=y
CONFIG_SPI=y
CONFIG_TIMESLICING=y
CONFIG_HEAP_MEM_POOL_SIZE=4096
CONFIG_MAIN_STACK_SIZE=2048
```

---

## Robot Framework Environment

### Python Dependencies

```text
robotframework==7.4.2
pyserial==3.5
```

Install (into a local virtualenv, which is git-ignored):

```bash
python3 -m venv robot_env
robot_env/bin/pip install -r robot/requirements.txt
```

---

## Running Tests

```bash
robot --variable PORT:/dev/ttyUSB0 robot/
```

Example:

```bash
robot --variable PORT:/dev/ttyACM0 robot/
```

---

## Command Execution Flow

1. Host sends ASCII command over USART1.
2. `cmd_rx` parses command.
3. Command is placed in `cmd_q`.
4. `cmd_dispatch` routes command to peripheral thread.
5. Peripheral performs operation.
6. Result is placed in `result_q`.
7. `result_tx` transmits response.
8. Robot Framework validates the result.

---

## Hardware Requirements

- STM32 Blackpill (STM32F4)
- Zephyr RTOS
- USB-UART connection
- Jumper wires to close each loop (see Loopback Wiring)
- SPI loopback wiring
- GPIO loopback wiring
- 6 jumper wires (no passive components)
- (see Loopback Wiring for the full jumper list)

---

## Project Goals

The purpose of Zephyr-Robot is to provide a reusable, automated hardware validation framework for embedded systems by combining:

- Zephyr RTOS
- Robot Framework
- Serial command protocols
- Automated regression testing

This enables repeatable verification of peripheral functionality during development and CI testing.

---

## Author

Developed as a Zephyr RTOS hardware test automation platform using Robot Framework and STM32 Blackpill hardware.

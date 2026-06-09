import serial
import time


class TestbenchLib:
    ROBOT_LIBRARY_SCOPE = "SUITE"

    def __init__(self, port="/dev/ttyUSB0", baudrate=115200, timeout=2.0):
        self._port = port
        self._baudrate = baudrate
        self._timeout = timeout
        self._ser = None

    # ------------------------------------------------------------------ #
    #  Lifecycle                                                           #
    # ------------------------------------------------------------------ #

    def open_connection(self):
        """Open the serial port. Called automatically by each keyword if needed."""
        if self._ser is None or not self._ser.is_open:
            self._ser = serial.Serial(
                self._port, self._baudrate, timeout=self._timeout
            )
            time.sleep(0.1)  # let the port settle

    def close_connection(self):
        """Close the serial port."""
        if self._ser and self._ser.is_open:
            self._ser.close()

    # ------------------------------------------------------------------ #
    #  Internal helpers                                                    #
    # ------------------------------------------------------------------ #

    def _send(self, cmd_str):
        """Send a CMD string (newline appended) and return the response line."""
        self.open_connection()
        self._ser.reset_input_buffer()
        line = cmd_str.rstrip("\n") + "\n"
        self._ser.write(line.encode())
        response = self._ser.readline().decode(errors="replace").strip()
        if not response:
            raise RuntimeError(f"No response from testbench for command: {cmd_str!r}")
        return response

    @staticmethod
    def _check_ok(response, prefix):
        """Assert response starts with OK:<prefix> and return the payload tokens."""
        if response.startswith("ERR:"):
            raise AssertionError(f"Testbench returned error: {response}")
        expected = f"OK:{prefix}"
        if not response.startswith(expected):
            raise AssertionError(
                f"Unexpected response '{response}' (expected prefix '{expected}')"
            )
        rest = response[len(expected):]
        return rest.lstrip(":").split(":") if rest else []

    # ------------------------------------------------------------------ #
    #  DIO keywords                                                        #
    # ------------------------------------------------------------------ #

    def set_dio_pin(self, pin, value):
        """Set a DIO pin high (1) or low (0).

        Example:
            Set DIO Pin    0    1
        """
        response = self._send(f"CMD:DIO_SET:{int(pin)}:{int(value)}")
        self._check_ok(response, "DIO_SET")
        time.sleep(0.005)

    def get_dio_pin(self, pin):
        """Read a DIO pin. Returns an integer 0 or 1.

        Example:
            ${val}=    Get DIO Pin    0
        """
        response = self._send(f"CMD:DIO_GET:{int(pin)}")
        tokens = self._check_ok(response, "DIO_GET")
        return int(tokens[1])

    # ------------------------------------------------------------------ #
    #  ADC keywords                                                        #
    # ------------------------------------------------------------------ #

    _ADC_VREF_MV = 3300
    _ADC_RAW_MAX = (1 << 12) - 1  # 4095

    def read_adc_millivolts(self, samples=8):
        """Read ADC, averaged over `samples` readings to reduce PWM ripple.

        Example:
            ${mv}=    Read ADC Millivolts
        """
        values = []
        for _ in range(samples):
            response = self._send("CMD:ADC_READ")
            tokens = self._check_ok(response, "ADC_READ")
            values.append(int(tokens[0]))
        return round(sum(values) / len(values))

    # ------------------------------------------------------------------ #
    #  PWM keywords                                                        #
    # ------------------------------------------------------------------ #

    def set_pwm(self, period_ns, pulse_ns):
        """Set PWM period and pulse width in nanoseconds.

        Example:
            Set PWM    1000000    500000
        """
        response = self._send(f"CMD:PWM_SET:{int(period_ns)}:{int(pulse_ns)}")
        self._check_ok(response, "PWM_SET")

    # ------------------------------------------------------------------ #
    #  UART passthrough keywords                                           #
    # ------------------------------------------------------------------ #

    def send_uart_to_dut(self, text):
        """Send a string to the DUT via the UART passthrough.

        The testbench flushes its RX ring buffer before sending, so
        Receive UART From DUT will only see the reply to THIS command.

        Example:
            Send UART To DUT    HELLO
        """
        response = self._send(f"CMD:UART_SEND:{text}")
        self._check_ok(response, "UART_SEND")

    def receive_uart_from_dut(self):
        """Receive the DUT's reply via the UART passthrough.

        Waits up to 300 ms (handled in firmware). Returns empty string
        if nothing arrives.

        Example:
            ${resp}=    Receive UART From DUT
        """
        response = self._send("CMD:UART_RECV")
        tokens = self._check_ok(response, "UART_RECV")
        return tokens[0] if tokens else ""

    # ------------------------------------------------------------------ #
    #  SPI keywords                                                        #
    # ------------------------------------------------------------------ #

    def spi_transfer(self, hex_bytes):
        """Perform a SPI transfer. hex_bytes is a space-separated hex string.

        Returns the received bytes as an uppercase hex string (no spaces).

        Example:
            ${rx}=    SPI Transfer    DE AD BE EF
        """
        response = self._send(f"CMD:SPI_XFER:{hex_bytes}")
        tokens = self._check_ok(response, "SPI_XFER")
        return tokens[0] if tokens else ""
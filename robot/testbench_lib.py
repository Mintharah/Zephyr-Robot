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
            time.sleep(0.1)  # lets the port settle

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
        """Set (generate) PWM period and pulse width in nanoseconds. PWM OUT.

        Example:
            Set PWM    1000000    500000
        """
        response = self._send(f"CMD:PWM_SET:{int(period_ns)}:{int(pulse_ns)}")
        self._check_ok(response, "PWM_SET")

    def read_pwm(self):
        """Capture (measure) an incoming PWM signal. PWM IN.

        Returns (period_ns, pulse_ns) as integers. Requires a capture
        channel and a jumper into the capture pin (see app.overlay).

        Example:
            ${period}    ${pulse}=    Read PWM
        """
        response = self._send("CMD:PWM_GET")
        tokens = self._check_ok(response, "PWM_GET")
        period_ns = int(tokens[0])
        pulse_ns = int(tokens[1])
        return period_ns, pulse_ns

    def read_pwm_frequency_hz(self):
        """Capture an incoming PWM and return its frequency in Hz. PWM IN.

        Example:
            ${freq}=    Read PWM Frequency Hz
        """
        period_ns, _ = self.read_pwm()
        if period_ns <= 0:
            raise AssertionError("Captured PWM period was zero")
        return round(1e9 / period_ns)

    def read_pwm_duty_percent(self):
        """Capture an incoming PWM and return its duty cycle in percent. PWM IN.

        Example:
            ${duty}=    Read PWM Duty Percent
        """
        period_ns, pulse_ns = self.read_pwm()
        if period_ns <= 0:
            raise AssertionError("Captured PWM period was zero")
        return round(100.0 * pulse_ns / period_ns)

    _ADC_DRIVE_PIN = 4

    def set_adc_source(self, value):
        """Drive the ADC input pin high (1) or low (0). ADC OUT.

        With no DAC or RC filter, the analog "output" is a spare GPIO
        (PA1, exposed as DIO4) jumpered to the ADC pin (PA0). This gives
        the two rail levels (~3300 mV / ~0 mV) that Read ADC Millivolts
        can verify.

        Example:
            Set ADC Source    1
        """
        self.set_dio_pin(self._ADC_DRIVE_PIN, value)

    # ------------------------------------------------------------------ #
    #  UART passthrough keywords                                           #
    # ------------------------------------------------------------------ #

    def send_uart(self, text):
        """Send a string out the loopback UART (USART2 TX -> RX jumper).

        The firmware flushes its RX ring buffer before sending, so
        Receive UART will only see the bytes echoed back for THIS command.

        Example:
            Send UART    HELLO
        """
        response = self._send(f"CMD:UART_SEND:{text}")
        self._check_ok(response, "UART_SEND")

    def receive_uart(self):
        """Receive bytes looped back on the UART (USART2 RX).

        Waits up to 300 ms (handled in firmware). Returns empty string
        if nothing arrives.

        Example:
            ${resp}=    Receive UART
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
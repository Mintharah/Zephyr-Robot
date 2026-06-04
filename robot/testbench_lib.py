-9/8/8# robot/testbench_lib.py
import serial, time
from robot.api.deco import keyword

class TestbenchLib:
    ROBOT_LIBRARY_SCOPE = "SUITE"

    def __init__(self, port="/dev/ttyUSB0", baud=115200):
        self._ser = serial.Serial(port, baud, timeout=1)
        time.sleep(0.1)

    def _cmd(self, *parts):
        line = "CMD:" + ":".join(str(p) for p in parts) + "\n"
        self._ser.write(line.encode())
        resp = self._ser.readline().decode().strip()
        if not resp.startswith("OK"):
            raise AssertionError(f"Testbench error: {resp}")
        return resp.split(":")[2:]  # payload tokens

    @keyword("Set DIO Pin")
    def set_dio(self, pin: int, value: int):
        self._cmd("DIO_SET", pin, value)

    @keyword("Get DIO Pin")
    def get_dio(self, pin: int) -> int:
        return int(self._cmd("DIO_GET", pin)[0])

    @keyword("Read ADC Millivolts")
    def read_adc(self) -> int:
        return int(self._cmd("ADC_READ")[0])

    @keyword("Set PWM")
    def set_pwm(self, period_ns: int, pulse_ns: int):
        self._cmd("PWM_SET", period_ns, pulse_ns)

    @keyword("Send UART To DUT")
    def uart_send(self, data: str):
        self._cmd("UART_SEND", data)

    @keyword("Receive UART From DUT")
    def uart_recv(self) -> str:
        return self._cmd("UART_RECV")[0]

    @keyword("SPI Transfer")
    def spi_xfer(self, *bytes_hex: str) -> str:
        return self._cmd("SPI_XFER", " ".join(bytes_hex))[0]
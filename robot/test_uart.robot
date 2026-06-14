*** Settings ***
Documentation     UART self-loopback: bytes sent on USART2 TX return on USART2 RX
...               via a PA2 -> PA3 jumper.
Library           testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}    /dev/ttyUSB0

*** Test Cases ***
UART Loopback Returns Sent Text
    Send UART    HELLO
    ${resp}=    Receive UART
    Should Contain    ${resp}    HELLO

UART Empty Response Timeout
    ${resp}=    Receive UART
    Should Be Empty    ${resp}

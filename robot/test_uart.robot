*** Settings ***
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}    /dev/ttyUSB0

*** Test Cases ***
UART HELLO Returns HELLO
    Send UART To DUT    HELLO
    ${resp}=    Receive UART From DUT
    Should Contain    ${resp}    HELLO

UART Empty Response Timeout
    ${resp}=    Receive UART From DUT
    Should Be Empty    ${resp}
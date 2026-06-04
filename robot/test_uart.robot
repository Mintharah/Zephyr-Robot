*** Settings ***
Library    testbench_lib.py
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Test Cases ***
UART Echo Single Word
    Send UART To DUT    HELLO
    ${resp}=    Receive UART From DUT
    Should Contain    ${resp}    HELLO

UART Echo Number String
    Send UART To DUT    12345
    ${resp}=    Receive UART From DUT
    Should Contain    ${resp}    12345

UART Empty Response Timeout
    ${resp}=    Receive UART From DUT
    Should Be Empty    ${resp}
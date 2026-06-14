*** Settings ***
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}    /dev/ttyUSB0

*** Test Cases ***
SPI Single Byte Transfer
    ${rx}=    SPI Transfer    AA
    Length Should Be    ${rx}    2

SPI Multi Byte Transfer
    ${rx}=    SPI Transfer    DE AD BE EF
    Should Be Equal    ${rx}    DEADBEEF

SPI Zero Byte
    ${rx}=    SPI Transfer    00
    Should Be Equal    ${rx}    00

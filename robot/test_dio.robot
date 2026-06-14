*** Settings ***
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}    /dev/ttyUSB0

*** Test Cases ***
DIO Pair 0-1 Set High And Readback
    Set DIO Pin    0    1
    ${val}=    Get DIO Pin    1
    Should Be Equal As Integers    ${val}    1

DIO Pair 0-1 Set Low And Readback
    Set DIO Pin    0    0
    ${val}=    Get DIO Pin    1
    Should Be Equal As Integers    ${val}    0

DIO Pair 2-3 Set High And Readback
    Set DIO Pin    2    1
    ${val}=    Get DIO Pin    3
    Should Be Equal As Integers    ${val}    1

DIO Pair 2-3 Set Low And Readback
    Set DIO Pin    2    0
    ${val}=    Get DIO Pin    3
    Should Be Equal As Integers    ${val}    0

DIO Both Pairs Toggle
    FOR    ${out_pin}    IN    0    2
        ${in_pin}=    Evaluate    ${out_pin} + 1
        Set DIO Pin    ${out_pin}    1
        ${val}=    Get DIO Pin    ${in_pin}
        Should Be Equal As Integers    ${val}    1
        Set DIO Pin    ${out_pin}    0
        ${val}=    Get DIO Pin    ${in_pin}
        Should Be Equal As Integers    ${val}    0
    END

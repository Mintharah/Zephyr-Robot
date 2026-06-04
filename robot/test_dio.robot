*** Settings ***
Library    testbench_lib.py
Library    testbench_lib.TestbenchLib

*** Test Cases ***
DIO Set High And Readback
    Set DIO Pin    1    1
    ${val}=    Get DIO Pin    0
    Should Be Equal As Integers    ${val}    1

DIO Set Low And Readback
    Set DIO Pin    1    0
    ${val}=    Get DIO Pin    0
    Should Be Equal As Integers    ${val}    0

DIO All Pins Toggle
    FOR    ${pin}    IN RANGE     0    4
        Set DIO Pin    ${pin}    1
        ${val}=    Get DIO Pin    ${pin}
        Should Be Equal As Integers    ${val}    1
        Set DIO Pin    ${pin}    0
    END
*** Settings ***
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}    /dev/ttyUSB0

*** Test Cases ***
ADC Reading Within Supply Range
    ${mv}=    Read ADC Millivolts
    Should Be True    ${mv} >= 0 and ${mv} <= 3300

ADC Reading Is Nonzero
    ${mv}=    Read ADC Millivolts
    Should Be True    ${mv} > 0

ADC Multiple Readings Are Stable
    ${a}=    Read ADC Millivolts
    ${b}=    Read ADC Millivolts
    ${diff}=    Evaluate    abs(${a} - ${b})
    Should Be True    ${diff} < 200

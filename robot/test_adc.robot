*** Settings ***
Library    testbench_lib.py
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Test Cases ***
ADC Reading Within Supply Range
    ${mv}=    testbench_lib.TestbenchLib.Read ADC Millivolts
    Should Be True    ${mv} >= 0 and ${mv} <= 3300

ADC Reading Is Nonzero
    ${mv}=    testbench_lib.TestbenchLib.Read ADC Millivolts
    Should Be True    ${mv} > 0

ADC Multiple Readings Are Stable
    ${a}=    testbench_lib.TestbenchLib.Read ADC Millivolts
    ${b}=    testbench_lib.TestbenchLib.Read ADC Millivolts
    ${diff}=    Evaluate abs(${a} - ${b})
    Should Be True ${diff} < 100
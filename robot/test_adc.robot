*** Settings ***
Documentation     ADC verification with no passive components. The ADC pin (PA0)
...               is driven by a spare GPIO (PA1, exposed as DIO4) via a jumper,
...               giving the two rail levels for in/out checks.
Library           testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}      /dev/ttyUSB0
${VREF}      ${3300}

*** Test Cases ***
# ------------------------------ ADC IN ----------------------------- #
ADC In Reads High Rail
    Set ADC Source    1
    Sleep    20ms
    ${mv}=    Read ADC Millivolts
    Should Be True    ${mv} > 3000 and ${mv} <= ${VREF}

ADC In Reads Low Rail
    Set ADC Source    0
    Sleep    20ms
    ${mv}=    Read ADC Millivolts
    Should Be True    ${mv} < 300

ADC In Stable When Driven
    Set ADC Source    1
    Sleep    20ms
    ${a}=    Read ADC Millivolts
    ${b}=    Read ADC Millivolts
    Should Be True    abs(${a} - ${b}) < 200

# ----------------------------- ADC OUT ----------------------------- #
ADC Out High Then Low Round Trip
    Set ADC Source    1
    Sleep    20ms
    ${high}=    Read ADC Millivolts
    Set ADC Source    0
    Sleep    20ms
    ${low}=    Read ADC Millivolts
    Should Be True    ${high} - ${low} > 2500

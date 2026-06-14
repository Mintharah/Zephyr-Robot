*** Settings ***
Library           testbench_lib.TestbenchLib    port=${PORT}
Suite Teardown    Set PWM    ${PERIOD}    0

*** Variables ***
${PORT}      /dev/ttyUSB0
${VREF}      ${3300}
${PERIOD}    ${1000000}

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

# ----------------------------- DIO ----------------------------- #
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

# ----------------------------- PWM OUT ----------------------------- #
PWM Out 50 Percent Duty
    Set PWM    ${PERIOD}    500000

PWM Out 25 Percent Duty
    Set PWM    ${PERIOD}    250000

PWM Out Full Duty
    Set PWM    ${PERIOD}    ${PERIOD}

PWM Out Off
    Set PWM    ${PERIOD}    0

# ------------------------------ PWM IN ----------------------------- #
PWM In Captures Period
    Set PWM    ${PERIOD}    500000
    Sleep    20ms
    ${period}    ${pulse}=    Read PWM
    Should Be True    abs(${period} - ${PERIOD}) < ${PERIOD} * 0.1

PWM In Captures 50 Percent Duty
    Set PWM    ${PERIOD}    500000
    Sleep    20ms
    ${duty}=    Read PWM Duty Percent
    Should Be True    45 <= ${duty} <= 55

PWM In Tracks Duty Sweep
    FOR    ${pct}    IN    25    50    75
        ${pulse}=    Evaluate    int(${PERIOD} * ${pct} / 100)
        Set PWM    ${PERIOD}    ${pulse}
        Sleep    20ms
        ${duty}=    Read PWM Duty Percent
        Should Be True    abs(${duty} - ${pct}) <= 7
    END

# ----------------------------- SPI ----------------------------- #
SPI Single Byte Transfer
    ${rx}=    SPI Transfer    AA
    Length Should Be    ${rx}    2

SPI Multi Byte Transfer
    ${rx}=    SPI Transfer    DE AD BE EF
    Should Be Equal    ${rx}    DEADBEEF

SPI Zero Byte
    ${rx}=    SPI Transfer    00
    Should Be Equal    ${rx}    00

# ----------------------------- UART ----------------------------- #
UART Loopback Returns Sent Text
    Send UART    HELLO
    ${resp}=    Receive UART
    Should Contain    ${resp}    HELLO

UART Empty Response Timeout
    ${resp}=    Receive UART
    Should Be Empty    ${resp}
*** Settings ***
Documentation     PWM verification: output (generation) and input (capture).
...               PWM IN tests require a capture channel plus a jumper from the
...               PWM source into the capture pin (see app.overlay).
Library           testbench_lib.TestbenchLib    port=${PORT}
Suite Teardown    Set PWM    ${PERIOD}    0

*** Variables ***
${PORT}      /dev/ttyUSB0
${PERIOD}    ${1000000}

*** Test Cases ***
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

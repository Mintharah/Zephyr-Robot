*** Settings ***
Library    testbench_lib.TestbenchLib    port=${PORT}

*** Variables ***
${PORT}    /dev/ttyUSB0

*** Test Cases ***
PWM 50 Percent Duty Cycle
    Set PWM    1000000    500000
    Sleep    0.1s

PWM 25 Percent Duty Cycle
    Set PWM    1000000    250000
    Sleep    0.1s

PWM 100 Percent Duty Cycle
    Set PWM    1000000    1000000
    Sleep    0.1s

PWM Off
    Set PWM    1000000    0
    Sleep    0.1s

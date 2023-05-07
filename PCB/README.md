# Door Lock Shield

<p align="center"> 
<img src="https://github.com/FREILab/DoorLockShield/blob/master/PCB/Pictures/DoorLockShield.png">
</p>

# Pin Info Arduino

| Name | Info                                       |
| ---- | ------------------------------------------ |
| A0   | Schließzylinder Taster                    |
| A1   | Read Schalter Tür                          |
| A2   | Tür zu Schalter                            |
| A3   | Terrassentür                               |
| A4   | **Freier Pin**                             |
| A5   | **Freier Pin**                             |
| 0    | **Freier Pin (Rx)**                        |
| 1    | **Freier Pin (Tx)**                        |
| 2    | STEP (Schrittmotor)                        |
| 3    | DIR (Schrittmotor)                         |
| 4    | SS für SD Karte                            |
| 5    | RFID (rot 2x schwarz markiert gr. Abstand) |
| 6    | Used for library is on Jumper              |
| 7    | SN74HC595 SER (Serial Data Input)          |
| 8    | SN74HC595 SRCLK (Shift Register Clock Pin) |
| 9    | SN74HC595 RCLK (Latch Pin)                 |
| 10   | Ethernet                                   |
| 11   | RFID (rot 2x schwarz markiert kl. Abstand) |
| 12   | RFID (rot 1x schwarz)                      |
| 13   | RFID (grün 1x schwarz)                     |

# Pin Info SN74HC595

| Name | Info                  |
| ---- | --------------------- |
| QA   | RFID LED1 (grau 2 Striche)            |
| QB   | RFID LED2 (grau einfacher Strich)            |
| QC   | RFID LED3 (grau)      |
| QD   | Relais Summer         |
| QE   | Enable (Schrittmotor) |
| QF   | Summer                |
| QG   | Power LED             |
| QH   | **Freier Pin**        |

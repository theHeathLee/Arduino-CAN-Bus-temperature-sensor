# Arduino-CAN-Bus-temperature-sensor-
Arduino with a can transceiver and temperature probe. that is all.

## Parts:

Arduino Nano

- Max6675

- MCP2515

External Libraries:
- [MCP_can - Coryjfowler](https://github.com/coryjfowler/MCP_CAN_lib)
- [Max6675 - SiriUli(Adafruit Fork)](https://github.com/SirUli/MAX6675)


## CAN Matrix

| Value       | ID        |DLC   | Byte Pos.|
| ------------- |:-----:  |----: |       --:|
| Temp (C)      | 0x100 |   8    | 0        |

## Diagrams

![alt tag](https://github.com/theHeathLee/Arduino-CAN-Bus-temperature-sensor-/blob/master/Pictures/frSchematic.png?raw=true "Connected oldtimer banner")

ß
![alt tag](https://github.com/theHeathLee/Arduino-CAN-Bus-temperature-sensor-/blob/master/Pictures/frBreadboared.png?raw=true "Connected oldtimer banner")


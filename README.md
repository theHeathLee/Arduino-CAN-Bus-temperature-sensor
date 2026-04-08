# Arduino CAN Bus temperature sensor
Arduino Nano with a CAN transceiver, K-type thermocouple, oil pressure sensor, ignition coil RPM input, and wheel speed Hall effect sensor.

Temperature range: 0°C to +1024°C

For use with cylinder head temperature (CHT), exhaust gas temperature (EGT), water/coolant, etc.

Galvanic isolation so the sensor can be mounted to vehicle ground without any ground loop issues.

## Parts:

Arduino Nano

- Max6675 (thermocouple amplifier)

- MCP2515 (CAN controller, CS on pin 10)

- ADUM5401 (galvanic isolator)

- Oil pressure sensor (resistive, connected to A0)

- Hall effect sensor for wheel speed (connected to pin 2 / INT0)

- Optocoupler PC817 for ignition coil RPM input (connected to pin 8 / ICP1)

- x2 1μF Capacitors

- [3D Printed Enclosure](https://www.thingiverse.com/thing:4293410)

- [x2 JST JWPF Sockets](https://www.jst-mfg.com/product/detail_e.php?series=151)

- x4 10x5mm neodymium magnets

External Libraries:
- [MCP_can - Coryjfowler](https://github.com/coryjfowler/MCP_CAN_lib)
- [Max6675 - SiriUli(Adafruit Fork)](https://github.com/SirUli/MAX6675)


## CAN Bus

**Baud rate:** 250 kbps  
**Oscillator:** 8 MHz  
**DBC file:** `can_database.dbc`

### Message: `SensorData` — ID 0x100

| Signal | Bytes | Type | Unit | Range | Notes |
|---|---|---|---|---|---|
| Temperature | 0–1 | int16 LE | °C | -32768–32767 | K-type thermocouple via MAX6675 |
| EngineRPM | 2–3 | int16 LE | RPM | 0–32767 | Ignition coil, 4 pulses/rev. Requires PC817 optocoupler on pin 8 (ICP1) |
| WheelRPM | 4–5 | int16 LE | RPM | 0–32767 | Hall effect sensor, 6 pulses/rev, pin 2 (INT0) |
| OilPressure | 6 | uint8 | PSI | 0–72 | Resistive sensor on A0, mapped 0–184 Ω → 0–72 PSI |

LE = Little-endian (Intel byte order)

### Signal conditioning — ignition coil input

> ⚠️ The ignition coil primary swings from +12V to ~−300V on collapse. **Never connect directly to the Arduino.**
>
> Use a **PC817 optocoupler**: drive the LED side through a 470 Ω resistor from the +12V coil driver output. Connect the open-collector output to pin 8 with a 10 kΩ pull-up to +5V.
## Prototype

![alt tag](https://github.com/theHeathLee/Arduino-CAN-Bus-temperature-sensor-/blob/master/Pictures/tempCANbus.jpg?raw=true "Connected oldtimer banner")

![alt tag](https://github.com/theHeathLee/Arduino-CAN-Bus-temperature-sensor-/blob/master/Pictures/tempcandisplay.jpg?raw=true "Connected oldtimer banner")



## Diagrams

![alt tag](https://github.com/theHeathLee/Arduino-CAN-Bus-temperature-sensor-/blob/master/Pictures/frSchematicWithIsolatorFinal.png?raw=true "Connected oldtimer banner")



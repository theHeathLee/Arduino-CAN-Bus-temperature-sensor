#include <mcp_can.h>
#include <SPI.h>
#include "max6675.h"

// old temp was 28 at room temperature of 21 and 100 at boiling water
#define tempScaler 1  //.825
// oil pressure sensor using balance bridge with reference resistor of 320 ohms
#define oilPressureResistanceCal .598

// ── Ignition coil RPM via Timer1 Input Capture ─────────────────────────────
// ICP1 = Arduino pin 8.  Connect the conditioned coil signal here.
//
// WARNING: The ignition coil primary swings from +12V to large negative spikes
//          (~-300 V on collapse).  You MUST condition the signal before the MCU:
//            Optocoupler (e.g. PC817) driven through a 470 Ω resistor from the
//            +12V side of the coil driver; open-collector output to pin 8 with
//            a 10 kΩ pull-up to +5V.  This is the recommended safe method.
#define PULSES_PER_REV    4          // Coil fires 4× per crank revolution
#define TIMER1_PRESCALER  8UL        // /8 → 2 MHz tick @ 16 MHz CPU
#define RPM_TIMEOUT_TICKS 4000000UL  // ~2 s at 2 MHz; treat as 0 RPM if exceeded

// ── Wheel speed via INT0 (pin 2) ───────────────────────────────────────────
// Hall effect sensor output connected to pin 2 (INT0) with 10 kΩ pull-up to +5V.
// To convert to km/h: speed_kmh = wheelRPM × (wheel_circumference_m × 60 / 1000)
#define WHEEL_SENSOR_PIN      2        // INT0
#define WHEEL_PULSES_PER_REV  6        // Hall sensor pulses per wheel revolution
#define WHEEL_TIMEOUT_US      500000UL // 0.5 s silence → wheel stopped

int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
int oilSensorPin = A0;

int16_t temperatureCelcius = 0;
uint8_t oilPressure = 0;
uint8_t oilPressureraw = 0;

// ── Timer1 state (ISR-shared) ───────────────────────────────────────────────
volatile uint32_t t1Overflows    = 0;
volatile uint32_t lastCapture32  = 0;
volatile uint32_t capturedPeriod = 0;
volatile bool     newCapture     = false;

int16_t rpm = 0;

// ── Wheel speed state (ISR-shared) ─────────────────────────────────────────
volatile uint32_t wheelLastPulse = 0;
volatile uint32_t wheelPeriodUs  = 0;
volatile bool     newWheelPulse  = false;

int16_t wheelRPM = 0;

void wheelSpeedISR() {
  uint32_t now   = micros();
  wheelPeriodUs  = now - wheelLastPulse;
  wheelLastPulse = now;
  newWheelPulse  = true;
}

// Extends 16-bit Timer1 to a 32-bit free-running counter
ISR(TIMER1_OVF_vect) {
  t1Overflows++;
}

// Fires on each rising edge at ICP1 (pin 8); ICR1 holds the exact capture time
ISR(TIMER1_CAPT_vect) {
  uint16_t icr = ICR1;
  uint32_t ov  = t1Overflows;
  // If a pending overflow occurred before we read TCNT1 but after ICR1 latched a
  // value in the lower half of the counter range, account for it.
  if ((TIFR1 & _BV(TOV1)) && (icr < 0x8000)) {
    ov++;
  }
  uint32_t now32   = (ov << 16) | icr;
  capturedPeriod   = now32 - lastCapture32;
  lastCapture32    = now32;
  newCapture       = true;
}

MCP_CAN CAN0(10);  // Set CS to pin 10

typedef union {
  uint8_t  Data8[8];
  uint16_t Data16[4];
  int16_t  Data16s[4];
} CANMsg_t;

CANMsg_t TxMsg;

void setupTimer1InputCapture() {
  // Normal (non-PWM) mode, prescaler /8, rising-edge capture, noise canceller on
  TCCR1A = 0;
  TCCR1B = _BV(CS11)    // prescaler /8  →  2 MHz tick @ 16 MHz
         | _BV(ICES1)   // capture on rising edge
         | _BV(ICNC1);  // 4-cycle input noise canceller
  TCNT1  = 0;
  TIMSK1 = _BV(ICIE1)   // Input Capture interrupt
         | _BV(TOIE1);  // Overflow interrupt (32-bit extension)
}

void setup() {

  Serial.begin(9600);

  // Initialize MCP2515 running at 8MHz with a baudrate of 250kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");
  CAN0.setMode(MCP_NORMAL);  // Change to normal mode to allow messages to be transmitted

  setupTimer1InputCapture();

  // Configure wheel speed Hall effect sensor on INT0
  pinMode(WHEEL_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WHEEL_SENSOR_PIN), wheelSpeedISR, RISING);

  // wait for MAX chip to stabilize
  delay(500);
}



void loop() {
  // ── RPM from Timer1 Input Capture ────────────────────────────────────────
  if (newCapture) {
    noInterrupts();
    uint32_t period = capturedPeriod;
    newCapture = false;
    interrupts();
    if (period > 0) {
      rpm = (int16_t)((60UL * (F_CPU / TIMER1_PRESCALER)) /
                      ((uint32_t)PULSES_PER_REV * period));
    }
    Serial.print("RPM = ");
    Serial.println(rpm);
  }

  // ── Stall / engine-off detection ─────────────────────────────────────────
  noInterrupts();
  uint32_t ov32  = t1Overflows;
  uint16_t cnt16 = TCNT1;
  uint32_t last  = lastCapture32;
  interrupts();
  uint32_t now32   = (ov32 << 16) | cnt16;
  uint32_t silence = now32 - last;
  if (silence > RPM_TIMEOUT_TICKS) {
    rpm = 0;
  }

  // ── Wheel speed from Hall effect sensor (INT0) ────────────────────────────
  if (newWheelPulse) {
    noInterrupts();
    uint32_t wPeriod = wheelPeriodUs;
    newWheelPulse = false;
    interrupts();
    if (wPeriod > 0) {
      wheelRPM = (int16_t)(60000000UL / ((uint32_t)WHEEL_PULSES_PER_REV * wPeriod));
    }
    Serial.print("Wheel RPM = ");
    Serial.println(wheelRPM);
  }

  // ── Wheel stopped detection ───────────────────────────────────────────────
  noInterrupts();
  uint32_t lastWheel = wheelLastPulse;
  interrupts();
  if ((micros() - lastWheel) > WHEEL_TIMEOUT_US) {
    wheelRPM = 0;
  }

  temperatureCelcius = int16_t(thermocouple.readCelsius());
  oilPressureraw = analogRead(oilSensorPin) * oilPressureResistanceCal;
  //convert from resistance range to psi range
  oilPressure = map(oilPressureraw, 0, 184, 0, 72);
  Serial.print("C = ");
  Serial.print(temperatureCelcius);
  //Serial.print(thermocouple.readCelsius());
  Serial.print("   ");
  Serial.print("Oil Pressure: ");
  Serial.println(oilPressure);
  delay(100);

  // bytes 0-1 = temperature, 2-3 = engine RPM, 4-5 = wheel RPM, 6 = oil pressure
  TxMsg.Data16s[0] = temperatureCelcius;
  TxMsg.Data16s[1] = rpm;
  TxMsg.Data16s[2] = wheelRPM;
  TxMsg.Data8[6]   = oilPressure;
  TxMsg.Data8[7]   = 0xFF;
  byte data[8] = { TxMsg.Data8[0], TxMsg.Data8[1], TxMsg.Data8[2], TxMsg.Data8[3], TxMsg.Data8[4], TxMsg.Data8[5], TxMsg.Data8[6], TxMsg.Data8[7] };

  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, data);
  if (sndStat == CAN_OK) {
    //Serial.println("Message Sent Successfully!");
  } else {
    //Serial.println("Error Sending Message...");
  }
  delay(100);
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

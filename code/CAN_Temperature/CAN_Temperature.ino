#include <mcp_can.h>
#include <SPI.h>
#include "max6675.h"

// old temp was 28 at room temperature of 21 and 100 at boiling water
#define tempScaler 1//.825
#define oilPressureResistanceCal .598

int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
int oilSensorPin = A0;

int16_t temperatureCelcius = 0;
uint8_t oilPressure = 0;
uint8_t oilPressureraw = 0;


MCP_CAN CAN0(10);     // Set CS to pin 10

void setup()
{

  Serial.begin(9600);
  
  // Initialize MCP2515 running at 8MHz with a baudrate of 250kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");
  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  
  // wait for MAX chip to stabilize
  delay(500);

}



void loop()
{
  temperatureCelcius = int16_t(thermocouple.readCelsius());
  oilPressureraw = analogRead(oilSensorPin)*oilPressureResistanceCal;
  //convert from resistance range to psi range
  oilPressure = map(oilPressureraw, 0, 184, 0, 72);
  Serial.print("C = "); 
  Serial.print(temperatureCelcius);
  //Serial.print(thermocouple.readCelsius());
  Serial.print("   ");
  Serial.print("Oil Pressure: ");
  Serial.println(oilPressure);
  delay(100);

  byte data[8] = {temperatureCelcius & 0x00FF, oilPressure, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
 
  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, data);
  if(sndStat == CAN_OK){
    //Serial.println("Message Sent Successfully!");
  } else {
    //Serial.println("Error Sending Message...");
  }
  delay(100); 
  
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

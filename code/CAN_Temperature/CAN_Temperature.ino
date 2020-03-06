// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>

#include "max6675.h"

int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

uint8_t temperatureCelcius =0;
uint8_t tempArray[4];

MCP_CAN CAN0(10);     // Set CS to pin 10

void setup()
{

  Serial.begin(9600);
  

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);

}

byte data[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

void loop()
{
  temperatureCelcius =uint8_t(thermocouple.readCelsius());
  Serial.print("C = "); 
  Serial.println(temperatureCelcius);
  Serial.println(thermocouple.readCelsius());
  delay(500);


  byte data[8] = {temperatureCelcius, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, data);
  if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }
  delay(500);   // send data per 100ms
  


  
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

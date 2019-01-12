/*
 * Reflow oven control software using UDP communication with LabView.
 * Developer: sabreatom
 * Date: 08.01.2019
 */

#include "max6675.h"
#include <Ethernet.h>
#include <EthernetUdp.h>

//-------------------------------------------
//pin definition:
//-------------------------------------------

#define BUZZER_PIN        9
#define RED_LED_PIN       8
#define GREEN_LED_PIN     7

#define HEATER_CNTRL_PIN  3

#define THERMO_DO_PIN     4
#define THERMO_CS_PIN     5
#define THERMO_CLK_PIN    6

//-------------------------------------------
//ethernet related definition:
//-------------------------------------------

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(192, 168, 1, 10);

unsigned int localPort = 8888;      // local port to listen on

#define _RX_PACKET_MAX_SIZE  128
#define _TX_PACKET_MAX_SIZE  3

//ethernet packet buffers:
char packet_tx_buffer[_TX_PACKET_MAX_SIZE];
char packet_rx_buffer[_RX_PACKET_MAX_SIZE];

// An EthernetUDP instance to let us send and receive packets over UDP:
EthernetUDP Udp;

//-------------------------------------------
//Reflow oven control commands:
//-------------------------------------------

//UDP packet command opcode definitions:
#define HEATER_CONTROL_CMD            0x01
#define OVEN_CONTROL_RESERVE_CMD      0x02
#define OVEN_CONTROL_RELEASE_CMD     0x03

//UDP packet offset definitions:
#define PCKT_CMD_OFFSET               0
#define PCKT_ARG_OFFSET               1

//UDP packet heater state argument definitions:
#define HEATER_STATE_ON               0x01
#define HEATER_STATE_OFF              0x00

//UDP status packet offsets:
#define STATUS_TEMP_MSB_OFFSET        0
#define STATUS_TEMP_LSB_OFFSET        1
#define STATUS_HEATER_STATE_OFFSET    2

//-------------------------------------------
//Thermocouple related definitions:
//-------------------------------------------

MAX6675 thermocouple(THERMO_CLK_PIN, THERMO_CS_PIN, THERMO_DO_PIN);

//-------------------------------------------
//Heater related definitions:
//-------------------------------------------

#define HEATER_ON   digitalWrite(HEATER_CNTRL_PIN, HIGH);
#define HEATER_OFF   digitalWrite(HEATER_CNTRL_PIN, LOW);

//-------------------------------------------
//Master device status structure:
//-------------------------------------------

bool oven_reserved = false;
IPAddress master_ip;
unsigned int master_port;

//-------------------------------------------

void setup() {
  
  
  // start the ethernet:
  Ethernet.begin(mac, ip);

  //open serial communications:
  Serial.begin(9600);

  //check ethernet hardware status:
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(localPort);
}

void loop() {
  //read temperature:
  double temperature = thermocouple.readCelsius();

  //check if command received:
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Udp.read(packet_rx_buffer, _RX_PACKET_MAX_SIZE);

    switch (packet_rx_buffer[PCKT_CMD_OFFSET]){ //check command opcode
      case HEATER_CONTROL_CMD:
        Serial.println("Received heater control command.");
        //check heater's new state:
        switch (packet_rx_buffer[PCKT_ARG_OFFSET]){
          case HEATER_STATE_ON:
            Serial.println("Heater on.");
            HEATER_ON;
            break;
          case HEATER_STATE_OFF:
            Serial.println("Heater off.");
            HEATER_OFF;
            break;
          default:
            Serial.println("Undefined heater state.");
            break;
        }
        break;
      case OVEN_CONTROL_RESERVE_CMD:
        Serial.println("Received oven reservation command.");
        if (!oven_reserved){
          master_ip = Udp.remoteIP();
          master_port = Udp.remotePort();
          oven_reserved = true;
          Serial.println("Oven reserved.");
        }
        else{
          Serial.println("Oven already reserved.");
        }
        break;
      case OVEN_CONTROL_RELEASE_CMD:
        Serial.println("Received oven release command.");
        if (oven_reserved){
          oven_reserved = false;
          if (digitalRead(HEATER_CNTRL_PIN)){
            HEATER_OFF;
          }
        }
        break;
      default:
        Serial.println("Received unknown command.");
        break;
    }
  }

  //send status packet if reserved:
  if (oven_reserved){ //send status only when oven is reserved
    Udp.beginPacket(master_ip, master_port);
    int temp_trunc = (int)temperature;
    packet_tx_buffer[STATUS_TEMP_MSB_OFFSET] = ((temp_trunc & 0xFF00) > 8);
    packet_tx_buffer[STATUS_TEMP_LSB_OFFSET] = temp_trunc & 0xFF;
    packet_tx_buffer[STATUS_HEATER_STATE_OFFSET] = digitalRead(HEATER_CNTRL_PIN);
    Udp.write(packet_tx_buffer, _TX_PACKET_MAX_SIZE);
    Udp.endPacket();
  }

  //safety logic:
  if ((!oven_reserved) && (digitalRead(HEATER_CNTRL_PIN))){ //if oven not reserved and heater is still on then disable it
    HEATER_OFF;
  }
}

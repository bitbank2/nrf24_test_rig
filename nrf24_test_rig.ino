//
// nRF24 test rig
// Copyright (c) 2018 BitBank Software, Inc.
// Written by Larry Bank (bitbank@pobox.com)
// Project started 10/21/2018
//
#include <BitBang_I2C.h>
#include <RF24.h>
#include <oled_96.h>

static uint8_t iChannel, iPower;
static int iPackets;

#define RF_PACKET_SIZE 8
#define RF_PING    0x40
#define RF_CHANNEL 0x50

#define NRF24_CE_PIN 10
#define NRF24_CSN_PIN 14
#define NRF24_IRQ_PIN 9
#define SDA_PIN 3
#define SCL_PIN 2
#define BUTTON0 4
#define BUTTON1 5

// instantiate radio object
RF24 radio(NRF24_CE_PIN, NRF24_CSN_PIN); // CE, CSN
const byte rxAddr[6] = "TEST1";

//
// Update the OLED display with status info
// different info for TX versus RX mode
//
void ShowStatus(int bTX)
{
char szTemp[16];

  if (bTX) // tx mode
  {
    sprintf(szTemp, "Chan %d ", iChannel);
    oledWriteString(0,1,szTemp, FONT_NORMAL, 0);
    sprintf(szTemp, "Pwr %d", iPower);
    oledWriteString(0,2,szTemp, FONT_NORMAL, 0);
  }
  else // rx mode
  {
    sprintf(szTemp, "Chan %d ", iChannel);
    oledWriteString(0,1,szTemp, FONT_NORMAL, 0);
    sprintf(szTemp, "Pkts: %d ", iPackets);
    oledWriteString(0,2,szTemp, FONT_SMALL, 0);    
  }
} /* ShowStatus() */

void setup()
{
  pinMode(BUTTON0, INPUT_PULLUP); // buttons can be wired to ground by using input_pullup
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(NRF24_IRQ_PIN, INPUT);
  
// void oledInit(int iAddr, int iType, int bFlip, int bInvert, int sda, int scl)
 oledInit(0x3c, OLED_64x32, 0, 0, SDA_PIN, SCL_PIN);
 oledFill(0);
 if (!radio.begin())
    oledWriteString(0,0,(char *)"nRF24 bad",FONT_SMALL, 0);
 else
    oledWriteString(0,0,(char *)"nRF24 good",FONT_SMALL, 0);
// setRetries(delay, count);
// delay - How long to wait between each retry, in multiples of 250us,
// max is 15.  0 means 250us, 15 means 4000us.
// count - How many retries before giving up, max 15
 radio.setRetries(8, 5); // 2ms, 5 retries
 radio.openWritingPipe(rxAddr);
 radio.setDataRate( RF24_250KBPS );
 radio.setPayloadSize(RF_PACKET_SIZE);
 radio.stopListening();
 radio.powerDown();
} /* setup() */

void loop()
{
uint8_t ucTemp[32];
char szTemp[16];
int i, bSignal;
uint8_t u8B0, u8B1, u8OldB0, u8OldB1;

  u8OldB0 = u8OldB1 = 1; // previous button states
  
  iChannel = 76; // start at default channel
  iPower = 0; // start at lowest power level
  
  radio.setChannel(iChannel);
  radio.setPALevel(iPower);
  delay(2000); // allow time for user to decide if they want to be in TX or RX mode
  
  if (digitalRead(BUTTON0) == LOW || digitalRead(BUTTON1) == LOW) // go into RX mode if either button pressed
  {
    oledWriteString(0,0,(char *)"RX Mode ", FONT_NORMAL, 1);
    radio.powerUp();
    radio.openReadingPipe(0, rxAddr);
    radio.startListening();
    iPackets = 0;
    while (1)
    {
        u8B0 = digitalRead(BUTTON0);
        u8B1 = digitalRead(BUTTON1);
        if (u8B0 == LOW && u8B1 == LOW) // both buttons pressed = reset
        {
          oledFill(0);
          oledWriteString(0,0,(char *)"Reset...", FONT_NORMAL, 0);
          return; // allows for switching modes after powerup
        }
        if (u8B0 == LOW && u8B0 != u8OldB0) // channel up
        {
          iChannel++;
          if (iChannel > 125) // 0-125
            iChannel = 0;
          radio.setChannel(iChannel);
          ShowStatus(0);
        }
        if (u8B1 == LOW && u8B1 != u8OldB1) // power up
        {
          iPower++;
          iPower &= 3; // 0-3
          radio.setPALevel(iPower);
          ShowStatus(0);
        }
        u8OldB0 = u8B0; u8OldB1 = u8B1;
      delay(50);
      if (digitalRead(NRF24_IRQ_PIN) == LOW) // data waiting
      {
        bSignal = radio.testRPD();
        while (radio.available()) // may be more than 1 packet in the queue
        {
          radio.read(ucTemp, RF_PACKET_SIZE); // all packets are fixed length
          if (ucTemp[0] == RF_CHANNEL)
          {
             iChannel = ucTemp[1]; // transmitter told us to change the channel number
             radio.setChannel(iChannel);
          }
          iPackets++;
          ShowStatus(0);
          if (bSignal) // good signal on return?
            oledWriteString(0,3,(char *)"sig strong", FONT_SMALL, 0);
          else
            oledWriteString(0,3,(char *)"sig weak  ", FONT_SMALL, 0);
        }
      } // IRQ indicates data is waiting
    }
  }
  else // TX mode
  {
    oledWriteString(0,0,(char *)"TX Mode ", FONT_NORMAL, 1);
    radio.powerUp();
    radio.stopListening();
    ShowStatus(1);
    while (1)
    {
      for (i=0; i<10; i++) // total delay = 10 x 50ms = 2x per second to transmit packets
      {
        u8B0 = digitalRead(BUTTON0);
        u8B1 = digitalRead(BUTTON1);
        if (u8B0 == LOW && u8B1 == LOW) // both buttons pressed = reset
        {
          oledFill(0);
          oledWriteString(0,0,(char *)"Reset...", FONT_NORMAL, 0);
          return; // allows for switching modes after powerup
        }
        if (u8B0 == LOW && u8B0 != u8OldB0) // channel up
        {
          iChannel++;
          if (iChannel > 125) // 0-125
             iChannel = 0;
          // Tell receiver to change the channel too
          ucTemp[0] = RF_CHANNEL;
          ucTemp[1] = (uint8_t)iChannel;
          radio.write(ucTemp, RF_PACKET_SIZE);
          
          radio.setChannel(iChannel);
          ShowStatus(1);
        }
        if (u8B1 == LOW && u8B1 != u8OldB1) // power up
        {
          iPower++;
          iPower &= 3; // 0-3
          radio.setPALevel(iPower);
          ShowStatus(1);
        }
        u8OldB0 = u8B0; u8OldB1 = u8B1;
        delay(50);
      } // read buttons for 1/2 second
      // Send this info to the base station
      ucTemp[0] = RF_PING;
      if (radio.write(ucTemp, RF_PACKET_SIZE)) // fixed size to make it easier for receiver
      {
        oledWriteString(0,3,(char *)"Ack recvd ", FONT_SMALL, 0);
      }
      else
      {
        oledWriteString(0,3,(char *)"Ack failed", FONT_SMALL, 0);
      }
    } // while 1
  } // TX mode
} /* loop() */

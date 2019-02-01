/*********************************************************************
  This is an example for our nRF51822 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/
#include <DmxSimple.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
//#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif

#define CHANNELS 129
#define SCENES     8

int counter = 0;
int channelValues[3];
int currentChannels[CHANNELS];
int blackout[CHANNELS];
int scenes[SCENES][CHANNELS];
int i = 1;

bool validScenes[SCENES];
bool off = false;


/*=========================================================================
    APPLICATION SETTINGS

      FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
     
                                Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                                running this at least once is a good idea.
     
                                When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                                Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
         
                                Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
/*
  SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

  Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  DmxSimple.usePin(4);
  DmxSimple.maxChannel(128);
  for (int i = 0; i <= 128; i++) {
    currentChannels[i] = 0;
    blackout[i] = 0;
    if (i < 8) {
      validScenes[i] = false;
    }
  }
  while (!Serial);  // required for Flora & Micro
  delay(500);

  Serial.begin(2000000);
  Serial.println(F("Adafruit Bluefruit Command <-> Data Mode Example"));
  Serial.println(F("------------------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
    delay(500);
  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  // Check for user input
  int n, inputs[BUFSIZE + 1];

  // Echo received data
  while ( ble.available() )
  {

    int c = ble.read();
    if (c != 0) {
      channelValues[counter] = c;
      counter++;

      // Data receiving codes:
      // 1 - Standard data transmission. Receives a channel number and value
      // 2 - Edge case of standard. Receives a channel number. Used to take care of max value 255.
      // 3 - Run. Changes are ignored. Runs through scenes.
      // 4 - Scene Save. Saves the current channel values.
      // 5 - Scene Timer. How long the current scene runs for before transitioning to next scene.
      // 6 - Transition Timer. How long it should take to go between Scenes.
      // 7 - None
      // 8 - None
      // 9 - Blackout 
      if (counter >= 3) {

        // Code: 1
        if (channelValues[0] == 1 && off == false) {

          currentChannels[channelValues[1]] = channelValues[2] - 1;
          DmxSimple.write(channelValues[1], channelValues[2] - 1);

          Serial.print(channelValues[1]);
          Serial.print(", ");
          Serial.println(channelValues[2] - 1);
          for (i = 1; i < 15; i++) {
            Serial.print(currentChannels[i]);
            Serial.print(" ");
          }
          
          // Code: 2
        } else if (channelValues[0] == 2) {
          currentChannels[channelValues[1]] = 255;
          DmxSimple.write(channelValues[1], 255);

          Serial.print(channelValues[1]);
          Serial.print(", ");
          Serial.println(255);
          for (i = 1; i < 15; i++) {
            Serial.print(currentChannels[i]);
            Serial.print(" ");
          }

          // Code: 4
        } else if (channelValues[0] == 4) {
          if (channelValues[1] == 1) {
            for (i = 1; i <= CHANNELS; i++) {
              scenes[channelValues[2]][i] = currentChannels[i];
            }
          } else if (channelValues[1] == 2) {
            for (i = i; i <= CHANNELS; i++ ) {
              currentChannels[i] = scenes[channelValues[2]][i];
              DmxSimple.write(i,currentChannels[i]);
            }
          }


          // Code: 9
        } else if (channelValues[0] == 9) {
          if (off) {
            off = false;
            for (int i = 1; i < 129; i++) {
              DmxSimple.write(i, currentChannels[i]);
              //counter = 0;
            }
          } else {
            off = true;
            for (int i = 1; i < 129; i++) {
              DmxSimple.write(i, blackout[i]);
              //counter = 0;
            }
          }
          if (channelValues[0] == 4 && off == false) {
            if (channelValues[1] == 0) {
              for (int i = 1; i < 129; i++) {
                scenes[channelValues[2]][i] = currentChannels[i];
              }
            }
            //counter = 0;
          }

        } counter = 0;
      }
    }
  }
}

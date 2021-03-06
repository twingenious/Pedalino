/*  __________           .___      .__  .__                   ___ ________________    ___
 *  \______   \ ____   __| _/____  |  | |__| ____   ____     /  / \__    ___/     \   \  \   
 *   |     ___// __ \ / __ |\__  \ |  | |  |/    \ /  _ \   /  /    |    | /  \ /  \   \  \  
 *   |    |   \  ___// /_/ | / __ \|  |_|  |   |  (  <_> ) (  (     |    |/    Y    \   )  )
 *   |____|    \___  >____ |(____  /____/__|___|  /\____/   \  \    |____|\____|__  /  /  /
 *                 \/     \/     \/             \/           \__\                 \/  /__/
 *                                                                (c) 2018 alf45star
 *                                                        https://github.com/alf45tar/Pedalino
 */

#define DEBUG_PEDALINO
//#define BLYNK_DEBUG

#include "Pedalino.h"
#include "Serialize.h"
#include "Controller.h"
#include "BlynkRPC.h"
#include "Config.h"
#include "MIDIRouting.h"
#include "Display.h"
#include "Menu.h"

// Standard setup() and loop()

void setup(void)
{
#ifdef DEBUG_PEDALINO
  SERIALDEBUG.begin(115200);

  DPRINTLNF("");
  DPRINTLNF("  __________           .___      .__  .__                   ___ ________________    ___");
  DPRINTLNF("  \\______   \\ ____   __| _/____  |  | |__| ____   ____     /  / \\__    ___/     \\   \\  \\");
  DPRINTLNF("   |     ___// __ \\ / __ |\\__  \\ |  | |  |/    \\ /  _ \\   /  /    |    | /  \\ /  \\   \\  \\");
  DPRINTLNF("   |    |   \\  ___// /_/ | / __ \\|  |_|  |   |  (  <_> ) (  (     |    |/    Y    \\   )  )");
  DPRINTLNF("   |____|    \\___  >____ |(____  /____/__|___|  /\\____/   \\  \\    |____|\\____|__  /  /  /");
  DPRINTLNF("                 \\/     \\/     \\/             \\/           \\__\\                 \\/  /__/");
  DPRINTLNF("                                                                (c) 2018 alf45star");
  DPRINTLNF("                                                        https://github.com/alf45tar/Pedalino");
  DPRINTLNF("");

#endif

  // Reset to factory default if RIGHT key is pressed and hold for 5 seconds at power on
  pinMode(A0, INPUT_PULLUP);
  unsigned long milliStart = millis();
  while ((digitalRead(A0) == LOW) && ((millis() - milliStart) < 5000)) {
    DPRINT("#");
    delay(100);
  }
  DPRINTLN("");
  if (digitalRead(A0) == LOW) {
    DPRINTLN("Reset EEPROM to factory default");
    load_factory_default();
    update_eeprom();
  }

  read_eeprom();

  // Initiate serial MIDI communications, listen to all channels and turn Thru off
#ifndef DEBUG_PEDALINO
  USB_MIDI.begin(MIDI_CHANNEL_OMNI);
  interfaces[PED_USBMIDI].midiThru ? USB_MIDI.turnThruOn() : USB_MIDI.turnThruOff();
#endif
  DIN_MIDI.begin(MIDI_CHANNEL_OMNI);
  interfaces[PED_DINMIDI].midiThru ? DIN_MIDI.turnThruOn() : DIN_MIDI.turnThruOff();
  ESP_MIDI.begin(MIDI_CHANNEL_OMNI);
  ESP_MIDI.turnThruOff();

  autosensing_setup();
  controller_setup();
  mtc_setup();
  midi_routing_start();

  pinMode(LCD_BACKLIGHT, OUTPUT);
  analogWrite(LCD_BACKLIGHT, backlight);

  irrecv.enableIRIn();                            // Start the IR receiver
  irrecv.blink13(true);

  bluetooth.begin(9600);                          // Start the Bluetooth receiver
  bluetooth.println(F("AT+NAME=Pedalino™"));      // Set bluetooth device name
  Blynk.config(bluetooth, blynkAuthToken);

  //Serial1.begin(115200);
  //delay(10);

  //Blynk.begin(blynkAuthToken, wifi, "MyGuest", "0123456789");

  menu_setup();
}

void loop(void)
{
  if (serialPassthrough) {
    
    static bool startSerialPassthrough = true;
    
    if (startSerialPassthrough) {
      lcd.clear();
      lcd.print("Firmware upload");
      lcd.setCursor(0, 1);
      lcd.print("Reset to stop");
      startSerialPassthrough = false;
    }

    if (Serial.available()) {         // If anything comes in Serial (USB),
      Serial3.write(Serial.read());   // read it and send it out Serial3
    }

    if (Serial3.available()) {        // If anything comes in Serial3
      Serial.write(Serial3.read());   // read it and send it out Serial (USB)
    }
  }
  else {
    // Display menu on request
    menu_run();

    // Process Blynk messages
    Blynk.run();

    // Check whether the input has changed since last time, if so, send the new value over MIDI
    midi_refresh();
    midi_routing();
  }
}

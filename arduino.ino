// This file will not be the actual file to be used but rather to build functions that
// can be used for the arduino file when it arrives.
#include <Conceptinetics.h>

#define SCENES      8
#define CHANNELS  129
#define RXEN_PIN    2

DMX_Master           dmx_master ( (CHANNELS - 1), RXEN_PIN)

int channels[SCENES][CHANNELS];
int currentChannel[CHANNELS];
int nextScene[CHANNELS];
bool availableScenes[SCENES];
int transitionTimer = 1;
int sceneTimer = 1;
int currentChannel = 1;
int currentScene = 0;
bool play = false;

// Updates specified channel to specified value
void updateChannel(int channel, int value) {
   currentChannel[channel] = value;
}

// Sets timer value
void setTimer(int time) {
   sceneTimer = time;
}
// Sets transition value
void setTransition(int time) {
   transitionTimer = time;
}

// Sets which scene should be the current scene
void sceneSelect(int choice) {
  currentScene = choice;
}

void setScene(int newScene[]) {
  sceneSelect = newScene[sizeof(newScene) - 1];
  for (int channel = 1; channel <= sizeof(newScene) - 2; channel++) {
     channels[sceneSelect][channel] = newScene[channel];
  }
  availableScenes[sceneSelect] = true;
}

void setup() {

  // This gets commented out since the DMX shield can not be used when
  // serial is running (two uses of UART with only one UART available)
  Serial.begin(115200);
  Serial.println("Initializing default channel values...");

  // This gets uncommented when the code finally compiles without problems
  // Begins dmx transmission
  // Sends a 0 to all channels(removes anything stuck in memory)
  dmx_master.enable();
  dmx_master.setChannelRange(1, CHANNELS - 1, 0);
  
  // initialize the array with empty values
  for (int scene = 0; scene < SCENES; scene++ ) {
    availableScenes[scene] = false;
    for (int channel = 0; channel < CHANNELS; channel++) {
      channel[i] = 0;
    }
  }

  Serial.println("Setting BLE device name...");
  // Set your BLE Shield name here, max. length 10
  // ble_set_name("TestingBLE");

  Serial.println("Starting BLE advertising...");
  // Init. and start BLE library.
  // ble_begin();

  Serial.println("Arduino is ready!");
}

void loop() {

  // TODO: get data by bluetooth

  // Extract useable data from BT
  int command = data[0];

  // Checks if independent running is set
  if (command == 5 && play == true) {
     play = !play;
  }

  // Only allow for changes when independent running is off
  if (play != true) {
  // Commands that can be given are:
  // 0 - set entire scene
  // 1 - Update single channel
  // 2 - Set scene timer
  // 3 - Set transition timer
  // 4 - Scene select
  // 5 - Independently run
    switch (command) {
      case 0;
        setScene(data);
        break;
      case 1:
        updateChannel(data[1], data[2]);
        break;
      case 2:
        setTimer(data[1]);
        break;
      case 3:
        setTransition(data[1]);
        break;
      case 4:
        sceneSelect(data[1]);
        break;
      case 5:
        play = !play;
        break;
      default:
        break;
     }
  }

  // Independently run through the scenes otherwise just output current values
  if (play) {
  // TODO: Write code to output smoothly the transitions
  } else {
    for (int running = 1; running < CHANNELS; running++) {
      // output the values to DMX
      dmx_master.setChannelValue(i, currentChannel[i]);
    }
  }
}
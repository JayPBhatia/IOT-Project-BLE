#include "BLEDevice.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// GAME Server UUID
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914c");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");


//   **********************************************************  BLE LOGIC  ****************************************************
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.write(pData, length);
  Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}
  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517);  //set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  connected = true;
  return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }  // Found our server
  }  // onResult
};  // MyAdvertisedDeviceCallbacks

//   **********************************************************  BLE LOGIC END ****************************************************

// player would use button press to flip flag to enable/disable persist data to file in server
const int buttonPin = 12;
int buttonState = 0; 
void onButtonPress(){
    //flip 
    buttonState = buttonState==0 ? 1 : 0;
    Serial.print("Button Press: buttonState flipped to : ");
    Serial.println(buttonState);

}

// player would use joystick to update position 
const int analogX = 4;
const int analogY = 39;
struct PlayerPixel {
    size_t row;
    size_t col;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
PlayerPixel player1{ 0, 0, 100, 0, 0 };
void readJoysticks() {
    // p1
    int x1 = analogRead(analogX);
    int y1 = analogRead(analogY);
    if (x1 > 750) {
      player1.row += 1;
      player1.row = player1.row % 4;
    } else if (x1 < 250) {
      player1.row--;
      player1.row = player1.row % 4;
    }
    if (y1 > 750) {
      player1.col--;
      player1.col = player1.col % 8;
    } else if (y1 < 250) {
      player1.col++;
      player1.col = player1.col % 8;
    }

    Serial.print("Player Position ");
    Serial.print(player1.row);
    Serial.print(",");
    Serial.println(player1.col);
}

uint32_t chipId = 0;
int SDFailed = 0;  // Fail Flag
File dataFile; 
bool isFileOpen =false;

void setup() {

  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  
  // for esp32 default resolution is 12 bit which would give 0 - 4095 
  // for arduino we were getting 0-1023 
  // to make it behave same as arduino we set resolution to 10 bit 
  analogReadResolution(10);

  Serial.begin(115200);
  while(!Serial){};

  if( SD.begin() == 0)
  {
    Serial.println("SD Card Setup Failed!");
    SDFailed = 1;
  }
  else
  {
    dataFile = SD.open("datalog.csv", FILE_WRITE);
    if(dataFile == 0 )
    {
      Serial.println("Can't open datalog.csv");
      SDFailed = 1;
    }
    else
    {
      isFileOpen = true;
      dataFile.println("xval, yval");
      Serial.println("Datafile Sucessfully created");
    }
  }

  //****** BLE  Setup ******
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  //****** BLE  Setup END ******

  // other pins and interrupt setup 
  pinMode(buttonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin), onButtonPress, RISING);

}  // End of setup.


typedef enum {
    UPDATE_COORDS = 0,
    PLAYER_NAME = 1,
    RECORD = 2,
} Command;

typedef struct {
    Command command;
    // char pname[8];

    // record: stored in params[0]
    // update coords: playerid, px, py
    uint32_t chip_id;
    int32_t params[4];

} BLECommandPacket;

// This is the Arduino main loop function.
void loop() {

  static int lastRecordSentState = 0;
  // update player position using joystick input
  readJoysticks();

  if( buttonState == 1)
  {
      if(SDFailed == 0 && isFileOpen == false)
      {
        dataFile = SD.open("datalog.csv", FILE_WRITE);
        if(dataFile == 0 )
        {
          Serial.println("Can't open datalog.csv");
          SDFailed = 1;
        }
        else
        {
          isFileOpen = true;
          dataFile.println("xval, yval");
          Serial.println("Datafile Sucessfully created");
        }
      }

      if(SDFailed == 0 && isFileOpen == true )
      {
        dataFile.print(player1.row);
        dataFile.print(',');
        dataFile.println(player1.col);
        Serial.print("File Updated " );   
      }
  }
  else if ( buttonState == 0 && isFileOpen )
  {       
    dataFile.close();
    SD.end();
  }

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");

      Serial.println("updated button state to 0");
      buttonState = 0;
      lastRecordSentState = 0;
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    Serial.println("Connected");
    //String newValue = "Time since boot: " + String(millis() / 1000);
    //Serial.println("Setting new characteristic value to \"" + newValue + "\"");

    BLECommandPacket bleCommand;
    bleCommand.command = Command::UPDATE_COORDS;
    bleCommand.chip_id = chipId;
    bleCommand.params[0] = player1.row;
    bleCommand.params[1] = player1.col;
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue((uint8_t*)&bleCommand, sizeof(bleCommand));

    if(lastRecordSentState != buttonState)
    {
      BLECommandPacket bleCommand;
      bleCommand.command = Command::RECORD;
      bleCommand.chip_id = chipId;
      bleCommand.params[0] = buttonState;
      
      // Set the characteristic's value to be the array of bytes that is actually a string.
      pRemoteCharacteristic->writeValue((uint8_t*)&bleCommand, sizeof(bleCommand));
      Serial.print("Record command sent with buttonState ");
      Serial.println(buttonState);
      lastRecordSentState = buttonState;
    }
  } 
  else if (doScan) 
  {
    Serial.println("getScan");
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }


  delay(500);  // Delay a second between loops.
}  // End of loop

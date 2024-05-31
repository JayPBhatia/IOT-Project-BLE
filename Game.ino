#include <Tone.h>
#include <Arduino_FreeRTOS.h>
#include <ScreenRender.h>

// macro to convert millis to tick
#define MS(x) (x / portTICK_PERIOD_MS)

// Pins
#define pinRotaryInputA 2
#define pinRotaryInputB 8
#define pinHcsrTrigger 7
#define pinHcsrDistance 3
#define pinScreenData 6
#define pinTone 11

// position data
const int positionMin = 0;
const int positionMax = 100;
volatile int position = 50;
volatile int aLastState = 0;

// distance data
volatile int distanceTimeDiff = 0;
const int maxDistance = 20;
Tone notePlayer;

int player1Id;
int player2Id;
PlayerPixel player1{ 0, 0, 100, 0, 0 };
PlayerPixel player2{ 0, 7, 0, 100, 0 };

int toneFlags = 0;
// renderer for the led matrix
ScreenRender matrix(pinScreenData, 4, 8);
// read input and update position
void updatePositionISR() {

  // Reads the "current" state of the pinRotaryInputA
  auto aState = digitalRead(pinRotaryInputA);

  // If the previous and the current state of the pinRotaryInputA are different, that means a Pulse has occured
  if (aState != aLastState) {

    // If the pinRotaryInputB state is different to the pinRotaryInputA state, that means the encoder is rotating clockwise
    int bState = digitalRead(pinRotaryInputB);

    if (bState != aState) {
      position++;
    } else {
      position--;
    }

    if (position < positionMin) {
      position = positionMax;
    } else if (position > positionMax) {
      position = positionMin;
    }
    //matrix.SetBrightness(position);
    // Updates the previous state of the pinRotaryInputA with the current state
    aLastState = aState;
  }
}



void updateDistanceISR() {
  static int prevMicros = 0;

  if (digitalRead(pinHcsrDistance) == HIGH) {
    prevMicros = micros();
  } else {
    distanceTimeDiff = micros() - prevMicros;
  }

  if (distanceTimeDiff < 0) distanceTimeDiff = 0;

  if (position > 50 && distanceTimeDiff < 300) {
    if (!notePlayer.isPlaying()) {
      notePlayer.play(NOTE_A3);
    }
    toneFlags |= 1;
  } else {
    if (notePlayer.isPlaying() && toneFlags == 1) {
      notePlayer.stop();
    }
    toneFlags &= 2;
  }
}

void readJoysticks(void* p) {
  while (1) {
    // p1
    int x1 = analogRead(0);
    int y1 = analogRead(1);
    if (x1 > 800) {
      player1.row += 1;
      player1.row = player1.row % 4;
    } else if (x1 < 250) {
      player1.row--;
      player1.row = player1.row % 4;
    }
    if (y1 > 800) {
      player1.col--;
      player1.col = player1.col % 8;
    } else if (y1 < 250) {
      player1.col++;
      player1.col = player1.col % 8;
    }
    // p2
    int x2 = analogRead(2);
    int y2 = analogRead(3);
    if (x2 > 800) {
      player2.row--;
      player2.row = player2.row % 4;
    } else if (x2 < 250) {
      player2.row++;
      player2.row = player2.row % 4;
    }
    if (y2 > 800) {
      player2.col += 1;
      player2.col = player2.col % 8;
    } else if (y2 < 250) {
      player2.col--;
      player2.col = player2.col % 8;
    }
    matrix.SetPlayerPosition(player1Id, player1.row, player1.col);
    matrix.SetPlayerPosition(player2Id, player2.row, player2.col);
    matrix.UpdateScreen();
    if (position > 50 && (player1.row == player2.row) && (player1.col == player2.col)) {
      if (!notePlayer.isPlaying()) {
        notePlayer.play(NOTE_A3);
      }

      toneFlags |= 2;
    } else {
      if (notePlayer.isPlaying() && (toneFlags == 2)) {
        notePlayer.stop();
      }
      toneFlags &= 1;
    }

    vTaskDelay(MS(100));
  }
}

void updateScreen(void* p) {

  while (1) {

    char starLine[positionMax + 1];
    memset(starLine, ' ', positionMax);
    starLine[positionMax] = '\0';

    taskENTER_CRITICAL();
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(pinHcsrTrigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinHcsrTrigger, LOW);
    taskEXIT_CRITICAL();

    int currentDistance = distanceTimeDiff / 100;
    int distance = currentDistance > maxDistance ? maxDistance : currentDistance;



    if (distance < maxDistance) {

      char blanks[maxDistance - distance + 1];
      memset(blanks, '\n', maxDistance - distance);
      blanks[maxDistance - distance] = '\0';
      Serial.println(blanks);
    }
    Serial.println(distanceTimeDiff);
    Serial.println(position);

    // X
    starLine[position] = 'X';
    Serial.println(starLine);


    // blanks
    char blanks[distance + 1];
    memset(blanks, '\n', distance);
    blanks[distance] = '\0';
    Serial.println(blanks);

    // Y
    starLine[position] = 'Y';
    Serial.println(starLine);


    vTaskDelay(MS(200));
  }
}


void setup() {
  // setup Serial Monitor
  Serial.begin(1000000);

  // setup rotary encoders
  pinMode(pinRotaryInputA, INPUT);
  pinMode(pinRotaryInputB, INPUT);

  pinMode(pinScreenData, OUTPUT);
  pinMode(pinHcsrDistance, INPUT);
  pinMode(pinHcsrTrigger, OUTPUT);


  pinMode(pinTone, OUTPUT);
  notePlayer.begin(pinTone);
  notePlayer.stop();

  attachInterrupt(digitalPinToInterrupt(pinRotaryInputA), updatePositionISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinHcsrDistance), updateDistanceISR, CHANGE);


  player1Id = matrix.AddPlayer(player1);

  player2Id = matrix.AddPlayer(player2);

  // Reads the initial state of the pinRotaryInputA
  aLastState = digitalRead(pinRotaryInputA);

  xTaskCreate(updateScreen, "update Screen", 300, NULL, 1, NULL);
  xTaskCreate(readJoysticks, "joysticks", 100, NULL, 1, NULL);
  vTaskStartScheduler();
}



void loop() {
}

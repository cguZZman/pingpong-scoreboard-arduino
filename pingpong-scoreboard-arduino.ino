#include "pitches.h"
#include "melodies.h"
#include "display.h"

#define CONFIG_SCORE_WIN_SW4 A4
#define CONFIG_SCORE_WIN_SW3 A3
#define CONFIG_SCORE_WIN_SW2 A2
#define CONFIG_SCORE_WIN_SW1 A1
#define CONFIG_SCORE_WIN_SW0 A0

#define CONFIG_SOUND_SW 13

#define BUZZER_PIN 12

#define LEFT_SERVER_BUTTON_ADD 11
#define LEFT_SERVER_BUTTON_UNDO 10
#define RIGHT_SERVER_BUTTON_ADD 9
#define RIGHT_SERVER_BUTTON_UNDO 8

#define CONFIG_CHANGE_SERVER_SW2 7
#define CONFIG_CHANGE_SERVER_SW1 6
#define CONFIG_CHANGE_SERVER_SW0 5

#define DATA_PIN 4
#define CLOCK_PIN 3
#define LATCH_PIN 2

#define MAX_BUTTONS 14
#define MAX_SCORE 99
#define LONG_PUSH 500

byte SERVER_BIT_SET = B10000000;
byte CHANGE_SERVER_AT_DEFAULT = 3;
byte CHANGE_SERVER_AT = 0;
byte SCORE_WIN_AT = 10;
byte SCORE_WIN_AT_DEFAULT = 10;

boolean playSound = false;
boolean isServerAssigned = false;
boolean haveWinner = false;
boolean isLeftWinner = false;
boolean isWinnerDigitsOn = false;

struct SCORE {
  byte left;
  byte right;
};

byte currentPoints = 0;
struct SCORE scoreWhen[MAX_SCORE * 2];
boolean leftServerStarted = false;

long startTime = 0;

boolean buttonState[MAX_BUTTONS];
long buttonPushStartTime[MAX_BUTTONS];
byte buttonLongPushTimes[MAX_BUTTONS];

byte buttonsSuscribed[] = {
  LEFT_SERVER_BUTTON_ADD,
  LEFT_SERVER_BUTTON_UNDO,
  RIGHT_SERVER_BUTTON_ADD,
  RIGHT_SERVER_BUTTON_UNDO,
  CONFIG_SOUND_SW
};

void setup() {
  pinMode(LEFT_SERVER_BUTTON_ADD, INPUT);
  pinMode(LEFT_SERVER_BUTTON_UNDO, INPUT);
  pinMode(RIGHT_SERVER_BUTTON_ADD, INPUT);
  pinMode(RIGHT_SERVER_BUTTON_UNDO, INPUT);

  pinMode(CONFIG_CHANGE_SERVER_SW2, INPUT);
  pinMode(CONFIG_CHANGE_SERVER_SW1, INPUT);
  pinMode(CONFIG_CHANGE_SERVER_SW0, INPUT);

  pinMode(CONFIG_SCORE_WIN_SW4, INPUT);
  pinMode(CONFIG_SCORE_WIN_SW3, INPUT);
  pinMode(CONFIG_SCORE_WIN_SW2, INPUT);
  pinMode(CONFIG_SCORE_WIN_SW1, INPUT);
  pinMode(CONFIG_SCORE_WIN_SW0, INPUT);

  pinMode(CONFIG_SOUND_SW, INPUT);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
  reset();
}

void reset() {
  currentPoints = 0;
  isServerAssigned = haveWinner = isWinnerDigitsOn = false;
  
  SCORE_WIN_AT = digitalRead(CONFIG_SCORE_WIN_SW4) << 4;
  SCORE_WIN_AT |= digitalRead(CONFIG_SCORE_WIN_SW3) << 3;
  SCORE_WIN_AT |= digitalRead(CONFIG_SCORE_WIN_SW2) << 2;
  SCORE_WIN_AT |= digitalRead(CONFIG_SCORE_WIN_SW1) << 1;
  SCORE_WIN_AT |= digitalRead(CONFIG_SCORE_WIN_SW0);
  if (SCORE_WIN_AT == 0) {
    SCORE_WIN_AT = SCORE_WIN_AT_DEFAULT;
  }
  CHANGE_SERVER_AT = digitalRead(CONFIG_CHANGE_SERVER_SW2) << 2;
  CHANGE_SERVER_AT |= digitalRead(CONFIG_CHANGE_SERVER_SW1) << 1;
  CHANGE_SERVER_AT |= digitalRead(CONFIG_CHANGE_SERVER_SW0);
  if (CHANGE_SERVER_AT == 0) {
    CHANGE_SERVER_AT = CHANGE_SERVER_AT_DEFAULT;
  }
  
  struct SCORE winDigits = getDigits(&SCORE_WIN_AT);
  updateScore(winDigits.left, winDigits.right, 17, CHANGE_SERVER_AT);
  playTone(NOTE_F5, 300);
  delay(500);
}

void playTone(unsigned int frequency, unsigned long duration) {
  if (playSound) {
    tone(BUZZER_PIN, frequency, duration);
  }
}

struct SCORE getDigits(byte *number) {
  struct SCORE d;
  d.right = *number % 10;
  d.left = (*number - d.right) / 10;
  return d;
}

void playMelody(int melody[][2]) {
  if (playSound) {
    for (int i = 0; melody[i][0] != -1; i++) {
      int noteDuration = 1000 / melody[i][1];
      playTone(melody[i][0], noteDuration);
      int pauseBetweenNotes = noteDuration * 1.20;
      delay(pauseBetweenNotes);
    }
  }
}

long timeDiff(long t1, long t2) {
  if (t2 < t1) {
    return t1 - t2;
  }
  return t2 - t1;
}

boolean isLeftServing() { return wasLeftServingWhen(currentPoints);}
boolean wasLeftServingWhen(byte points) {
  boolean odd = (points / CHANGE_SERVER_AT) % 2;
  return leftServerStarted?!odd:odd;
}

void updateScore() {
  struct SCORE *score = &scoreWhen[currentPoints];
  struct SCORE leftDigits = getDigits(&score->left);
  struct SCORE rightDigits = getDigits(&score->right);
  updateScore(leftDigits.left, leftDigits.right, rightDigits.left, rightDigits.right);
}

void updateScore(byte leftScoreLeftDigit, byte leftScoreRightDigit, byte rightScoreLeftDigit, byte rightScoreRightDigit) {
  /*
  Serial.print("[");
  Serial.print(leftScoreLeftDigit);
  Serial.print("][");
  Serial.print(leftScoreRightDigit);
  Serial.print("] ");
  if (isLeftServing()) {
    Serial.print("<-");
  } else {
    Serial.print("->");
  }
  Serial.print(" [");
  Serial.print(rightScoreLeftDigit);
  Serial.print("][");
  Serial.print(rightScoreRightDigit);
  Serial.println("]");
  */
  leftScoreRightDigit = DISPLAY_DIGIT[leftScoreRightDigit];
  leftScoreLeftDigit = DISPLAY_DIGIT[leftScoreLeftDigit];
  rightScoreRightDigit = DISPLAY_DIGIT[rightScoreRightDigit];
  rightScoreLeftDigit = DISPLAY_DIGIT[rightScoreLeftDigit];
  if (isLeftServing()) {
    leftScoreRightDigit |= SERVER_BIT_SET;
  } else {
    rightScoreRightDigit |= SERVER_BIT_SET;
  }

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, leftScoreLeftDigit);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, leftScoreRightDigit);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, rightScoreLeftDigit);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, rightScoreRightDigit);
  digitalWrite(LATCH_PIN, HIGH);
}

void addEffect(boolean left) {
  int delayTime = 50;
  struct SCORE *score = &scoreWhen[currentPoints];
  struct SCORE leftDigits = getDigits(&score->left);
  struct SCORE rightDigits = getDigits(&score->right);
  struct SCORE *toAdd = left?&leftDigits:&rightDigits;
  byte effect[][2] = {
    {11,10},{10,11},{10,12},{10,13},{10,14},{14,10},{15,10},{16,10},{255}
  };
  for (int times = 0; times < 2; times++) {
    for (int i = 0; effect[i][0] != 255; i++) {
      toAdd->left = effect[i][0];
      toAdd->right = effect[i][1];
      updateScore(leftDigits.left, leftDigits.right, rightDigits.left, rightDigits.right);
      delay(delayTime);
    }
  }
}

void increaseWinEffect() {
  int delayTime = 200;
  byte effect[] = {10,14,17,11,255};
  for (int i = 0; effect[i] != 255; i++) {
    delay(delayTime);
    updateScore(effect[i], effect[i], effect[i], effect[i]);
  }
  delay(delayTime);
  updateScore();
}

void scoreAdd(boolean left) {
  if (currentPoints < MAX_SCORE && !haveWinner) {
    //playMelody(MELODY_SCORE);
    playTone(NOTE_C4, 250);
    addEffect(left);
    struct SCORE *oldScore = &scoreWhen[currentPoints];
    boolean leftServingBefore = isLeftServing();
    byte scoreWinAt = getScoreWinAt();
    currentPoints++;
    struct SCORE *newScore = &scoreWhen[currentPoints];
    boolean leftServingNow = isLeftServing();
    newScore->left = oldScore->left + left;
    newScore->right = oldScore->right + !left;
    updateScore();
    byte *updatedScore = left?&newScore->left:&newScore->right;
    if (*updatedScore == scoreWinAt - 1 && newScore->left == newScore->right) {
      increaseWinEffect();
    }
    haveWinner = *updatedScore == scoreWinAt;
    if (haveWinner) {
      isLeftWinner = left;
    } else if (leftServingBefore != leftServingNow) {
      playMelody(MELODY_CHANGE_SERVER);
    }
  }
}

byte getScoreWinAt() {
  struct SCORE *score = &scoreWhen[currentPoints];
  boolean even = score->left == score->right;
  byte maxScore = max(score->right,score->left);
  byte scoreWinAt = SCORE_WIN_AT;
  if (maxScore >= scoreWinAt) {
    scoreWinAt = maxScore + (even?2:1);
  } else if (maxScore + 1 == scoreWinAt && even) {
    scoreWinAt = maxScore + 2;
  }
  return scoreWinAt;
}

void scoreUndo() {
  if (currentPoints > 0) {
    haveWinner = isWinnerDigitsOn = false;
    currentPoints--;
    updateScore();
    playMelody(MELODY_UNDO);
  }
}

void printEvent(byte button, int oldState, int newState, long timeInOldState) {
  Serial.print("B");
  Serial.print(button);
  Serial.print(" | ");
  Serial.print(oldState);
  Serial.print(" -> ");
  Serial.print(newState);
  Serial.print(" | ");
  Serial.println(timeInOldState);
}

void buttonEvent(byte button, int oldState, int newState, long timeInOldState) {
  printEvent(button, oldState, newState, timeInOldState);
  if (button == CONFIG_SOUND_SW) {
    playSound = newState;
  } else if (oldState == HIGH && newState == LOW && timeInOldState >= 40) {
    if (!isServerAssigned) {
      if (button == LEFT_SERVER_BUTTON_ADD || button == RIGHT_SERVER_BUTTON_ADD) {
        Serial.println("Server assigned");
        isServerAssigned = true;
        leftServerStarted = button == LEFT_SERVER_BUTTON_ADD;
        updateScore();
        playMelody(MELODY_CHANGE_SERVER);
      }
    } else if (button == LEFT_SERVER_BUTTON_ADD || button == RIGHT_SERVER_BUTTON_ADD) {
      Serial.println("Score added");
      scoreAdd(button == LEFT_SERVER_BUTTON_ADD);
    } else if (button == LEFT_SERVER_BUTTON_UNDO || button == RIGHT_SERVER_BUTTON_UNDO) {
      Serial.println("Score undo");
      scoreUndo();
    }
  } else if (oldState == HIGH && newState == HIGH && timeInOldState > LONG_PUSH * 2) {
    if (button == LEFT_SERVER_BUTTON_UNDO || button == RIGHT_SERVER_BUTTON_UNDO) {
      Serial.println("Reset");
      reset();
    }
  }
}

void readButtonEvents() {
  for (int i = 0; i < sizeof(buttonsSuscribed); i++) {
    int state = digitalRead(buttonsSuscribed[i]);
    long newTime = millis();
    long td = timeDiff(buttonPushStartTime[i], newTime);
    if (buttonState[i] != state) {
      if (state == HIGH) {
        td = 0;
      }
      buttonEvent(buttonsSuscribed[i], buttonState[i], state, td);
      buttonState[i] = state;
      buttonPushStartTime[i] = newTime;
      buttonLongPushTimes[i] = 0;
    } else if (buttonsSuscribed[i] != CONFIG_SOUND_SW && buttonState[i] == HIGH && td > (buttonLongPushTimes[i] + 1) * LONG_PUSH) {
      buttonEvent(buttonsSuscribed[i], buttonState[i], state, td);
      buttonLongPushTimes[i] += 1;
      buttonPushStartTime[i] = newTime;
    }
  }
}

void loop() {
  readButtonEvents();
  if (!isServerAssigned) {
    long now = millis();
    if (timeDiff(startTime, now) > 400) {
      startTime = now;
      leftServerStarted = !leftServerStarted;
      updateScore(17, 17, 17, 17);
    }
  } else if (haveWinner) {
    long now = millis();
    if (timeDiff(startTime, now) > 250) {
      startTime = now;
      if (isWinnerDigitsOn) {
        struct SCORE *score = &scoreWhen[currentPoints];
        struct SCORE leftDigits = getDigits(&score->left);
        struct SCORE rightDigits = getDigits(&score->right);
        struct SCORE *toTurnOff = isLeftWinner?&leftDigits:&rightDigits;
        toTurnOff->left = toTurnOff->right = 10;
        updateScore(leftDigits.left, leftDigits.right, rightDigits.left, rightDigits.right);
      } else {
        updateScore();
      }
      isWinnerDigitsOn = !isWinnerDigitsOn;
    }
  }
}


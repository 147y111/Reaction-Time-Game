/*
 * Two-Player Reaction Time Game
 * Board: Arduino Uno
 *
 * Hardware :
 *   - 2 buttons          -> Pin 2 (P1), Pin 3 (P2), other leg to GND (INPUT_PULLUP)
 *   - 2 player LEDs LED     -> Pin 4 (P1), Pin 5 (P2), through 220 ohm resistor to GND
 *   - 5 score LEDs LED      -> Pin 6,7,8,9,10 (left to right )
 *   - Start buzzer     -> Pin 11 (passive buzzer )
 *   - Victory buzzer   -> Pin 12 (passive buzzer )
 *
 * Rules :
 *   1. Random wait, then start buzzer beeps
 *   2. Both players press their button ASAP
 *   3. Faster player wins the round
 *   4. Points (1-5) based on winning margin  (1-5)
 *   5. Winner's LED flashes, then score LEDs light up left-to-right
 *      LED,LED
 *   6. After TOTAL_ROUNDS rounds, higher total score wins:
 *      winner LED flashes + victory melody + all 5 score LEDs flash together
 *      :LED++LED
 */

// ---------------- Pin definitions  ----------------
const int BTN_P1 = 2;
const int BTN_P2 = 3;
const int LED_P1 = 4;
const int LED_P2 = 5;
const int SCORE_LEDS[5] = {6, 7, 8, 9, 10};  // left to right
const int BUZZER_START   = 11;
const int BUZZER_VICTORY = 12;

// ---------------- Game settings  ----------------
const int TOTAL_ROUNDS = 5;              // number of rounds
const unsigned long TIMEOUT_MS = 3000;   // max wait for a press

// Winning-margin thresholds (ms) -> points () ->
// margin >= 400 -> 5 pts, >= 250 -> 4, >= 150 -> 3, >= 50 -> 2, else 1
const unsigned long MARGIN_THRESHOLDS[4] = {400, 250, 150, 50};

// ---------------- Game state  ----------------
int scoreP1 = 0;
int scoreP2 = 0;

// ============================================================
void setup() {
  pinMode(BTN_P1, INPUT_PULLUP);
  pinMode(BTN_P2, INPUT_PULLUP);
  pinMode(LED_P1, OUTPUT);
  pinMode(LED_P2, OUTPUT);
  for (int i = 0; i < 5; i++) pinMode(SCORE_LEDS[i], OUTPUT);
  pinMode(BUZZER_START, OUTPUT);
  pinMode(BUZZER_VICTORY, OUTPUT);

  Serial.begin(9600);
  randomSeed(analogRead(A0));  // unconnected pin for random noise

  Serial.println(F("=== Reaction Time Game  ==="));
  startupAnimation();
}

// ============================================================
void loop() {
  scoreP1 = 0;
  scoreP2 = 0;

  for (int round = 1; round <= TOTAL_ROUNDS; round++) {
    Serial.print(F("--- Round  "));
    Serial.print(round);
    Serial.println(F("  ---"));
    playRound();
    delay(1500);  // pause between rounds
  }

  announceWinner();

  // Wait for any button to restart the game
  Serial.println(F("Press any button to play again "));
  waitForAnyPress();
  delay(500);
}

// ============================================================
// One round of the game
void playRound() {
  // 1. Random wait (2-5 s). Watch for false starts during the wait.
  //    2-5,
  unsigned long waitTime = random(2000, 5001);
  unsigned long waitStart = millis();
  while (millis() - waitStart < waitTime) {
    if (digitalRead(BTN_P1) == LOW) { handleFalseStart(1); return; }
    if (digitalRead(BTN_P2) == LOW) { handleFalseStart(2); return; }
  }

  // 2. GO signal: start buzzer beeps :
  tone(BUZZER_START, 1000, 200);
  unsigned long goTime = millis();

  // 3. Record each player's reaction time
  unsigned long timeP1 = 0, timeP2 = 0;  // 0 = not pressed yet
  bool p1Done = false, p2Done = false;

  while ((!p1Done || !p2Done) && (millis() - goTime < TIMEOUT_MS)) {
    if (!p1Done && digitalRead(BTN_P1) == LOW) {
      timeP1 = millis() - goTime;
      p1Done = true;
      digitalWrite(LED_P1, HIGH);  // instant feedback
    }
    if (!p2Done && digitalRead(BTN_P2) == LOW) {
      timeP2 = millis() - goTime;
      p2Done = true;
      digitalWrite(LED_P2, HIGH);
    }
  }
  digitalWrite(LED_P1, LOW);
  digitalWrite(LED_P2, LOW);

  Serial.print(F("P1: ")); Serial.print(p1Done ? timeP1 : 0);
  Serial.print(F(" ms | P2: ")); Serial.print(p2Done ? timeP2 : 0);
  Serial.println(F(" ms"));

  // 4. Decide the round winner
  if (!p1Done && !p2Done) {
    Serial.println(F("Nobody pressed! No points. ,"));
    errorBeep();
    return;
  }

  int winner;
  unsigned long margin;
  if (!p2Done)      { winner = 1; margin = TIMEOUT_MS - timeP1; }
  else if (!p1Done) { winner = 2; margin = TIMEOUT_MS - timeP2; }
  else if (timeP1 < timeP2) { winner = 1; margin = timeP2 - timeP1; }
  else if (timeP2 < timeP1) { winner = 2; margin = timeP1 - timeP2; }
  else {
    Serial.println(F("Exact tie! No points. ,"));
    errorBeep();
    return;
  }

  // 5. Convert margin to points
  int points = marginToPoints(margin);
  if (winner == 1) scoreP1 += points; else scoreP2 += points;

  Serial.print(F("Winner : P")); Serial.print(winner);
  Serial.print(F(" | Margin : ")); Serial.print(margin);
  Serial.print(F(" ms | Points : ")); Serial.println(points);
  Serial.print(F("Total   P1: ")); Serial.print(scoreP1);
  Serial.print(F("  P2: ")); Serial.println(scoreP2);

  // 6. Flash winner's LED LED
  flashPlayerLED(winner, 3);

  // 7. Show points on score LEDs, left to right LED
  showScoreLEDs(points);
}

// ============================================================
// False start: opponent gets 1 point :1
void handleFalseStart(int player) {
  Serial.print(F("P")); Serial.print(player);
  Serial.println(F(" false start! Opponent +1. !1"));

  errorBeep();
  // Flash the offender's LED quickly as a warning LED
  int led = (player == 1) ? LED_P1 : LED_P2;
  for (int i = 0; i < 5; i++) {
    digitalWrite(led, HIGH); delay(100);
    digitalWrite(led, LOW);  delay(100);
  }

  if (player == 1) scoreP2 += 1; else scoreP1 += 1;
  showScoreLEDs(1);

  // Wait until both buttons released
  while (digitalRead(BTN_P1) == LOW || digitalRead(BTN_P2) == LOW) delay(10);
}

// ============================================================
// Margin (ms) -> 1-5 points
int marginToPoints(unsigned long margin) {
  if (margin >= MARGIN_THRESHOLDS[0]) return 5;
  if (margin >= MARGIN_THRESHOLDS[1]) return 4;
  if (margin >= MARGIN_THRESHOLDS[2]) return 3;
  if (margin >= MARGIN_THRESHOLDS[3]) return 2;
  return 1;
}

// ============================================================
// Light up N score LEDs left to right, hold, then clear
// LEDN,
void showScoreLEDs(int points) {
  for (int i = 0; i < points; i++) {
    digitalWrite(SCORE_LEDS[i], HIGH);
    tone(BUZZER_START, 800 + i * 200, 100);  // rising pitch
    delay(300);
  }
  delay(1000);
  for (int i = 0; i < 5; i++) digitalWrite(SCORE_LEDS[i], LOW);
}

// ============================================================
// Flash a player's LED LED
void flashPlayerLED(int player, int times) {
  int led = (player == 1) ? LED_P1 : LED_P2;
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH); delay(200);
    digitalWrite(led, LOW);  delay(200);
  }
}

// ============================================================
// Final winner celebration
void announceWinner() {
  Serial.println(F("=== FINAL  ==="));
  Serial.print(F("P1: ")); Serial.print(scoreP1);
  Serial.print(F("  P2: ")); Serial.println(scoreP2);

  if (scoreP1 == scoreP2) {
    Serial.println(F("It's a tie! !"));
    // Both LEDs + all score LEDs flash LEDLED
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_P1, HIGH); digitalWrite(LED_P2, HIGH);
      setAllScoreLEDs(HIGH); delay(250);
      digitalWrite(LED_P1, LOW);  digitalWrite(LED_P2, LOW);
      setAllScoreLEDs(LOW);  delay(250);
    }
    return;
  }

  int winner = (scoreP1 > scoreP2) ? 1 : 2;
  Serial.print(F("WINNER : P")); Serial.println(winner);

  int winnerLED = (winner == 1) ? LED_P1 : LED_P2;

  // Victory melody on the second buzzer + winner LED + all score LEDs flash together
  //  + LED + LED
  int melody[]    = {523, 659, 784, 1047, 784, 1047};  // C5 E5 G5 C6 G5 C6
  int duration[]  = {150, 150, 150, 300, 150, 500};

  for (int n = 0; n < 6; n++) {
    digitalWrite(winnerLED, HIGH);
    setAllScoreLEDs(HIGH);
    tone(BUZZER_VICTORY, melody[n], duration[n]);
    delay(duration[n]);
    digitalWrite(winnerLED, LOW);
    setAllScoreLEDs(LOW);
    delay(50);
  }

  // A few extra celebration flashes
  for (int i = 0; i < 6; i++) {
    digitalWrite(winnerLED, HIGH); setAllScoreLEDs(HIGH); delay(150);
    digitalWrite(winnerLED, LOW);  setAllScoreLEDs(LOW);  delay(150);
  }
}

// ============================================================
// Helpers
void setAllScoreLEDs(int state) {
  for (int i = 0; i < 5; i++) digitalWrite(SCORE_LEDS[i], state);
}

void errorBeep() {
  tone(BUZZER_START, 200, 400);
  delay(400);
}

void waitForAnyPress() {
  while (digitalRead(BTN_P1) == HIGH && digitalRead(BTN_P2) == HIGH) delay(10);
}

// Quick LED sweep at power-on LED
void startupAnimation() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(SCORE_LEDS[i], HIGH); delay(100);
  }
  digitalWrite(LED_P1, HIGH); digitalWrite(LED_P2, HIGH);
  tone(BUZZER_START, 880, 200);
  delay(400);
  digitalWrite(LED_P1, LOW); digitalWrite(LED_P2, LOW);
  setAllScoreLEDs(LOW);
}


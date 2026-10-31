/*
* Two-Player Reaction Time Game v12
* (Turn-based, Pin-13 godlike triple-flash, PWM fade)
* Board: Arduino Uno
* Button wired to 5V with 10k pull-down resistor (pressed = HIGH)
*
* Wiring:
* Shared button -> Pin 3
* P1 LED (left) -> Pin 4; P2 LED (right) -> Pin 5
* Score LEDs -> Pins 6, 7, 8, 9, 10, 13 (left to right)
* Start buzzer -> Pin 11 Victory buzzer -> Pin 12
*
* Scoring (based on each player's reaction time):
* <=125ms -> 6 pts (Godlike), <=150 -> 5, <=200 -> 4,
* <=300 -> 3, <=500 -> 2, slower -> 1
* False start or timeout -> 0
*/

// ---------------- Pin definitions ----------------
const int BTN = 3; // shared button
const int LED_P1 = 4; // left
const int LED_P2 = 5; // right
const int NUM_SCORE_LEDS = 6;
const int SCORE_LEDS[NUM_SCORE_LEDS] = {6, 7, 8, 9, 10, 13};
const int BUZZER_START = 11;
const int BUZZER_VICTORY = 12;

// ---------------- Game settings ----------------
const int TOTAL_ROUNDS = 5; // number of rounds
const unsigned long TIMEOUT_MS = 5000; // response window after the buzzer

// Reaction time (ms) -> points
// <=125 -> 6, <=150 -> 5, <=200 -> 4, <=300 -> 3, <=500 -> 2, else 1
const unsigned long TIME_THRESHOLDS[5] = {125, 150, 200, 300, 500};

// ---------------- Game state ----------------
int scoreP1 = 0;
int scoreP2 = 0;

// ============================================================
void setup() {
pinMode(BTN, INPUT);
pinMode(LED_P1, OUTPUT);
pinMode(LED_P2, OUTPUT);
for (int i = 0; i < NUM_SCORE_LEDS; i++) pinMode(SCORE_LEDS[i], OUTPUT);
pinMode(BUZZER_START, OUTPUT);
pinMode(BUZZER_VICTORY, OUTPUT);

Serial.begin(9600);
randomSeed(analogRead(A0));

Serial.println(F("=== Reaction Time Game 反应速度对战 ==="));
startupAnimation();
}

// ============================================================
void loop() {
scoreP1 = 0;
scoreP2 = 0;

for (int round = 1; round <= TOTAL_ROUNDS; round++) {
Serial.print(F("--- Round 第 "));
Serial.print(round);
Serial.println(F(" 轮 ---"));

// Play P1's turn and score it immediately.
unsigned long timeP1 = playTurn(1);
int pointsP1 = (timeP1 < TIMEOUT_MS) ? timeToPoints(timeP1) : 0;
scoreP1 += pointsP1;
reportPoints(1, timeP1, pointsP1);
delay(1000);

// Play P2's turn and score it immediately.
unsigned long timeP2 = playTurn(2);
int pointsP2 = (timeP2 < TIMEOUT_MS) ? timeToPoints(timeP2) : 0;
scoreP2 += pointsP2;
reportPoints(2, timeP2, pointsP2);

// Flash the faster player's LED twice.
if (timeP1 < timeP2) flashPlayerLED(1, 2);
else if (timeP2 < timeP1) flashPlayerLED(2, 2);

Serial.print(F("Total 总分 P1: ")); Serial.print(scoreP1);
Serial.print(F(" P2: ")); Serial.println(scoreP2);
delay(1500);
}

announceWinner();

Serial.println(F("Press the button to play again 按按钮再来一局"));
waitForPress();
delay(500);
}

// ============================================================
// Run one player's turn and return the reaction time in milliseconds.
// A false start or timeout returns TIMEOUT_MS and earns zero points.
unsigned long playTurn(int player) {
int led = (player == 1) ? LED_P1 : LED_P2;

Serial.print(F("P")); Serial.print(player);
Serial.println(F("'s turn 的回合"));

// 1. Signal the active player with three LED flashes and short beeps.
for (int i = 0; i < 3; i++) {
digitalWrite(led, HIGH);
tone(BUZZER_START, 600, 80);
delay(180);
digitalWrite(led, LOW);
delay(180);
}

// Wait until the button is released.
while (digitalRead(BTN) == HIGH) delay(10);

// 2. Wait a random 2-5 seconds while checking for a false start.
unsigned long waitTime = random(2000, 5001);
unsigned long waitStart = millis();
while (millis() - waitStart < waitTime) {
if (digitalRead(BTN) == HIGH) {
Serial.println(F("False start! 0 points. 抢跑!0分"));
errorBeep();
for (int i = 0; i < 5; i++) {
digitalWrite(led, HIGH); delay(100);
digitalWrite(led, LOW); delay(100);
}
while (digitalRead(BTN) == HIGH) delay(10);
return TIMEOUT_MS;
}
}

// 3. Play a loud GO signal near the buzzer's resonant frequency.
tone(BUZZER_START, 2500, 400);
unsigned long goTime = millis();

// 4. Measure reaction time.
while (millis() - goTime < TIMEOUT_MS) {
if (digitalRead(BTN) == HIGH) {
unsigned long t = millis() - goTime;
digitalWrite(led, HIGH); // instant feedback
delay(500);
digitalWrite(led, LOW);
while (digitalRead(BTN) == HIGH) delay(10);
return t;
}
}

// Timeout
Serial.println(F("Too slow! 0 points. 超时!0分"));
errorBeep();
return TIMEOUT_MS;
}

// ============================================================
// Print the result and show points on the score LEDs.
void reportPoints(int player, unsigned long t, int points) {
Serial.print(F("P")); Serial.print(player);
if (t < TIMEOUT_MS) {
Serial.print(F(" time 用时: ")); Serial.print(t);
Serial.print(F(" ms | Points 得分: ")); Serial.println(points);
} else {
Serial.println(F(" scored 0 得0分"));
}
if (points > 0) showScoreLEDs(points);
}

// ============================================================
// Convert reaction time to points; faster reactions earn more.
int timeToPoints(unsigned long t) {
if (t <= TIME_THRESHOLDS[0]) return 6; // Godlike level
if (t <= TIME_THRESHOLDS[1]) return 5;
if (t <= TIME_THRESHOLDS[2]) return 4;
if (t <= TIME_THRESHOLDS[3]) return 3;
if (t <= TIME_THRESHOLDS[4]) return 2;
return 1;
}

// ============================================================
// Fade in the first five score LEDs; the sixth flashes three times.
void showScoreLEDs(int points) {
int fadeCount = (points > 5) ? 5 : points;
for (int i = 0; i < fadeCount; i++) {
tone(BUZZER_START, 800 + i * 200, 100);
fadeInLED(SCORE_LEDS[i], 350); // fade in each LED over 350 ms
}
if (points >= 6) {
delay(200);
godlikeFlash(); // flash the sixth LED three times
}
delay(1000);
fadeOutAll((points >= 6) ? 6 : fadeCount, 500); // fade out all lit LEDs together
}

// ============================================================
// Godlike effect: flash the Pin 13 LED three times with an ascending riff.
void godlikeFlash() {
int notes[] = {1047, 1319, 1568}; // C6 E6 G6
for (int i = 0; i < 3; i++) {
digitalWrite(SCORE_LEDS[5], HIGH);
tone(BUZZER_VICTORY, notes[i], 100);
delay(150);
digitalWrite(SCORE_LEDS[5], LOW);
delay(120);
}
digitalWrite(SCORE_LEDS[5], HIGH); // stay on until fade-out
tone(BUZZER_VICTORY, 1568, 300); // finish with a sustained note
delay(300);
}

// ============================================================
// Software PWM fade-in for any digital pin.
// Use a quadratic curve so the fade appears linear to the eye.
void fadeInLED(int pin, int durationMs) {
const int steps = 40;
const long periodUs = 2000; // 2 ms period (~500 Hz, no visible flicker)
for (int s = 1; s <= steps; s++) {
long onUs = periodUs * s * s / ((long)steps * steps);
unsigned long stepEnd = millis() + durationMs / steps;
while (millis() < stepEnd) {
if (onUs > 0) {
digitalWrite(pin, HIGH);
delayMicroseconds(onUs);
}
digitalWrite(pin, LOW);
delayMicroseconds(periodUs - onUs);
}
}
digitalWrite(pin, HIGH); // finish at full brightness
}

// ============================================================
// Fade out the first N score LEDs together.
void fadeOutAll(int count, int durationMs) {
const int steps = 40;
const long periodUs = 2000;
for (int s = steps; s >= 0; s--) {
long onUs = periodUs * s * s / ((long)steps * steps);
unsigned long stepEnd = millis() + durationMs / steps;
while (millis() < stepEnd) {
if (onUs > 0) {
for (int i = 0; i < count; i++) digitalWrite(SCORE_LEDS[i], HIGH);
delayMicroseconds(onUs);
}
for (int i = 0; i < count; i++) digitalWrite(SCORE_LEDS[i], LOW);
delayMicroseconds(periodUs - onUs);
}
}
for (int i = 0; i < NUM_SCORE_LEDS; i++) digitalWrite(SCORE_LEDS[i], LOW);
}

// ============================================================
void flashPlayerLED(int player, int times) {
int led = (player == 1) ? LED_P1 : LED_P2;
for (int i = 0; i < times; i++) {
digitalWrite(led, HIGH); delay(200);
digitalWrite(led, LOW); delay(200);
}
}

// ============================================================
void announceWinner() {
Serial.println(F("=== FINAL 终局 ==="));
Serial.print(F("P1: ")); Serial.print(scoreP1);
Serial.print(F(" P2: ")); Serial.println(scoreP2);

if (scoreP1 == scoreP2) {
Serial.println(F("It's a tie! 平局!"));
for (int i = 0; i < 5; i++) {
digitalWrite(LED_P1, HIGH); digitalWrite(LED_P2, HIGH);
setAllScoreLEDs(HIGH); delay(250);
digitalWrite(LED_P1, LOW); digitalWrite(LED_P2, LOW);
setAllScoreLEDs(LOW); delay(250);
}
return;
}

int winner = (scoreP1 > scoreP2) ? 1 : 2;
Serial.print(F("WINNER 获胜者: P")); Serial.println(winner);

int winnerLED = (winner == 1) ? LED_P1 : LED_P2;

int melody[] = {523, 659, 784, 1047, 784, 1047}; // C5 E5 G5 C6 G5 C6
int duration[] = {150, 150, 150, 300, 150, 500};

for (int n = 0; n < 6; n++) {
digitalWrite(winnerLED, HIGH);
setAllScoreLEDs(HIGH);
tone(BUZZER_VICTORY, melody[n], duration[n]);
delay(duration[n]);
digitalWrite(winnerLED, LOW);
setAllScoreLEDs(LOW);
delay(50);
}

for (int i = 0; i < 6; i++) {
digitalWrite(winnerLED, HIGH); setAllScoreLEDs(HIGH); delay(150);
digitalWrite(winnerLED, LOW); setAllScoreLEDs(LOW); delay(150);
}
}

// ============================================================
// Helper functions
void setAllScoreLEDs(int state) {
for (int i = 0; i < NUM_SCORE_LEDS; i++) digitalWrite(SCORE_LEDS[i], state);
}

void errorBeep() {
tone(BUZZER_START, 200, 400);
delay(400);
}

void waitForPress() {
while (digitalRead(BTN) == LOW) delay(10);
}

void startupAnimation() {
for (int i = 0; i < NUM_SCORE_LEDS; i++) {
digitalWrite(SCORE_LEDS[i], HIGH); delay(100);
}
digitalWrite(LED_P1, HIGH); digitalWrite(LED_P2, HIGH);
tone(BUZZER_START, 880, 200);
delay(400);
digitalWrite(LED_P1, LOW); digitalWrite(LED_P2, LOW);
setAllScoreLEDs(LOW);
}



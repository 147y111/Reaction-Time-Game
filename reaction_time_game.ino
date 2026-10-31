/*
* Two-Player Reaction Time Game v12
* (Turn-based, Pin-13 godlike triple-flash, PWM fade)
* 双人反应速度对战游戏 v12(轮流制,6个分数LED,PWM渐亮,超神专属音效)
* Board: Arduino Uno
* Button wired to 5V with 10k pull-down resistor (pressed = HIGH)
* 按钮接5V + 10kΩ下拉电阻(按下 = 高电平)
*
* Wiring 接线:
* Button (shared 共用) -> Pin 3
* LED P1 (left 左) -> Pin 4 LED P2 (right 右) -> Pin 5
* Score LEDs 分数LED -> Pin 6,7,8,9,10,13 (left to right 从左到右)
* Start buzzer -> Pin 11 Victory buzzer -> Pin 12
*
* Scoring 计分 (your own reaction time 自己的反应时间):
* <=125ms -> 6 pts (godlike 超神), <=150 -> 5, <=200 -> 4,
* <=300 -> 3, <=500 -> 2, slower -> 1
* False start / timeout 抢跑或超时 -> 0
*/

// ---------------- Pin definitions 引脚定义 ----------------
const int BTN = 3; // shared button 共用按钮
const int LED_P1 = 4; // left 左
const int LED_P2 = 5; // right 右
const int NUM_SCORE_LEDS = 6;
const int SCORE_LEDS[NUM_SCORE_LEDS] = {6, 7, 8, 9, 10, 13};
const int BUZZER_START = 11;
const int BUZZER_VICTORY = 12;

// ---------------- Game settings 游戏设置 ----------------
const int TOTAL_ROUNDS = 5; // number of rounds 总轮数
const unsigned long TIMEOUT_MS = 5000; // press window after buzzer 响后按键时限

// Your own reaction time (ms) -> points 自己的反应时间(毫秒) -> 得分
// <=125 -> 6 pts, <=150 -> 5, <=200 -> 4, <=300 -> 3, <=500 -> 2, else 1
const unsigned long TIME_THRESHOLDS[5] = {125, 150, 200, 300, 500};

// ---------------- Game state 游戏状态 ----------------
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

// P1's turn: play, then score immediately P1回合:玩完立刻计分
unsigned long timeP1 = playTurn(1);
int pointsP1 = (timeP1 < TIMEOUT_MS) ? timeToPoints(timeP1) : 0;
scoreP1 += pointsP1;
reportPoints(1, timeP1, pointsP1);
delay(1000);

// P2's turn: play, then score immediately P2回合:玩完立刻计分
unsigned long timeP2 = playTurn(2);
int pointsP2 = (timeP2 < TIMEOUT_MS) ? timeToPoints(timeP2) : 0;
scoreP2 += pointsP2;
reportPoints(2, timeP2, pointsP2);

// Round-winner cue: faster player's LED flashes 本轮更快者LED闪两下
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
// One player's turn. Returns reaction time in ms (TIMEOUT_MS = failed).
// 单人回合,返回反应时间(毫秒);抢跑或超时返回TIMEOUT_MS(记0分)
unsigned long playTurn(int player) {
int led = (player == 1) ? LED_P1 : LED_P2;

Serial.print(F("P")); Serial.print(player);
Serial.println(F("'s turn 的回合"));

// 1. "Your turn" cue: this player's LED flashes 3 times + short beeps
// 轮次提示:该玩家的LED闪3下配短滴声
for (int i = 0; i < 3; i++) {
digitalWrite(led, HIGH);
tone(BUZZER_START, 600, 80);
delay(180);
digitalWrite(led, LOW);
delay(180);
}

// Wait until button is released 确保按钮已松开
while (digitalRead(BTN) == HIGH) delay(10);

// 2. Random wait (2-5 s) with false-start detection
// 随机等待2-5秒,检测抢跑
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

// 3. GO signal: loud beep near buzzer resonance 开始信号(共振频率更响)
tone(BUZZER_START, 2500, 400);
unsigned long goTime = millis();

// 4. Measure reaction time 测反应时间
while (millis() - goTime < TIMEOUT_MS) {
if (digitalRead(BTN) == HIGH) {
unsigned long t = millis() - goTime;
digitalWrite(led, HIGH); // instant feedback 即时反馈
delay(500);
digitalWrite(led, LOW);
while (digitalRead(BTN) == HIGH) delay(10);
return t;
}
}

// Timeout 超时
Serial.println(F("Too slow! 0 points. 超时!0分"));
errorBeep();
return TIMEOUT_MS;
}

// ============================================================
// Print result and show points on the score LEDs
// 打印成绩并在分数LED上显示得分
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
// Your own time -> points: faster = more 越快分越高
int timeToPoints(unsigned long t) {
if (t <= TIME_THRESHOLDS[0]) return 6; // godlike 超神档
if (t <= TIME_THRESHOLDS[1]) return 5;
if (t <= TIME_THRESHOLDS[2]) return 4;
if (t <= TIME_THRESHOLDS[3]) return 3;
if (t <= TIME_THRESHOLDS[4]) return 2;
return 1;
}

// ============================================================
// Score LEDs: first 5 fade in; the 6th point makes the Pin-13 LED flash 3x
// 分数LED:前5个渐亮;第6分由Pin 13的灯单独快闪3下
void showScoreLEDs(int points) {
int fadeCount = (points > 5) ? 5 : points;
for (int i = 0; i < fadeCount; i++) {
tone(BUZZER_START, 800 + i * 200, 100);
fadeInLED(SCORE_LEDS[i], 350); // each fades in over 350ms 每个用350ms渐亮
}
if (points >= 6) {
delay(200);
godlikeFlash(); // 6th LED flashes 3x 第六灯快闪3下
}
delay(1000);
fadeOutAll((points >= 6) ? 6 : fadeCount, 500); // all fade out together 一起渐灭
}

// ============================================================
// Godlike: Pin-13 LED flashes 3 times with ascending riff, then stays on
// 超神特效:Pin 13的灯快闪3下配上行音效,闪完保持亮
void godlikeFlash() {
int notes[] = {1047, 1319, 1568}; // C6 E6 G6
for (int i = 0; i < 3; i++) {
digitalWrite(SCORE_LEDS[5], HIGH);
tone(BUZZER_VICTORY, notes[i], 100);
delay(150);
digitalWrite(SCORE_LEDS[5], LOW);
delay(120);
}
digitalWrite(SCORE_LEDS[5], HIGH); // stay on until fade-out 保持亮到渐灭
tone(BUZZER_VICTORY, 1568, 300); // hold note 收尾长音
delay(300);
}

// ============================================================
// Software PWM fade-in on any pin 任意引脚软件PWM渐亮
// Quadratic curve so it LOOKS linear to the eye 二次曲线让肉眼看起来均匀
void fadeInLED(int pin, int durationMs) {
const int steps = 40;
const long periodUs = 2000; // 2ms period (~500Hz, no flicker) 无闪烁
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
digitalWrite(pin, HIGH); // end fully on 结束保持全亮
}

// ============================================================
// Fade out the first N score LEDs together 前N个分数LED一起渐灭
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
// Helpers 辅助函数
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

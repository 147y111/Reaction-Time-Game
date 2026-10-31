/*
 * Two-Player Reaction Time Game  双人反应速度对战游戏
 * Board: Arduino Uno
 *
 * Hardware 硬件:
 *   - 2 buttons  两个按钮        -> Pin 2 (P1), Pin 3 (P2), other leg to GND (INPUT_PULLUP)
 *   - 2 player LEDs 玩家LED     -> Pin 4 (P1), Pin 5 (P2), through 220 ohm resistor to GND
 *   - 5 score LEDs 分数LED      -> Pin 6,7,8,9,10 (left to right 从左到右)
 *   - Start buzzer 开始蜂鸣器    -> Pin 11 (passive buzzer 无源蜂鸣器)
 *   - Victory buzzer 胜利蜂鸣器  -> Pin 12 (passive buzzer 无源蜂鸣器)
 *
 * Rules 规则:
 *   1. Random wait, then start buzzer beeps  随机等待后蜂鸣器响
 *   2. Both players press their button ASAP  两位玩家尽快按下自己的按钮
 *   3. Faster player wins the round          更快的玩家赢下本轮
 *   4. Points (1-5) based on winning margin  按领先时间差打分(1-5分)
 *   5. Winner's LED flashes, then score LEDs light up left-to-right
 *      赢家LED闪烁,然后分数LED从左到右依次点亮
 *   6. After TOTAL_ROUNDS rounds, higher total score wins:
 *      winner LED flashes + victory melody + all 5 score LEDs flash together
 *      打满设定轮数后总分高者获胜:赢家LED闪烁+胜利音效+五个分数LED齐闪
 */

// ---------------- Pin definitions 引脚定义 ----------------
const int BTN_P1 = 2;
const int BTN_P2 = 3;
const int LED_P1 = 4;
const int LED_P2 = 5;
const int SCORE_LEDS[5] = {6, 7, 8, 9, 10};  // left to right 从左到右
const int BUZZER_START   = 11;
const int BUZZER_VICTORY = 12;

// ---------------- Game settings 游戏设置 ----------------
const int TOTAL_ROUNDS = 5;              // number of rounds 总轮数
const unsigned long TIMEOUT_MS = 3000;   // max wait for a press 按键超时时间

// Winning-margin thresholds (ms) -> points 领先时间差阈值(毫秒) -> 得分
// margin >= 400 -> 5 pts, >= 250 -> 4, >= 150 -> 3, >= 50 -> 2, else 1
const unsigned long MARGIN_THRESHOLDS[4] = {400, 250, 150, 50};

// ---------------- Game state 游戏状态 ----------------
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
  randomSeed(analogRead(A0));  // unconnected pin for random noise 悬空引脚做随机种子

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
    playRound();
    delay(1500);  // pause between rounds 轮间停顿
  }

  announceWinner();

  // Wait for any button to restart the game 按任意按钮重新开始
  Serial.println(F("Press any button to play again 按任意键再来一局"));
  waitForAnyPress();
  delay(500);
}

// ============================================================
// One round of the game 进行一轮游戏
void playRound() {
  // 1. Random wait (2-5 s). Watch for false starts during the wait.
  //    随机等待2-5秒,期间检测抢跑
  unsigned long waitTime = random(2000, 5001);
  unsigned long waitStart = millis();
  while (millis() - waitStart < waitTime) {
    if (digitalRead(BTN_P1) == LOW) { handleFalseStart(1); return; }
    if (digitalRead(BTN_P2) == LOW) { handleFalseStart(2); return; }
  }

  // 2. GO signal: start buzzer beeps 开始信号:蜂鸣器响
  tone(BUZZER_START, 1000, 200);
  unsigned long goTime = millis();

  // 3. Record each player's reaction time 记录两位玩家的反应时间
  unsigned long timeP1 = 0, timeP2 = 0;  // 0 = not pressed yet 尚未按下
  bool p1Done = false, p2Done = false;

  while ((!p1Done || !p2Done) && (millis() - goTime < TIMEOUT_MS)) {
    if (!p1Done && digitalRead(BTN_P1) == LOW) {
      timeP1 = millis() - goTime;
      p1Done = true;
      digitalWrite(LED_P1, HIGH);  // instant feedback 即时反馈
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

  // 4. Decide the round winner 判定本轮胜者
  if (!p1Done && !p2Done) {
    Serial.println(F("Nobody pressed! No points. 没人按,双方不得分"));
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
    Serial.println(F("Exact tie! No points. 完全平局,双方不得分"));
    errorBeep();
    return;
  }

  // 5. Convert margin to points 根据领先差值换算得分
  int points = marginToPoints(margin);
  if (winner == 1) scoreP1 += points; else scoreP2 += points;

  Serial.print(F("Winner 胜者: P")); Serial.print(winner);
  Serial.print(F(" | Margin 领先: ")); Serial.print(margin);
  Serial.print(F(" ms | Points 得分: ")); Serial.println(points);
  Serial.print(F("Total 总分  P1: ")); Serial.print(scoreP1);
  Serial.print(F("  P2: ")); Serial.println(scoreP2);

  // 6. Flash winner's LED 闪烁胜者的LED
  flashPlayerLED(winner, 3);

  // 7. Show points on score LEDs, left to right 分数LED从左到右依次点亮
  showScoreLEDs(points);
}

// ============================================================
// False start: opponent gets 1 point 抢跑:对手得1分
void handleFalseStart(int player) {
  Serial.print(F("P")); Serial.print(player);
  Serial.println(F(" false start! Opponent +1. 抢跑!对手得1分"));

  errorBeep();
  // Flash the offender's LED quickly as a warning 快速闪烁抢跑者LED作警告
  int led = (player == 1) ? LED_P1 : LED_P2;
  for (int i = 0; i < 5; i++) {
    digitalWrite(led, HIGH); delay(100);
    digitalWrite(led, LOW);  delay(100);
  }

  if (player == 1) scoreP2 += 1; else scoreP1 += 1;
  showScoreLEDs(1);

  // Wait until both buttons released 等待双方松开按钮
  while (digitalRead(BTN_P1) == LOW || digitalRead(BTN_P2) == LOW) delay(10);
}

// ============================================================
// Margin (ms) -> 1-5 points 领先差值换算得分
int marginToPoints(unsigned long margin) {
  if (margin >= MARGIN_THRESHOLDS[0]) return 5;
  if (margin >= MARGIN_THRESHOLDS[1]) return 4;
  if (margin >= MARGIN_THRESHOLDS[2]) return 3;
  if (margin >= MARGIN_THRESHOLDS[3]) return 2;
  return 1;
}

// ============================================================
// Light up N score LEDs left to right, hold, then clear
// 分数LED从左到右依次点亮N个,保持后熄灭
void showScoreLEDs(int points) {
  for (int i = 0; i < points; i++) {
    digitalWrite(SCORE_LEDS[i], HIGH);
    tone(BUZZER_START, 800 + i * 200, 100);  // rising pitch 音调递增
    delay(300);
  }
  delay(1000);
  for (int i = 0; i < 5; i++) digitalWrite(SCORE_LEDS[i], LOW);
}

// ============================================================
// Flash a player's LED 闪烁某位玩家的LED
void flashPlayerLED(int player, int times) {
  int led = (player == 1) ? LED_P1 : LED_P2;
  for (int i = 0; i < times; i++) {
    digitalWrite(led, HIGH); delay(200);
    digitalWrite(led, LOW);  delay(200);
  }
}

// ============================================================
// Final winner celebration 终局胜利庆祝
void announceWinner() {
  Serial.println(F("=== FINAL 终局 ==="));
  Serial.print(F("P1: ")); Serial.print(scoreP1);
  Serial.print(F("  P2: ")); Serial.println(scoreP2);

  if (scoreP1 == scoreP2) {
    Serial.println(F("It's a tie! 平局!"));
    // Both LEDs + all score LEDs flash 双方LED和分数LED一起闪
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_P1, HIGH); digitalWrite(LED_P2, HIGH);
      setAllScoreLEDs(HIGH); delay(250);
      digitalWrite(LED_P1, LOW);  digitalWrite(LED_P2, LOW);
      setAllScoreLEDs(LOW);  delay(250);
    }
    return;
  }

  int winner = (scoreP1 > scoreP2) ? 1 : 2;
  Serial.print(F("WINNER 获胜者: P")); Serial.println(winner);

  int winnerLED = (winner == 1) ? LED_P1 : LED_P2;

  // Victory melody on the second buzzer + winner LED + all score LEDs flash together
  // 第二个蜂鸣器播放胜利音效 + 赢家LED + 五个分数LED齐闪
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

  // A few extra celebration flashes 再多闪几下庆祝
  for (int i = 0; i < 6; i++) {
    digitalWrite(winnerLED, HIGH); setAllScoreLEDs(HIGH); delay(150);
    digitalWrite(winnerLED, LOW);  setAllScoreLEDs(LOW);  delay(150);
  }
}

// ============================================================
// Helpers 辅助函数
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

// Quick LED sweep at power-on 开机LED流水灯自检
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

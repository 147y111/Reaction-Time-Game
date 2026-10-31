# Reaction Time Game

**Platform:** Arduino Uno · **Development Environment:** Arduino IDE · **Language:** Arduino/C++

Reaction Time Game is a two-player physical arcade game developed during Cooper Union Summer STEM 2026. Players take turns using a shared button and try to respond as quickly as possible after an unpredictable audio signal. The Arduino measures each reaction time, converts it into points, tracks both players across five rounds, and announces the final winner through LEDs, sound, and the Serial Monitor.

The project was created to explore how timing, physical interaction, and immediate feedback can make an electronics project engaging. Before every turn, the active player's LED flashes to show who should play. The Arduino then waits for a random interval of two to five seconds. Pressing before the signal counts as a false start and earns zero points; failing to respond within five seconds also earns zero points.

<p align="center">
  <img src="media/reaction-time-game-prototype.jpg" alt="Reaction Time Game Arduino prototype" width="700">
</p>

Reaction times are scored on six levels. A response of 500 milliseconds or less earns two points, and faster responses receive progressively higher scores. During testing, some players reached the original highest level, so the scoring system was improved with a sixth **Godlike** level for reactions of 125 milliseconds or less. Reaching this level activates a special effect: the sixth score LED flashes three times while the buzzer plays an ascending C6–E6–G6 sound sequence. This addition made exceptional results more recognizable and gave players a new target to achieve.

The repository preserves both major iterations to show how the project developed. [Version 1](versions/v1_initial_two-button_game.ino) used two separate buttons and awarded one to five points based on the difference between the players' reaction times. [Version 2](versions/v2_godlike_shared-button_game.ino) changed the game to a turn-based design with one shared button, scored each player's individual reaction time, and introduced the sixth Godlike level. The main [reaction_time_game.ino](reaction_time_game.ino) file contains the current version.

The prototype uses an Arduino Uno, one shared push button, two player LEDs, six score LEDs, two passive buzzers, resistors, breadboards, and jumper wires. It was developed in Arduino IDE using Arduino/C++. The program demonstrates digital input, millisecond-based timing, randomized delays, false-start detection, timeout handling, reaction-time scoring, software PWM LED fading, tone generation, serial debugging, round management, and coordinated visual and audio feedback.

The current version completes five turn-based rounds, calculates each player's score immediately after a turn, identifies the faster player in every round, displays earned points through the score LEDs, and plays a final victory sequence. After the results are announced, the shared button can start a new match. Future improvements could include cleaner permanent wiring, a custom enclosure, adjustable difficulty, saved reaction-time records, and a display for scores and instructions.

AI tools were used to assist with parts of the coding process and documentation. My responsibilities included debugging and refining the code, designing and completing the circuit wiring, assembling the breadboard prototype, integrating the physical components, testing the hardware, and troubleshooting the complete system. The gameplay decisions and final implementation were completed and reviewed by me.

The project demonstration video will be added after compression is complete.

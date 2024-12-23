 **Introduction**: This is a game created for my custom labratory project for my Embedded Systems class. We worked on this project for 3 weeks utilizing the arduino uno kit, platformio.ini, and C code to construct and program the game.
 I created a game inspired by Buckshot Roulette in which it follows a turn based format in which two players take turns with a blaster that has a certain number of live and dummy rounds and both players take turns 
 at shooting each other or themselves. Shooting oneself incentivises risk taking because if it was dummy then it would still stay the current player’s turn, essentially skipping the other player's turn, but if the current player shot the opponent 
 with a dummy, it would then just move to the opponent's turn. Regardless if a player shot themselves or their opponent, if it was a live round, the player receiving the shot would lose health. The game eventually 
 ends when one player’s health hits 0 and there's also the addition of items and rounds in which the number of live and dummy shots increase in the blaster. This is the basic premise of the game and I have completed 
 almost all the complexities and features that I wanted to add to the project.

**User Guide**: The way that the user interacts with this game is to first begin/end the game with the IR remotes power button, from here the user will see flashing numbers and letter that indicate the number of 
dummy and live rounds on the LED screen and shortly after it will also display the starting player with either an A or B (you can decide whose A or B). After the flashing, you now have the option to either press 
the one of the two buttons next to the LED screen, left being shoot yourself and right being shoot the opponent. You’ll each take turns until all the live rounds in the balster have depleted in which it will 
start a new round with a new set of dummy/live rounds and display the current players turn, or until one of the player’s healths have decreased to 0 which will then flash the winner. At the same time, after the 
first round has finished, both players have the option to click the vol+ button in which all 3 LEDS will light up and you can either choose 1, 2, or 3 on the IR remote which will activate a specific item (double 
damage, increase health, skip opponent's turn respectively for 1-3) and this can only be done once a round after the first. There will also be music playing throughout the game. 

**Hardware Components Used**: 
  * Breadboard jumper wires
  * Remote control
  * IR receiver
  * Passive buzzer
  * 4 digit 7 segment display
  * x2 buttons 
  * Shift register
  * x1 red LED
  * x1 blue LED
  * x1 green LED
  * x5 220 Ohm resistors

**Software Libraries Used**: 
  * #include “timerISR.h”: Library used to integrate functionality of the timerISR and create a concurrent sync SM for our project.
  * #include “helper.h”: Used this library for functions like SetBit and GetBit.
  * #include “irAVR.h”: Library used for IR remote getting the input from the receiver.
  * #include “serialATmega.h”:Library to print out info onto the terminal for debugging
  * #include “pitches.h”: contains all the pitches used for the passive buzzer to create different sounds for music.

**Wiring Diagram**:

![Wiring Diagram](https://github.com/user-attachments/assets/5f6c128b-1ee6-4cb9-91b9-0ab7a5728403)

**Demo Video**: https://www.youtube.com/watch?v=Osk4n7E4cj0&t=1s 









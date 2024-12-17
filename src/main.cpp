#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "irAVR.h"
#include "serialATmega.h"
#include <stdio.h> 
#include <stdlib.h> 
#include "pitches.h" //Frequencies for passive buzzer

#define NUM_TASKS 6 //TODO: Change to the number of tasks being used

//GLOBAL VARIABLES
decode_results results;
//Health for both players start at 3, and at the beginning of each round, it will increase the health
int healthA = 3;
int healthB = 3;
//The number of live and dummy rounds will be reassigned at creation
int dummy = 0;
int live = 0;

//Max number of ammo in the weapon
int max = 5;

bool gameStart = false; 
bool playerA = false; //In game it is A's turn
bool playerB = false; //In game it is b's turn
bool gameOver = false;
unsigned long flashStart = 0;
int chance = 0;
int rounds = 1;
int cnt = 0;
//MAKE SURE TO ADD FUNCTIONALITY WHERE IF ONE PERSON TAKES DAMAGE MOVE TO NEXT ROUND(MAYBE?)

//Note that when shiftOut for all shift register Q1 to Q7, Q1 starts at bit 1 not bit 0 and Q2 starts at bit 2 not bit 1 and etc.
//The numbers are all 8 bits so slightly different from the previous nums but bit 0 is always 0 cuz it starts at bit 1 for the shift register
int newNums[17] = {0b01111110, 0b00001100, 0b10110110, 0b10011110, 0b11001100, 0b11011011, 0b11111010, 0b00001110, 0b11111110, 0b11001110, 0b11101110, 0b11111000, 0b01110010, 0b10111100, 0b11110010, 0b11100010, 0b01110000 }; 
//0 1 2 3 4 5 6 7 8 9 A  b  C  d  E  F L
//Adding extra letter L

//Used for passive buzzer
int buzzerTop = 0;
int noteCnt = 0;
int gameActiveMusic[] = {
  NOTE_CS5, NOTE_FS4, NOTE_FS4, NOTE_CS5, NOTE_FS4, NOTE_FS4, NOTE_CS5, NOTE_FS4, NOTE_D5, NOTE_FS4, NOTE_CS5, NOTE_FS4, NOTE_FS4, NOTE_CS5
};

int durationActiveMusic[] = {
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
};

int d = 0;

//Global Varaiables for items
int selectedItem = 0;
bool selectingItem = false;
int bonusDamage = 0;
bool skipOppTurn = false;

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>

const unsigned long LED_PERIOD = 5;
const unsigned long IR_PERIOD = 1000;
const unsigned long LEFT_PERIOD = 250;
const unsigned long RIGHT_PERIOD = 250;
const unsigned long TURN_PERIOD = 100;
const unsigned long PASSIVE_PERIOD = 100;
const unsigned long GCD_PERIOD = 1;//TODO:Set the GCD Period

task tasks[NUM_TASKS]; // declared task array with 5 tasks

//TODO: Declare your tasks' function and their states here
enum LED_state {DISPLAY_INIT, DISPLAY_FIRST, DISPLAY_SECOND, DISPLAY_THIRD, DISPLAY_FOURTH};
enum IR_state {GAME_START, GAME_END};
enum Left_state {LEFT_OFF_RELEASE, LEFT_ON_PRESS, LEFT_ON_RELEASE};
enum Right_state {RIGHT_OFF_RELEASE, RIGHT_ON_PRESS, RIGHT_ON_RELEASE};
enum Turn_state {TURN_INIT, PLAYERA, PLAYERB};
enum PassiveBuzzer_state {SOUND_OFF, SOUND_ON};

//TODO: Create your tick functions for each task
int Tick_LED(int state);
int Tick_IR(int state);
int Tick_LeftButton(int state);
int Tick_RightButton(int state);
int Tick_Turn(int state);
int Tick_PassiveBuzzer(int state);

void TimerISR() {
	// for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
	// 	if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
	// 		tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
	// 		tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
	// 	}
	// 	tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	// }
  TimerFlag = 1;
}

void clearShiftRegister() {
    //Clear Shift Register after every use
  for(int i = 7; i >= 0; --i) {
    PORTD = SetBit(PORTD, 4, 0);
    PORTD = SetBit(PORTD, 3, 0);
    PORTD = SetBit(PORTD, 4, 1);
  }

  //Set RCLK to high to be able to send values to output
  PORTD = SetBit(PORTD, 5, 1);
  PORTD = SetBit(PORTD, 5, 0);
}


void OutputNums(int value) {
  //Used for shift register
  // SRCLK = 4;      // Latch pin of 74HC595 is connected to Digital pin 4, pin used to allow data to be placed into memory, it's also active low
  // RCLK = 5;      // Clock pin of 74HC595 is connected to Digital pin 5, data in memory is copied onto output
  // SER = 3;       // Data pin of 74HC595 is connected to Digital pin 3, stores value of 0 or 1
  // SRCLR = 2      //Clear pin that is active low, clears data in output
  
  clearShiftRegister();
  for(int i = 7; i >= 0; --i) {
    //This toggles to shift data into memory
    //Set SRCLK to low to allow shift to memory
    PORTD = SetBit(PORTD, 4, 0);
    if(((newNums[value] >> i) & 0x01) == 1) { //Shfit bits over to get bit at i position and store the value into memory
      PORTD = SetBit(PORTD, 3, 1);
    }
    else {
      PORTD = SetBit(PORTD, 3, 0);
    }
    //sets SRCLK to high and doesn't allow shift to memory
    PORTD = SetBit(PORTD, 4, 1);
  }
  
  //Set RCLK to high to be able to send values to output
  PORTD = SetBit(PORTD, 5, 1);
  PORTD = SetBit(PORTD, 5, 0);
}

int main(void) {
    //TODO: initialize all your inputs and ouputs
    DDRC = 0b00000000;
    PORTC = 0b11111111;

    DDRD = 0b11111111;
    PORTD = 0b00000000;

    DDRB = 0b11111111;
    PORTB = 0b00000000;

    //TODO: Initialize the buzzer timer/pwm(timer1) to be used 
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8

    //TODO: Initialize the IR remote 
    IRinit(&DDRD, &PIND, 7);

    //TODO: Initialize tasks here
    // e.g. 
    // tasks[0].period = ;
    // tasks[0].state = ;
    // tasks[0].elapsedTime = ;
    // tasks[0].TickFct = ;

    unsigned int i = 0;

    tasks[i].period = IR_PERIOD;
    tasks[i].state = GAME_END;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_IR;
    ++i;

    tasks[i].period = TURN_PERIOD;
    tasks[i].state = TURN_INIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Turn;
    ++i;

    tasks[i].period = LED_PERIOD;
    tasks[i].state = DISPLAY_INIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_LED;
    ++i;

    tasks[i].period = LEFT_PERIOD;
    tasks[i].state = LEFT_OFF_RELEASE;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_LeftButton;
    ++i;

    tasks[i].period = RIGHT_PERIOD;
    tasks[i].state = RIGHT_OFF_RELEASE;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_RightButton;
    ++i;

    tasks[i].period = PASSIVE_PERIOD;
    tasks[i].state = SOUND_ON;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_PassiveBuzzer;

    serial_init(9600);
    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
      for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
        if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
          tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
          tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
        }
        tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
      }
      while(!TimerFlag){}
      TimerFlag = 0;
    }

    return 0;
}

int Tick_IR(int state) {
  //Transitions
  switch(state) {
    case GAME_END:  
      if(IRdecode(&results)) { //decode result for IR remote
        if(results.value == 16753245) {
          gameStart = true;
          state = GAME_START;
          //Set dummy and live rounds
          //First round will have starting rounds of 3 dummy, 1 live, and as rounds progress we'll add more shots to it to increase fairness
          healthA = 3;
          healthB = 3;
          max = 4;
          dummy = 3;
          live = 1;
          cnt = live;
          gameOver = false;
          flashStart = 0;
          rounds = 1;
          serial_println("Game Starting");
        }
        IRresume(); //Resets result value
      }
      else {
        gameStart = false;
        state = GAME_END;
      }
      PORTB = SetBit(PORTB, 0, 0);
      PORTD = SetBit(PORTD, 6, 0);
      PORTD = SetBit(PORTD, 2, 0);
      break;

    case GAME_START:
      if(IRdecode(&results)) { //decodes results for IR remote
        if(results.value == 16753245) {
          gameStart = false;
          state = GAME_END;
          dummy = 0;
          live = 0;
          cnt = 0;
          rounds = 0;
          serial_println("Game Ending");
        }
        else if(results.value == 16736925 && rounds > 1) { //Now selecting item, by pressing vol+ button, items will apply to whatever players turn it is
          PORTD = SetBit(PORTD, 2, 1);
          PORTD = SetBit(PORTD, 6, 1);
          PORTB = SetBit(PORTB, 0, 1);
          selectingItem = true;
        }
        else if(results.value == 16724175 && selectingItem && rounds > 1) { //Chose option 1, double damage next shot
          PORTD = SetBit(PORTD, 6, 0);
          PORTD = SetBit(PORTD, 2, 0);
          serial_println("choosing item 1");
          selectedItem = 1;
          selectingItem = false;
        }
        else if(results.value == 16718055 && selectingItem && rounds > 1) { //Chose option 2, increase current player health by 1
          PORTD = SetBit(PORTD, 2, 0);
          PORTB = SetBit(PORTB, 0, 0);
          serial_println("choosing item 2");
          selectedItem = 2;
          selectingItem = false;
        }
        else if(results.value == 16743045 && selectingItem && rounds > 1) { //Chose option 3, skip opponents turn
          PORTD = SetBit(PORTD, 6, 0);
          PORTB = SetBit(PORTB, 0, 0);
          serial_println("choosing item 3");
          selectedItem = 3;
          selectingItem = false;
        }
        IRresume(); //Resets result value
      }
      else if(healthA <= 0 || healthB <= 0) {
        gameStart = false;
        state = GAME_END;
        flashStart = 0;
        gameOver = true;
      }
      else if(live == 0) {
        //Start new round when live rounds have decreased to 0
        cnt += 1; //a simple cnt for tracking the live bullets to calculate the most optimal dummy bullets
        max += 2; //Increase total number of shots
        dummy  = max - cnt;
        live = max - dummy;
        flashStart = 0; //reset the flash to blink again 
        ++rounds;
      }
      else {
        gameStart = true;
        state = GAME_START;
      }
      break;
    default:
      state = GAME_END;
      break;
  }

  //Actions
  switch(state) {
    case GAME_END:
      // serial_println(results.value);
      selectingItem = false;
      selectedItem = 0;
      break;

    case GAME_START:
      //serial_println(results.value);
      if(selectedItem == 1) {
        bonusDamage = 1;
      }
      else if(selectedItem == 2) {
        if(playerA) {
          healthA += 1;
        }
        else if(playerB) {
          healthB += 1;
        }
        PORTD = SetBit(PORTD, 6, 0);
      }
      else if (selectedItem == 3) {
        skipOppTurn = true;
      }
      selectedItem = 0;
      break;

    default:
      break;
  }

  return state;
}

int Tick_Turn(int state) { //State machine to keep track of turns 
  //Transitions
  switch(state) {
    case TURN_INIT:
      if(gameOver) {
        state = TURN_INIT;
      }
      else {
        int turn = rand() % 2;
        if(turn == 0) {
          state = PLAYERA;
          playerA = true;
          playerB = false;
        }
        else  if(turn == 1) {
          state = PLAYERB;
          playerA = false;
          playerB = true;
        }
      }
      break;

    case PLAYERA:
      if(gameOver) {
        state = TURN_INIT;
      }
      else if(!playerA) {
        state = PLAYERB;
        playerB = true;
        playerA = false;
        serial_println("B's Turn");
      }
      else {
        state = PLAYERA;
      }
      break;

    case PLAYERB:
      if(gameOver) {
        state = TURN_INIT;
      }
      else if(!playerB) {
        state = PLAYERA;
        playerA = true;
        playerB = false;
        serial_println("A's Turn");
      }
      else {
        state = PLAYERB;
      }
      break;

    default:
      state = TURN_INIT;
      break;
  }

  //Actions
  switch(state) {
    case TURN_INIT:
      break;
    case PLAYERA:
      break;
    case PLAYERB:
      break;
    default:
      break;
  }

  return state;
}

int Tick_LED(int state) {
  //Transitions
  switch(state) {
    case DISPLAY_INIT:
      if(gameStart == true) {
        if((flashStart > 100 && flashStart < 200) || (flashStart > 300 && flashStart < 400) || (flashStart > 500 && flashStart < 600) || (flashStart > 650 && flashStart < 700) || (flashStart > 750 && flashStart < 800)) {
            state = DISPLAY_INIT;
            ++flashStart;
        }
        else {
          state = DISPLAY_FIRST;
        }
      }
      else if(gameOver == true) { //Used for gameOver state to flash message that game has ended
        if((flashStart > 100 && flashStart < 200) || (flashStart > 300 && flashStart < 400) || (flashStart > 500 && flashStart < 600) || (flashStart > 650 && flashStart < 700) || (flashStart > 750 && flashStart < 800)) {
            state = DISPLAY_INIT;
            ++flashStart;
        }
        else if(flashStart > 4000) {
          gameOver = false;
        }
        else {
          state = DISPLAY_FIRST;
        }
      }
      break;

    case DISPLAY_FIRST:
      if(gameOver == true) {
        if(flashStart > 600) { //After flashing is finished, return to first state and set gameOver to false to stop flashing
          state = DISPLAY_INIT;
          gameOver = false;
        }
        state = DISPLAY_SECOND;
      }
      else if(gameStart == false) {
        flashStart = 0;
        state= DISPLAY_INIT;
      }
      else {
        state = DISPLAY_SECOND;
      }
      break;

    case DISPLAY_SECOND:
      if(gameOver == true) {
        if(flashStart > 600) {
          state = DISPLAY_INIT;
          gameOver = false;
        }
        state = DISPLAY_THIRD;
      }
      else if(gameStart == false) {
        flashStart = 0;
        state= DISPLAY_INIT;
      }
      else {
        state = DISPLAY_THIRD;
      }
      break;

    case DISPLAY_THIRD:
      if(gameOver == true) {
        if(flashStart > 600) {
          state = DISPLAY_INIT;
          gameOver = false;
        }
        state = DISPLAY_FOURTH;
      }
      else if(gameStart == false) {
        flashStart = 0;
        state= DISPLAY_INIT;
      }
      else {
        state = DISPLAY_FOURTH;
      }
      break;

    case DISPLAY_FOURTH:
      if(gameOver == true) {
        if(flashStart > 600) {
          gameOver = false;
        }
        state = DISPLAY_INIT;
      }
      else if(gameStart == false) {
        flashStart = 0;
        state= DISPLAY_INIT;
      }
      else {
        state = DISPLAY_INIT;
      }
      break;

    default:
      state = DISPLAY_INIT;
      break;
  }

  //Actions
  switch(state) {
    case DISPLAY_INIT:
      //The digits are are low active, and to display all different digits, and simulate strobing(move through states quickly to make numbers appear)
      //serial_println(flashStart);

      PORTB = PORTB | 0x3C; //Reset bits to this 0bxx1111xx
      break;

    case DISPLAY_FIRST:
      PORTB = PORTB | 0x3C; //Reset bits to this 0bxx1111xx
      PORTB = PORTB & 0xDF; //0bxx0111xx; Display's 1000's digit
      if(flashStart < 600) {
        if(gameOver == true) {//display winner
          if(healthA > 0) {
            OutputNums(10);
          }
          else if(healthB > 0) {
            OutputNums(11);
          }
        }
        else {
          OutputNums(13);
        } 
        ++flashStart;
      }
      else if(flashStart < 850) {
        //Display who goes first
          if(playerA) {
            OutputNums(10);
          }
          else if(playerB) {
            OutputNums(11);
          }
        ++flashStart;
      }  
      else {
        OutputNums(10);
      }
      break;

    case DISPLAY_SECOND:
      PORTB = PORTB | 0x3C; //Reset bits to this 0bxx1111xx
      PORTB = PORTB & 0xEF; //0bxx1011xx; Display's 100's digit
      if(flashStart < 600) {
        if(gameOver == true) {//display winner
          if(healthA > 0) {
            OutputNums(10);
          }
          else if(healthB > 0) {
            OutputNums(11);
          }
        }
        else {
          OutputNums(dummy);
        }
        ++flashStart;
      }
      else if(flashStart < 850) {
        //Display who goes first
          if(playerA) {
            OutputNums(10);
          }
          else if(playerB) {
            OutputNums(11);
          }
        ++flashStart;
      }  
      else {
        OutputNums(healthA);
      }
      break;

    case DISPLAY_THIRD:
      PORTB = PORTB | 0x3C; //Reset bits to this 0bxx1111xx
      PORTB = PORTB & 0xF7; //0bxx1101xx; Displays 10's digit
      if(flashStart < 600) {
        if(gameOver == true) {//display winner
          if(healthA > 0) {
            OutputNums(10);
          }
          else if(healthB > 0) {
            OutputNums(11);
          }
        }
        else {
          OutputNums(16);
        }
        ++flashStart;
      }
      else if(flashStart < 850) {
        //Display who goes first
          if(playerA) {
            OutputNums(10);
          }
          else if(playerB) {
            OutputNums(11);
          }
        ++flashStart;
      }  
      else {
        OutputNums(11);
      }
      break;

    case DISPLAY_FOURTH:
      PORTB = PORTB | 0x3C; //Reset bits to this 0bxx1111xx
      PORTB = PORTB & 0xFB; //0bxx1110xx; Display's 1's digit
      if(flashStart < 600) {
        if(gameOver == true) {//display winner
          if(healthA > 0) {
            OutputNums(10);
          }
          else if(healthB > 0) {
            OutputNums(11);
          }
        }
        else {
          OutputNums(live);
        }
        ++flashStart;
      }
      else if(flashStart < 850) {
        //Display who goes first
          if(playerA) {
            OutputNums(10);
          }
          else if(playerB) {
            OutputNums(11);
          }
        ++flashStart;
      }  
      else {
        OutputNums(healthB);
      }
      break;
      
    default:
      break;
  }

  return state;
}

int Tick_LeftButton(int state) {
  //Transitions
  switch(state) {
    case LEFT_OFF_RELEASE:
      if(GetBit(PINC, 0) && gameStart) {
        state = LEFT_ON_PRESS;
      }
      else {
        state = LEFT_OFF_RELEASE;
      }
      break;

    case LEFT_ON_PRESS:
      if(!GetBit(PINC, 0)) {
        state = LEFT_ON_RELEASE;
      }
      else {
        state = LEFT_ON_PRESS;
      }
      break;

    case LEFT_ON_RELEASE:
      state = LEFT_OFF_RELEASE;
      break;

    default:
      state = LEFT_OFF_RELEASE;
      break;
  }

  //Actions
  switch(state) {
    case LEFT_OFF_RELEASE:
      break;
    case LEFT_ON_PRESS:
       break;
    case LEFT_ON_RELEASE:
      if(gameStart) {
        if(playerA || playerB) {
          chance = rand() % (live + dummy) + 1; // Gives odds based on total number of shots 
          serial_println(chance);
          if(chance <= live) { //if the chance is less than the number of live rounds, then then the player afflicted will take damaga(self)
            serial_println("Live Round fired");
            if(playerA) {
              serial_println("Decrease A health");
              healthA = healthA - 1 - bonusDamage;
            }
            else if(playerB) {
              serial_println("Decrease B health");
              healthB = healthB - 1 - bonusDamage;
            }
            --live;
            //Swtich turns if shot fired
            if(!skipOppTurn) { //If skip turn item not used, then change turns
              playerA = !playerA;
              playerB = !playerB;
            }
            else { //else still current player's turn
              skipOppTurn = false; 
              PORTD = SetBit(PORTD, 2, 0);
            }
          }
          else if(chance > live) {//If dummy round shot, then skip other player's turn and let the current player continue
            dummy -= 1;
            serial_println("Dummy round fired");
          }

          //if it was a dymmy or live round, regardless the bonus damage needs to be taken away
          bonusDamage = 0; //Reset bonusDamage after being used 
          PORTB = SetBit(PORTB, 0, 0);

          //Change the player turns after every button click
          serial_println("live rounds");
          serial_println(live);
          serial_println("dummy rounds");
          serial_println(dummy);
        }
      }
      break;
    default:
      break;
  }

  return state;
}

int Tick_RightButton(int state) {
  //Transitions
  switch(state) {
    case RIGHT_OFF_RELEASE:
      if(GetBit(PINC, 1) && gameStart) {
        state = RIGHT_ON_PRESS;
      }
      else {
        state = RIGHT_OFF_RELEASE;
      }
      break;

    case RIGHT_ON_PRESS:
      if(!GetBit(PINC, 1)) {
        state = RIGHT_ON_RELEASE;
      }
      else {
        state = RIGHT_ON_PRESS;
      }
      break;

    case RIGHT_ON_RELEASE:
      state = RIGHT_OFF_RELEASE;
      break;

    default:
      state = RIGHT_OFF_RELEASE;
      break;
  }

  //Actions
  switch(state) {
    case RIGHT_OFF_RELEASE:
      break;
    case RIGHT_ON_PRESS:
      break;
    case RIGHT_ON_RELEASE:
      if(gameStart) {
        if(playerA || playerB) {
          chance = rand() % (live + dummy) + 1; // Gives odds based on total number of shots 
          serial_println(chance);
          if(chance <= live) { //if the chance is less than the number of live rounds, then then the opponent will take damage
            serial_println("Live round fired");
            if(playerA) {
              serial_println("Decrease B health");
              healthB = healthB - 1 - bonusDamage;
            }
            else if(playerB) {
              serial_println("Decrease A health");
              healthA = healthA - 1 - bonusDamage;
            }
            --live;
          }
          else if(chance > live) {//If dummy round shot, then skip other player's turn and let the current player continue
            serial_println("Dummy round fired");
            dummy -= 1;
          }
          //Change the player turns after every turn of shooting the opponent
          //Swtich turns if shot fired
          if(!skipOppTurn) { //If skip turn item not used, then change turns
            playerA = !playerA;
            playerB = !playerB;
          }
          else { //else still current player's turn
            skipOppTurn = false; 
            PORTD = SetBit(PORTD, 2, 0);
          }

          //Since if it was a dmmy round, then regardless the bonus damage needs to be taken away
          bonusDamage = 0; //Reset bonusDamage after being used 
          PORTB = SetBit(PORTB, 0, 0);

          serial_println("live rounds");
          serial_println(live);
          serial_println("dummy rounds");
          serial_println(dummy);
        }
      }
      break;
    default:
      break;
  }

  return state;
}

int Tick_PassiveBuzzer(int state) {
  //Transitions
  switch(state) {
    case SOUND_OFF:
      if(gameStart) {
        state = SOUND_ON;
      }
      break;
    case SOUND_ON:
      if(!gameStart) {
        state = SOUND_OFF;
      }
      break;
    default:
      state = SOUND_OFF;
      break;
  }

  //Actions
  switch(state) {
    case SOUND_OFF:
      ICR1 = buzzerTop;
      OCR1A = buzzerTop; //If value is equal to ICR1, then audio doesn't play
      noteCnt = 0;
      d = 0;
      //serial_println("here");
      break;
    case SOUND_ON:
    //Equation used TOP = (fclk)/(fpwm * N) - 1
    //fclk is 16 million, fpwm is the note frequency, and N is set 8 by prescaler
      if(noteCnt < 14) {
        buzzerTop = ((16000000)/(gameActiveMusic[noteCnt] * 8)) - 1;
        if(d < durationActiveMusic[noteCnt]) {
          ++d;
        }
        else {
          d = 0;
          ++noteCnt;
        }
      }
      else {
        noteCnt = 0;
        d = 0;
      }
      ICR1 = buzzerTop;
      OCR1A = buzzerTop/2; //If value is equal to buzzerTop/2, play frequency
      break;
    default:
      break;
  }
  return state; 
}

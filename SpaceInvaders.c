// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 11/20/2018 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2018

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2018

 Copyright 2018 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "DAC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer0B.h"
#include "Timer1.h"
#include "Timer2.h"
#include "Timer3.h"

extern int shootflag;
extern int explosionflag;

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

//#define PE0	     (GPIO_PORTE_DATA_R&0x01)	//fire button
//#define PE1      (GPIO_PORTE_DATA_R&0x02)	//shield button
#define PF0			 (GPIO_PORTF_DATA_R&0x01)	//fire button
#define PF4			 (GPIO_PORTF_DATA_R&0x10)	//pause button

//Port initialization for PE1-0 (button inputs)
//PE0 = fire missile, PE1 = shield
void PortE_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x10;
	while ((SYSCTL_PRGPIO_R&0x10) ==0){};
	GPIO_PORTF_DIR_R &= ~0x03;	//inputs PE1-0
	GPIO_PORTF_DEN_R |= 0x03;
}
void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20;
	while ((SYSCTL_PRGPIO_R&0x20) ==0){};
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTF_CR_R |= 0x11;	//enable commit
	GPIO_PORTF_AMSEL_R &= 0xEE;
	GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFF0)+0x00000000;
	GPIO_PORTF_AFSEL_R &= 0xEE;
	GPIO_PORTF_DIR_R &= ~0x11;
	GPIO_PORTF_DEN_R |= 0x11;
	GPIO_PORTF_PUR_R = 0x11;
}
void Main_Menu(void){
	ST7735_DrawBitmap(0,153,mainmenu,128,146);	//draws main menu
}

typedef enum {dead, alive} status_t;
struct sprite{
	uint32_t x;
	uint32_t y;
	const unsigned short *image;
	status_t life;
};
typedef struct sprite sprite_t;

////////////////////////////Sprite Initialization///////////////////////////////
sprite_t player = {63, 158, PlayerShip0, alive};
sprite_t fire = {0, 150, fireball, dead};
sprite_t enemy = {0, 0, SmallEnemy10pointA, alive};
sprite_t enemyblock[3][4];
sprite_t efire = {0, 0, enemyfire, dead};
////////////////////////////////////////////////////////////////////////////////

int ADCMail;
int ADCStatus;
int ADCvalue;
void UserPosition(void){	//grabs ADC sample (controlled by Timer0)
	ADCMail = ADC_In();		//sample ADC and put in mailbox
	ADCStatus = 1;				//set ADC flag to say there is mail to be read
}

//COLLISION DETECTION BETWEEN ENEMY AND PLAYER SPRITES
uint8_t EnemyPlayerCollision(int ex, int ey){	
	if(ey>=(player.y-8)){
		for(int j=0; j<16; j++){
			if(((ex+j)>=player.x)&&((ex+j)<=(player.x+18))){
				return 1;
			}
		}
	}
	return 0;
}
//if RespawnEnemies()==1, call EnemyBlock_Init with enemyx=? enemyy=?
uint8_t RespawnEnemies(void){	//return 1 if all enemies are dead
	for(int a=0; a<3; a++){
		for(int b=0; b<4; b++){
			if(enemyblock[a][b].life == alive){
				return 0;		//if any enemies are alive, return 0
			}
		}
	}
	return 1;		//return 1 if all enemies are dead
}

//Initializes block of enemies (3 by 4)
void EnemyBlock_Init(void){
	DisableInterrupts();		
	int enemyx = 0;
	int enemyy = 40;
	for(int a=0; a<3; a++){
		for(int b=0; b<4; b++){
			enemyblock[a][b].x = enemyx;
			enemyblock[a][b].y = enemyy;
			enemyblock[a][b].image = SmallEnemy10pointA;
			enemyblock[a][b].life = alive;
			enemyx = enemyx + 28;		//16 + distance between enemies
		}
		enemyx = 0;
		enemyy = enemyy + 14;
	}
	for(int x=0; x<3; x++){
		for(int y=0; y<4; y++){
			ST7735_DrawBitmap(enemyblock[x][y].x, enemyblock[x][y].y, SmallEnemy10pointA, 16,10);
		}
	}
	EnableInterrupts();
}

uint8_t LastInCol(int col){
	for(int i=2; i>=0; i--){
		if(enemyblock[i][col].life == alive){
			return i;
		}
	}
}

//Finds enemy closest to the player
void FindBestEnemy(int px){
	//int comparey=0;
	int difference;
	int closestdiff=128;
	int enx, eny;
	int enxpos;
	int lastincol;
	//FIND X POSITION THATS THE CLOSEST TO THE PLAYER
	
	for(int b=0; b<3; b++){	
		for(int a=0; a<4; a++){
			if(enemyblock[b][a].life==alive){
				difference = (enemyblock[b][a].x+7)-px;
				if(difference<0){
					difference = 0-difference;
				}
				if(difference<closestdiff){
					closestdiff = difference;
					enx = enemyblock[b][a].x; //enx has xpos of closet enemy
					enxpos = a;
					//eny = enemyblock[b][a].y;
				}
			}
		}
	}
	lastincol = LastInCol(enxpos);
	eny = enemyblock[lastincol][enxpos].y;
	efire.x = enx+3;
	efire.y = eny+16;
}

//COLLISION DETECTION BETWEEN ENEMY FIRE AND PLAYER
uint8_t FirePlayerCollision(int fx, int fy){
	if(fy>=(player.y-8)){
		for(int j=0; j<10; j++){
			if(((fx+j)>=player.x)&&((fx+j)<=player.x+18)){
				return 1;
			}
		}
	}
	return 0;
}



//Called by Timer; Moves enemy fire downwards while checking for collision
void EnemyFire(void){
	if(efire.life == dead){
		return;
	}
	if(efire.y >= 159){
			ST7735_DrawBitmap(efire.x, efire.y, clear, 10, 16);
			efire.life = dead;
	}
	else if(FirePlayerCollision(efire.x, efire.y) == 1){
		player.life = dead;
		return;
	}
	else{
		ST7735_DrawBitmap(efire.x, efire.y, enemyfire, 10, 16);
		efire.y = efire.y + 1;
		if(efire.y >= 159){
			ST7735_DrawBitmap(efire.x, efire.y, clear, 10, 16);
			efire.life = dead;
		}
	}
}
int enemyfiredelay=0;
int oldx=0;
int moveright = 1;	//start with moving enemies to the right
int moveleft = 0;
int movedown = 0;
//Moves enemy block uniformly, and shoots enemy fire periodically
void EnemyBlock_Move(void){
	enemyfiredelay++;
	if(enemyfiredelay==4){		//make number higher to allow more time for enemy fire to fall****************************************************************	
		FindBestEnemy(oldx);
		if(efire.life == dead){
			efire.life = alive;			
		}
		oldx = player.x;				//get player's x-position
		enemyfiredelay = 0;
	}
	//move down
	if(movedown==1){						///////////////MOVE DOWN/////////////////////
		//clear old location
		ST7735_DrawBitmap(enemyblock[0][0].x, enemyblock[0][0].y, clearenemy, 128, 10);
		//move enemies down once
		for(int a=0; a<3; a++){
			for(int b=0; b<4; b++){
				enemyblock[a][b].y = enemyblock[a][b].y + 14;
				if(enemyblock[a][b].y>=159){
					player.life = dead;
				}
			}
		}
		moveleft = moveleft^1;
		moveright = moveright^1;
		movedown = 0;
	}	//move to right
	else if(moveright == 1){		//////////////MOVE RIGHT///////////////////
		//move enemies to the right
		for(int a=0; a<3; a++){
			for(int b=0; b<4; b++){
				enemyblock[a][b].x = enemyblock[a][b].x + 1;
			}
		}
		//check for right bound
		for(int i=0; i<4; i++){
			if((enemyblock[0][i].life==alive)||(enemyblock[1][i].life==alive)||(enemyblock[2][i].life==alive)){
				if(((enemyblock[0][i].x+16)==127)||((enemyblock[1][i].x+16)==127)||((enemyblock[2][i].x+16)==127)){
					movedown = 1;
				}
			}
		}
	}	//move to left
	else if(moveleft == 1){	 	  /////////////MOVE LEFT////////////////////
		//move enemies to the left
		for(int a=0; a<3; a++){
			for(int b=0; b<4; b++){
				enemyblock[a][b].x = enemyblock[a][b].x - 1;
			}
		}
		//check for left bound
		for(int i=0; i<4; i++){
			if((enemyblock[0][i].life==alive)||(enemyblock[1][i].life==alive)||(enemyblock[2][i].life==alive)){
				if((enemyblock[0][i].x==0)||(enemyblock[1][i].x==0)||(enemyblock[2][i].x==0)){
					movedown = 1;
				}
			}
		}
	}
	//print the enemies
	for(int x=0; x<3; x++){
		for(int y=0; y<4; y++){
			if(enemyblock[x][y].life == alive){
				ST7735_DrawBitmap(enemyblock[x][y].x, enemyblock[x][y].y, SmallEnemy10pointA, 16,10);
				if(EnemyPlayerCollision(enemyblock[x][y].x, enemyblock[x][y].y)==1){
					//player loses
					player.life = dead;
				}
			}
		}
	}
}

int playerxpos;
int savexpos;
int xposdiff;
int offsetxpos;
int offsetclear;
void ConvertUserPos(int ADCval){	//convert slidepot position to user position
	playerxpos = (128*ADCval)/4700;		//convert ADC sample input to x position
		if(playerxpos<0){
			playerxpos = 0;
		}
		else if(playerxpos>141){
			playerxpos = 141;
		}
	//playerxpos now has the new xpos of player	
	xposdiff = playerxpos-savexpos;	//calculate xpos difference
		
	//new position is to the left of old position	
		if(xposdiff<0){
			xposdiff = ~xposdiff + 1;	
			if(xposdiff<=18){
				xposdiff = 18 - xposdiff;		//xposdiff now has overlap xdiff of the two images
				offsetxpos = savexpos + xposdiff;			//add overlap to old xpos to make new xpos to clear
				offsetclear = 18 - xposdiff;		//calculate area to clear
				ST7735_DrawBitmap(offsetxpos, 158, clear, offsetclear,8);
				ST7735_DrawBitmap(playerxpos, 158, PlayerShip0, 18,8);
				player.x = playerxpos;	//update player's x position
				savexpos = playerxpos;	//save new position as old position
				return;
			}
			xposdiff = 18;
		}
		
	//new position is to the right of old position	
	player.x = playerxpos;	//update player's x position
	ST7735_DrawBitmap(savexpos, 158, clear, xposdiff,8);	
	ST7735_DrawBitmap(playerxpos, 158, PlayerShip0, 18,8);
	savexpos = playerxpos;	//save new position as old position
}

//COLLISION DETECTION FOR PLAYER FIRE AND ENEMY
uint8_t EnemyFireCollision(int fx, int fy){	//return 1 if collision detected
	for(int a=0; a<3; a++){
		for(int b=0; b<4; b++){
			if(enemyblock[a][b].life == alive){
				if((fy-14)==enemyblock[a][b].y){
					for(int j=0; j<6; j++){
						if(((fx+j)>=enemyblock[a][b].x)&&((fx+j)<=enemyblock[a][b].x+16)){
							//make enemy dead and erase enemy image
							if(explosionflag==0){
								explosionflag=1;
								UpdateSound(Sound_Explosion);
							}
							ST7735_DrawBitmap(enemyblock[a][b].x, enemyblock[a][b].y, deadenemy, 16, 10);
							enemyblock[a][b].life = dead;
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}
int score=0;
//Called by Timer; Controls upward motion of player fire while checking for collisions
void Fire(void){
	if(fire.life == dead){
		return;
	}
	else{
		if(EnemyFireCollision(fire.x, fire.y) == 1){	//erase fire	//make statements as else if???********************************************
			score = score + 10;		//increase score each time enemy dies
			ST7735_DrawBitmap(fire.x, fire.y, clear, 6, 14);
			fire.life = dead;
			if(RespawnEnemies()==1){	//if all enemies are dead
				EnemyBlock_Init();		//initialize enemy block again
			}		
			return;
		}
		ST7735_DrawBitmap(fire.x, fire.y, shortgreen, 6, 14);
		fire.y = fire.y - 1;
		if(fire.y == 0){
			ST7735_DrawBitmap(fire.x, fire.y, clear, 6, 14);
			fire.life = dead;
		}
	}
}	

volatile int delay;
volatile int delay1;
void Pause(void){
	for(int i=0; i<1000000; i++){
		delay++;
	}
	while(PF4!=0){};
	for(int y=0; y<1000000; y++){
		delay1++;
	}
}

int main(void){
	//INITIALIZATION
	PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
	ADC_Init();
	DAC_Init();
	PortF_Init();				//PF4 and PF0
	PortE_Init();				//Buttons PE1-0
	ST7735_InitR(INITR_REDTAB);
	Sound_Init();			//SysTick initialized in Sound_Init()
	EnableInterrupts();
	Timer0_Init(&EnemyBlock_Move, 50000000);		//speed of enemy block
	Timer3_Init(&UserPosition, 1333333);		//samples ADC regularly
	Timer1_Init(&Fire, 333333);		//controls speed of fireball 1/(150*12.5*10^-9)
	Timer2_Init(&EnemyFire, 800000);		//controls speed of enemy missiles
	
	//MAIN MENU
	UpdateSound(Sound_Bkgd);
	while(PF0==1){
		Main_Menu();	//keep displaying main menu unless a button is pressed
	}
	
	ST7735_FillScreen(0x0000); 
	//GAME PLAY
	EnemyBlock_Init();	//initialize enemy block
	score = 0;	//initialize score to 0
	player.life = alive;
	
	while(player.life==alive){
		//grab user position and display on lcd
		while(ADCStatus == 0){};
		ADCvalue = ADCMail;		//grab ADC value from mail (0-2000)
    ADCStatus = 0;				//clear ADC status flag
	  ConvertUserPos(ADCvalue);
		//watch for button press to fire missiles
		if((PF0 == 0)&&(fire.life == dead)){
			fire.x = player.x + 6;
			fire.y = 150;
			if(shootflag==0){
				shootflag=1;
				UpdateSound(Sound_Shoot);
			}
			fire.life = alive;
		}
		if(PF4 == 0){
			ST7735_SetCursor(0, 0);
			ST7735_OutString("PAUSED");
			DisableInterrupts();
			Pause();
			EnableInterrupts();
			ST7735_DrawBitmap(0, 10, clear, 45,12);
		}
	}
	//GAME OVER, PLAYER DIED
	//DISPLAY SCORE
	DisableInterrupts();
	ST7735_FillScreen(0x0000);
	ST7735_DrawBitmap(0, 159, gameover1, 128, 160);
	ST7735_SetCursor(0, 0);
	ST7735_OutString("SCORE: ");
	ST7735_SetCursor(7, 0);
	LCD_OutDec(score);	
	
	while(1){
		
	}
}


// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

// Sound.h
// Runs on TM4C123 or LM4F120
// Prototypes for basic functions to play sounds from the
// original Space Invaders.
// Jonathan Valvano
// November 17, 2014
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

void Sound_Init(void);
void UpdateSound(void(*task)(void));
void Sound_Shoot(void);
void Sound_Bkgd(void);
void Sound_Explosion(void);

#endif

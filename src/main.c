///////////////////////////////////////////////////////////////////////////////////
//  This file is part of Sorcerers. Copyright (C) 2020 @salvakantero
//                                                2024 @HackTechDev
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////////

#include <cpctelera.h>

#include "gfx/font.h"			// letters and numbers (6x8 px)
#include "gfx/logo.h"			// game logo for the main menu (106x36 px)

///////////////////////////////////////////////////////////////////////////////////
// DEFINITIONS AND VARIABLES
///////////////////////////////////////////////////////////////////////////////////

#define TRUE	1
#define FALSE	0

#define GLOBAL_MAX_X  80 	// X maximum value for the screen (bytes)
#define GLOBAL_MAX_Y  200	// Y maximun value for the screen (px)

#define FNT_W 3 // width of text characters (bytes)
#define FNT_H 8 // height of text characters (px)

#define BG_COLOR 1 // background color (1 = black)

// Palette uses hardware values.
const u8 g_palette[16] = { 0x4d, 0x54, 0x40, 0x4b, 0x44, 0x55, 0x57, 0x53, 0x5c, 0x4c, 0x4e, 0x47, 0x56, 0x52, 0x5e, 0x4a };

// other global variables
u8 ctInactivity[2];	// counters to detect inactive players
i16 ctMainLoop; 	// main loop iteration counter

// keyboard / joystick controls
cpct_keyID ctlUp[2];
cpct_keyID ctlDown[2];
cpct_keyID ctlLeft[2];
cpct_keyID ctlRight[2];
cpct_keyID ctlAbort;
cpct_keyID ctlMusic;
cpct_keyID ctlPause;


// Transparency mask table
cpctm_createTransparentMaskTable(g_maskTable, 0x100, M0, 0);


///////////////////////////////////////////////////////////////////////////////////
//  FUNCTION STATEMENTS
///////////////////////////////////////////////////////////////////////////////////

void InitGame();

///////////////////////////////////////////////////////////////////////////////////
// GENERIC FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

// returns the absolute value of a number
i16 Abs(i16 number) __z88dk_fastcall {
  if (number < 0)
    number *= -1;
  return (number);
}


// get the length of a string
u8 Strlen(const unsigned char *str) __z88dk_fastcall {
  const unsigned char *s;
  for (s = str; *s; ++s);
  return (s - str);
}


// converts an integer to ASCII (Lukás Chmela / GPLv3)
char* Itoa(u16 value, char* result, int base) {
  int tmp_value;
  char* ptr = result, *ptr1 = result, tmp_char;

  if (base < 2 || base > 36) {
    *result = '\0';
    return result;
  }

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while (value);

  if (tmp_value < 0)
    *ptr++ = '-';
  *ptr-- = '\0';

  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }

  return result;
}


// generates a pause
void Pause(u16 value) __z88dk_fastcall {
  u16 i;
  for(i=0; i < value; i++) {
    __asm
      halt
      __endasm;
  }
}



// every x calls, plays the music and/or FX and reads the keyboard
void Interrupt() {
  static u8 nInt;

  if (++nInt == 6) {
    cpct_scanKeyboard_if();
    nInt = 0;
  }
}


///////////////////////////////////////////////////////////////////////////////////
// GRAPHICS, TILES AND SCREEN MANAGEMENT FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

// cleans the screen with the specified color
void ClearScreen() {
  cpct_memset(CPCT_VMEM_START, cpct_px2byteM0(BG_COLOR, BG_COLOR), 16384);
}

// print a number as a text string
void PrintNumber(u16 num, u8 len, u8 x, u8 y, u8 prevDel) {
  u8 txt[6];
  u8 zeros;
  u8 pos = 0;
  u8 nAux;

  Itoa(num, txt, 10);
  zeros = len - Strlen(txt);
  nAux = txt[pos];

  while(nAux != '\0')	{
    u8* ptr = cpct_getScreenPtr(CPCT_VMEM_START, (zeros + pos) * FNT_W + x, y);
    if (prevDel)
      cpct_drawSolidBox(ptr, cpct_px2byteM0(BG_COLOR, BG_COLOR), FNT_W, FNT_H); // previous deletion
    cpct_drawSpriteMaskedAlignedTable(g_font[nAux - 48], ptr, FNT_W, FNT_H, g_maskTable);
    nAux = txt[++pos];
  }
}


// prints a character string at XY coordinates
void PrintText(u8 txt[], u8 x, u8 y, u8 prevDel) {
  u8 pos = 0;
  u8 car = txt[pos];

  while(car != '\0') { // "@" = blank    ";" = -   ">" = !!   "[" = ,
    u8* ptr = cpct_getScreenPtr(CPCT_VMEM_START, (pos * FNT_W) + x, y);
    if (prevDel)
      cpct_drawSolidBox(ptr, cpct_px2byteM0(BG_COLOR, BG_COLOR), FNT_W, FNT_H); // previous deletion
    cpct_drawSpriteMaskedAlignedTable(g_font[car - 48], ptr, FNT_W, FNT_H, g_maskTable);
    car = txt[++pos];
  }
}


///////////////////////////////////////////////////////////////////////////////////
// KEYBOARD FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

// returns the key pressed
cpct_keyID ReturnKeyPressed() {
  u8 i = 10, *keys = cpct_keyboardStatusBuffer + 9;
  u16 keypressed;
  // We wait until a key is pressed
  do { cpct_scanKeyboard(); } while ( ! cpct_isAnyKeyPressed() );
  // We detect which key has been pressed
  do {
    keypressed = *keys ^ 0xFF;
    if (keypressed)
      return (keypressed << 8) + (i - 1);
    keys--;
  } while(--i);
  return 0;
}


// wait for the full press of a key
// useful to empty the keyboard buffer
void Wait4Key(cpct_keyID key) {
  do cpct_scanKeyboard_f();
  while(!cpct_isKeyPressed(key));
  do cpct_scanKeyboard_f();
  while(cpct_isKeyPressed(key));
}


// asks for a key and returns the key pressed
cpct_keyID RedefineKey(u8 *info) {
  cpct_keyID key;
  PrintText(info, 28, 120, 1);
  key = ReturnKeyPressed();
  Wait4Key(key);
  return key;
}


///////////////////////////////////////////////////////////////////////////////////
// MAIN MENU
///////////////////////////////////////////////////////////////////////////////////

void PrintStartMenu() {
  ClearScreen();

  cpct_drawSprite(g_logo_0, cpctm_screenPtr(CPCT_VMEM_START, 0, 0), G_LOGO_0_W, G_LOGO_0_H);
  cpct_drawSprite(g_logo_1, cpctm_screenPtr(CPCT_VMEM_START, G_LOGO_0_W, 0), G_LOGO_0_W, G_LOGO_0_H);

  PrintText("1@@@MISSION", 10, 50, 0);

  PrintText("NEKROFAGE", 13, 190, 0);
}


void StartMenu() {
  u8 randSeed = 254;
  u8 page = 0;

  while(1) {
    // get the seed of randomness based on the time it takes to press a key
    // switches between menu and help after 256 cycles
    if (++randSeed == 255) {
      if (page == 0)
        PrintStartMenu();	// page 1; menu
      randSeed = 0;
      if (++page == 8) //12
        page = 0;
    }
    // get keystrokes from menu options
    cpct_scanKeyboard_f();
    Pause(3);
  }
  cpct_setSeed_lcg_u8(randSeed); // set the seed
  //ClearScreen();
}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP
///////////////////////////////////////////////////////////////////////////////////

// assigns default values ​​that do not vary between games
void InitValues() {
  ctlAbort = Key_X;
  ctlMusic = Key_M;
  ctlPause = Key_H;
}



// initialization of some variables
void InitGame() {
  StartMenu(); // run the start menu
}


void main(void) {
  // disable firmware
  cpct_disableFirmware();
  // initialize sound effects
  // initialize the interrupt manager (keyboard and sound)
  cpct_setInterruptHandler(Interrupt);
  // activate mode 0; 160 * 200 16 colors
  cpct_setVideoMode(0);
  // assign palette
  cpct_setPalette(g_palette, 16);
  // print border (black)
  cpct_setBorder(g_palette[BG_COLOR]);
  // assigns default values ​​that do not vary between games
  InitValues();
  // initial value assignment for each item
  InitGame();

  // main loop
  while (1) {
    cpct_waitVSYNC(); // wait for vertical retrace

    // shift system to avoid double video buffer
    switch (ctMainLoop % 3) {
      // turn 1
      case 0: {
                break;
              }
              // turn 2
      case 1:	{
                break;
              }
              // turn 3
      case 2:	{
                break;
              }
    }

    if (ctMainLoop == 174) {
      ctMainLoop++;
    }
    else if (ctMainLoop++ == 350) {
      ctMainLoop = 0; // reset counter
    }

    // DEBUG
    //PrintNumber(spr[3].dir, 2, 72, 185, 1);
  }
}

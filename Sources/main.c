/*
************************************************************************
ECE 362 - Mini-Project C Source File - Spring 2016
***********************************************************************

Team ID: < 01 >

Project Name: < TAPS >

Team Members:

- Team/Doc Leader: < Tiger Cheng >       Signature: ______________________

- Software Leader: < Pradyuman Vig >     Signature: ______________________

- Interface Leader: < Annan Ma >         Signature: ______________________

- Peripheral Leader: < Shichen Lin >     Signature: ______________________


Academic Honesty Statement:  In signing above, we hereby certify that we
are the individuals who created this HC(S)12 source file and that we have
not copied the work of any other student (past or present) while completing
it. We understand that if we fail to honor this agreement, we will receive
a grade of ZERO and be subject to possible disciplinary action.

***********************************************************************

The objective of this Mini-Project is to .... < ? >


***********************************************************************

List of project-specific success criteria (functionality that will be
demonstrated):

1.

2.

3.

4.

5.

***********************************************************************

Date code started: < 4/23/2016 >

Update history (add an entry every time a significant change is made):

Date: < ? >  Name: < ? >   Update: < ? >

Date: < ? >  Name: < ? >   Update: < ? >

Date: < ? >  Name: < ? >   Update: < ? >


***********************************************************************
*/

#include <hidef.h>
#include <mc9s12c32.h>

#include "derivative.h"

/* User Defined Functions */
void sample_switches(void);
void sample_sensors(void);
void set_speed(void);
void update_destination(long location);
void servo_driver(void);
void disp(void);
void shiftout(char x);
void lcdwait(void);
void send_byte(char x);
void send_i(char x);
void chgline(char x);
void print_c(char x);
void pmsglcd(char[]);
void reset_motor(void);
void delay_10us(int);

/*  Variable declarations */
long despos;
long curpos;
int all_zero = 1;
long wait = 0;
char prevdir = 0;
char reset = 1;
char fservo = 0;
int dest_switch = 0;

/* USELESS BOX PARAMETERS TO SAVE CPU CYCLES */
long sw[8] = {900, 4500, 8100, 11700, 15300, 18900, 22500, 26100}; //sw[i] = 900+3600*i
long sensor[16] = {1800, 1800, 3600, 5400, 7200, 9000, 10800,
                   12600, 14400, 16200, 18000, 19800, 21600,
                   23400, 25200, 25200}; //sensor[i] = 1800*i
char state = 0;
#define OFFSET 0

/* LCD COMMUNICATION BIT MASKS */
#define RS 0x04		// RS pin mask (PTT[2])
#define RW 0x08		// R/W pin mask (PTT[3])
#define LCDCLK 0x10	// LCD EN/CLK pin mask (PTT[4])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position

/* Initializations */
void initializations(void) {
  // Set the PLL speed (bus clock = 24 MHz)
  CLKSEL &= 0x80; // disengage PLL from system
  PLLCTL |= 0x40; // turn on PLL
  SYNR = 0x02; // set PLL multiplier
  REFDV = 0; // set PLL divider
  while (!(CRGFLG & 0x08));
  CLKSEL |= 0x80; // engage PLL

  // Disable watchdog timer (COPCTL register)
  COPCTL = 0x40; //COP off; RTI and COP stopped in BDM-mode

  // Initialize digital I/O port pins
  ATDDIEN = 0xFF; //Digital Input
  DDRAD = 0xFC; //Port AD direction
  DDRT = 0x7F; //Port T direction

  // Initialize TIM Ch 7 (TC7) for periodic interrupts every 30 us
  TIOS = 0x80; //set channel 7 for output compare
  TSCR1 = 0x80; //enable timer subsystem
  TSCR2 = 0x08; //set appropriate pre-scale factor and enable counter reset after OC7
  TIE = 0x00; //set up channel 7 to generate 30 us interrupt rate
  TC7 = 720; //initially disable TIM Ch 7 interrupts

  //Initialize ATD module
  ATDCTL2 = 0x80;
  ATDCTL3 = 0x10;
  ATDCTL4 = 0x85;

  /*
    Initialize the PWM unit to produce a signal with the following
    characteristics on PWM output channel 3:
    - sampling frequency of approximately 100 Hz
    - left-aligned, positive polarity **
    - period register = $FF (yielding a duty cycle range of 0% to 100%,
    for duty cycle register values of $00 to $FF
    - duty register = $00 (motor initially stopped) **

    IMPORTANT: Need to set MODRR so that PWM Ch 3 is routed to port pin PT3
  */
 	MODRR = 0x08; // PT0** used as PWM Ch 3 output
	PWME = 0x0C; // enable PWM Ch 3, 2
	PWMPOL = 0x08; // set active high polarity on ch 3 **
	PWMCTL = 0x20; // concatenate 2&3 (16-bit) CON45:$40 CON23:$20 CON10:$10
	PWMCAE = 0x00; // left-aligned output mode
	PWMPER2	= 0x3A; // set maximum 16-bit period (Higher Byte) **
	PWMPER3	= 0x98; // set maximum 16-bit period
	PWMDTY2	= 0x00; // initially clear DUTY register H
	PWMDTY3	= 0x00; // initially clear DUTY register
	PWMCLK = 0x00; // select Clock B for Ch 3
	PWMPRCLK = 0x40; // set Clock B Prescalar = 1.5 MHz (prescaler = 16) rate
  // needs to count to 15000 for 100 Hz **??

  /* SPI setting and LCD initialization*/
  SPICR1 = 0x50;
  SPICR2 = 0;
  SPIBR = 1;
  PTT_PTT0 = 0;
  PTT_PTT4 = 1;
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait();

  INTCR = 0x00; // Disable IRQ interrupts

  PTT_PTT5 = 0; // Enable motor
}

/* Main */
void main(void) {
  DisableInterrupts;
	initializations();
	EnableInterrupts;
  //reset_motor();
  curpos = 3000;

  for(;;) {
    all_zero = 1;
    set_speed();
    sample_switches();
    if (all_zero) sample_sensors();
    disp();
  }
}


/* Step Motor Driver */
interrupt 15 void TIM_ISR(void) {
  if (curpos != despos) {
    PTT_PTT6 = curpos < despos ? 0 : 1;

    if (prevdir != PTT_PTT6) {
      if (wait++ >= 40000) {
        wait = 0;
        prevdir = PTT_PTT6;
      }
      return;
    }

    PTT_PTT1 = !PTT_PTT1;

    if (PTT_PTT6) curpos -= PTT_PTT1;
    else curpos += PTT_PTT1;
  } else if (!all_zero && fservo) {
    servo_driver();
  } else if (wait < 40000) {
    wait++;
  }

  // clear TIM CH 7 interrupt flag
  TFLG1 |= 0x80;
}

/* Servo Driver */
void servo_driver(void) {
  int i;
  fservo = 0;
  PWMDTY2 = 0x09; //0x0A;
  PWMDTY3 = 0xF6; //0x28;
  for (i = 0; i < 100; i++) delay_10us(100);
  sample_switches();
  PWMDTY2 = all_zero ? 0x02 : 0x08;
  PWMDTY3 = all_zero ? 0xEE : 0xCA;
  for (i = 0; i < 100; i++) delay_10us(1000);
}

/* Switch Sampling */
void sample_switches(void) {
  int i;
  state = 0;
  for (i = 0; i < 8; i++) {
    PTAD_PTAD5 = (i & 4) >> 2;
    PTAD_PTAD6 = (i & 2) >> 1;
    PTAD_PTAD7 = (i & 1) >> 0;

    if (PTT_PTT7) {
      state |= 1 << i;
      fservo = 1;
      all_zero = 0;
      update_destination(sw[i] + OFFSET);
      dest_switch = i + 1;
    }
  }
}

/* Sensor Sampling */
void sample_sensors(void) {
  int i;
  for (i = 0; i < 8; i++) {
    PTAD_PTAD2 = (i & 4) >> 2;
    PTAD_PTAD3 = (i & 2) >> 1;
    PTAD_PTAD4 = (i & 1) >> 0;

    if (PTAD_PTAD0) update_destination(sensor[i] + OFFSET);
    if (PTAD_PTAD1) update_destination(sensor[i + 8] + OFFSET);
    fservo = 1;
  }
}

/* Set Speed of Step Motor */
void set_speed(void) {
  ATDCTL5 = 0x10;
  while(!ATDSTAT0_SCF);
  TC7 = 720 * ATDDR0H / 255;
}

/* Update Destination */
void update_destination(long loc) {
  long diff = curpos - despos;
  long newdiff = curpos - loc;

  if (all_zero) {
    despos = loc;
    dest_switch = 0;
  } else {
    if (diff < 0) diff *= -1;
    if (newdiff < 0) newdiff *= -1;
    if (diff > newdiff) despos = loc;
  }
}

/* Clears reset flag */
interrupt 6 void IRQ_ISR(void) {
  reset = 0;
}

/* Display to LCD */
void disp(void) {
  int i;

  chgline(LINE1);
  pmsglcd("Hello World!   ");
  if (dest_switch) print_c(dest_switch + 0x30);
  else print_c(0x20);

  chgline(LINE2);
  for (i = 0; i < 16; i++) {
    if (i == (curpos - OFFSET) * 16 / 26100) print_c(0xFF);
    else if (!(i % 2) && (state & 1 << i / 2)) print_c(i / 2 + 1 + 0x30);
    else print_c(0x20);
  }
}

/* Transmits the character x to external shift register using the SPI. */
void shiftout(char x) {
  int i = 10;
  while(!SPISR_SPTEF); // read the SPTEF bit, continue if bit is 1
  SPIDR = x; // write data to SPI data register

  while(i--); // wait for SPI data to shift out
}

/* Delay for LCD to update */
void lcdwait(void) {
	int i = 1000;
	while(i--);
}

/* send_byte: writes character x to the LCD */
void send_byte(char x) {
  shiftout(x);
  PTT_PTT4 = 0;
  PTT_PTT4 = 1;
  PTT_PTT4 = 0;
  lcdwait();
}

/* send_i: Sends instruction byte x to LCD */
void send_i(char x) {
  PTT_PTT2 = 0; // set the register select line low (instruction data)
  send_byte(x);
}

/* chgline: Move LCD cursor to position x */
void chgline(char x) {
	send_i(CURMOV);
	send_i(x);
}

/* Print (single) character x on LCD */
void print_c(char x) {
	PTT_PTT2 = 1;
	send_byte(x);
}

/* Print character string str[] on LCD */
void pmsglcd(char str[]) {
	int i = 0;
	while(str[i] != '\0') print_c(str[i++]);
}

/* Reset motor to rest near calibration button */
void reset_motor(void) {
  int i;
  TIE = 0x00;
  INTCR = 0x40;

  reset = 1;
  PTT_PTT6 = 1;
  while (reset) {
    PTT_PTT1 = 1;
    delay_10us(5);
    PTT_PTT1 = 0;
    delay_10us(5);
  }

  PTT_PTT6 = 0;
  for (i = 0; i < 3200; i++) {
    PTT_PTT1 = 1;
    delay_10us(5);
    PTT_PTT1 = 0;
    delay_10us(5);
  }

  reset = 1;
  PTT_PTT6 = 1;
  while (reset) {
    PTT_PTT1 = 1;
    delay_10us(50);
    PTT_PTT1 = 0;
    delay_10us(50);
  }

  INTCR = 0x00;
  curpos = 0;
  TIE = 0x80;
}

/* Delay around 10us */
void delay_10us(int time) {
  int i;
  long loopnum = 18 * time;
  for(i = 0; i < loopnum; i++);
}

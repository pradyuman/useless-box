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

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* User Defined Functions */

void delay_10us(int);
void reset_motor(void);

/*  Variable declarations */

long despos;
long curpos;
int wait = 0;;
int prevdir = 0;
int reset = 1;

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

/*
***********************************************************************
 Initializations
***********************************************************************
*/

void  initializations(void) {

/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; //; disengage PLL from system
  PLLCTL = PLLCTL | 0x40; //; turn on PLL
  SYNR = 0x02;            //; set PLL multiplier
  REFDV = 0;              //; set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; //; engage PLL


/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode

/* Initialize digital I/O port pins */
   DDRT = 0xFF;

/* Initialize RTI for 2.048 ms interrupt rate */

/* Initialize TIM Ch 7 (TC7) for periodic interrupts every 1.000 ms
     - enable timer subsystem
     - set channel 7 for output compare
     - set appropriate pre-scale factor and enable counter reset after OC7
     - set up channel 7 to generate 1 ms interrupt rate
     - initially disable TIM Ch 7 interrupts
*/
     TIOS = 0x80;
     TSCR1 = 0x80;
     TSCR2 = 0x08;
     TIE = 0x00;
     TC7 = 720;
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
 	MODRR	 = 0x08; // PT0** used as PWM Ch 3 output
	PWME	 = 0x0C; // enable PWM Ch 3, 2
	PWMPOL	 = 0x08; // set active high polarity on ch 3 **
	PWMCTL	 = 0x20; // concatenate 2&3 (16-bit) CON45:$40 CON23:$20 CON10:$10
	PWMCAE	 = 0x00; // left-aligned output mode
	PWMPER2	 = 0xFF; // set maximum 16-bit period (Higher Byte) **
	PWMPER3	 = 0xFF; // set maximum 16-bit period
	PWMDTY2	 = 0x00; // initially clear DUTY register H
	PWMDTY3	 = 0x00; // initially clear DUTY register
	PWMCLK	 = 0x00; // select Clock B for Ch 3
	PWMPRCLK = 0x40; // set Clock B Prescalar = 1.5 MHz (prescaler = 16) rate
                     // needs to count to 15000 for 100 Hz **??
// IRQ interrupt setting
     INTCR = 0x00;
     asm{
       cli
     }
}

/*
***********************************************************************
 Main
***********************************************************************
*/

void main(void) {

  DisableInterrupts;
	initializations();
	EnableInterrupts;

  INTCR = 0x40;
  //TIE = 0x80;

  for(;;) {

  }


  /* write your code here */


  /* loop forever */

}  /* do not leave main */




/*
***********************************************************************
 RTI interrupt service routine: RTI_ISR

  Initialized for 2.048 ms interrupt rate

  Samples state of pushbuttons (PAD7 = left, PAD6 = right)

  If change in state from "high" to "low" detected, set pushbutton flag
     leftpb (for PAD7 H -> L), rghtpb (for PAD6 H -> L)
     Recall that pushbuttons are momentary contact closures to ground
***********************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flag
  	CRGFLG = CRGFLG | 0x80;

}

/*
***********************************************************************
  TIM interrupt service routine
  used to initiate ATD samples (on Ch 0 and Ch 1)
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{

        // clear TIM CH 7 interrupt flag
 	TFLG1 = TFLG1 | 0x80;
  if(curpos != despos){
    if(curpos < despos){
       PTT_PTT2 = 0;
    }else{
       PTT_PTT2 = 1;
    }
    if(prevdir != PTT_PTT2){
      if(wait < 2048) {
        wait++;
      }else{
        wait = 0;
        prevdir = PTT_PTT2;
      }
      return;
    }else{
      PTT_PTT3 = !PTT_PTT3;
      if(PTT_PTT2){
        curpos -= PTT_PTT3;
      }else{
        curpos += PTT_PTT3;
      }
    }
  }else{
    if(wait < 2048) {
      wait++;
    }
  }
}

//IRQ interrupt

interrupt 6 void IRQ_ISR(void)
{
  reset = 0;
}


void reset_motor(void)
{
  int i;
  INTCR = 0x40;
  TIE = 0x00;
  reset = 1;
  while(reset == 1){
    PTT_PTT2 = 1;
    PTT_PTT3 = 1;
    delay_10us(5);
    PTT_PTT3 = 0;
    delay_10us(5);
  }

  PTT_PTT2 = 0;
  for(i = 0;i < 3200;i++){
    PTT_PTT3 = 1;
    delay_10us(5);
    PTT_PTT3 = 0;
    delay_10us(5);
  }

  reset = 1;
  while(reset == 1){
    PTT_PTT2 = 1;
    PTT_PTT3 = 1;
    delay_10us(50);
    PTT_PTT3 = 0;
    delay_10us(50);
  }

  INTCR = 0x00;
  curpos = 0;
}

//delay around 10us
void delay_10us(int time){
  int i;
  for(i = 0;i < 18*time
  ;i++);
}

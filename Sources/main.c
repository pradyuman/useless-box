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
         
/* Add additional port pin initializations here */

	 	   			 		  			 		  		
/* Initialize digital I/O port pins */
   DDRT = 0xff;
     
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
// PWM Setting
     PWMPRCLK = 0x08;
     PWME = 0x01;
     PWMPOL = 0x01;
     PWMSCLA = 0x08;
     PWMCLK = 0x01; 
     DDRP = 0x01;
*/

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
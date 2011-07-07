#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <scans.c>                   //HEADER CONTAINING THE SCAN CODES WITH THEIR CORRESPONDING CHARACTERS
#include <stdlib.h>
#include <compat/deprecated.h>
#include <avr/interrupt.h>


#define DATA_DDR DDRA     // ENTER THE PORT WHICH CONTROLS
#define DATA_PORT PORTA    //  THE DATA PINS D0 TO D7 


#define CONTROL_DDR DDRD     // ENTER THE PORT WHICH CONTROLS
#define CONTROL_PORT PORTD    // THE CONTROL PINS
#define Enable          6          // CoNNECTION OF ENABLE TO PIN OF ABOVE PORT
#define RS              4         // CoNNECTION OF RegisterSelect TO PIN OF ABOVE PORT
#define RW               5       // CoNNECTION OF Read/~Write TO PIN OF ABOVE PORT
#define CONTROL_MASK     0X70     // CHANGE THIS VALUE CONSIDERING THE PINS ABOVE AS HIGH



#define Set_Enable  CONTROL_PORT |=_BV(Enable)           // THE MACROS HERE ARE
#define Clear_Enable CONTROL_PORT &=~_BV(Enable)          //SELF EXPLANATORY
#define Write_Lcd     CONTROL_PORT &=~_BV(RW)              
#define Read_Lcd    CONTROL_PORT |=_BV(RW)       
#define Select_InstructionRegister CONTROL_PORT &=~_BV(RS)                     
#define Select_DataRegister     CONTROL_PORT |=_BV(RS)                       
#define Data_Lcd(b) DATA_PORT=b                  
#define delay(b) _delay_ms(b) 

void Init_Ports(void);                // Initialise Ports , Necessary for selecting input output pins in the MCU
void Init_Lcd(void);                  // Initialise LCD   , Necessary for starting and clearing LCD screen
void Lcd_Send(unsigned char a);       // For sending a character to the LCD                 
void newline(void);                   // To move cursor to the next line in the LCD
void usart_init(void);                  //  Initialise INT2
void interpret(void);                  //  Interpret the scan code received-when caps lock is off
void interpret1(void);                 //when caps lock is on
void clear(void);

static unsigned int count=0;
static unsigned char data=0x00,temp=0x00;
static unsigned char decoded=0x00;
static unsigned int flag=0;

int main(void)
{

Init_Ports();
Init_Lcd();
usart_init();    
                           

while(1)
{ }

}


void usart_init(void)
{ UCSRB|=(1<<RXCIE)|(1<<RXEN);                                                 //enable receiver and rx complete interrupt
  UCSRB&=~(1<<UCSZ2);
  UCSRC|=(1<<URSEL)|(1<<UMSEL)|(1<<UCSZ1)|(1<<UCSZ0)|(1<<UPM1)|(1<<UPM0);    //synchronous mode and 8 bit char size and parity mode enabled with odd parity
  UCSRA&=~((1<<FE)|(1<<DOR)|(1<<PE));                                        //frame,parity and data overrun errors
  UCSRC&=~(1<<UCPOL);                                                            //received data sampled on falling xck edge
 sei(); 
}

ISR(USART_RXC_vect)
{  count++;
  if(count==1)
     { data=UDR;}
  if(count>1&&count<4)
     {temp=UDR;}
  if(count==3)
     {count=0;
	  if(data==0x66){clear();}         // Scan Code corresponding to backspace
         else if(data==0x5a){newline();}       // Scan Code corresponding to Return
              else if(data==0x58){flag=!flag;}      // Scan code corresponding to Caps Lock
                   else if(flag)                          // Check whether Caps should be on
                          {interpret1();
                          Lcd_Send(decoded);}
                        else {interpret();                   
                              Lcd_Send(decoded);}
         
      }
}

void Lcd_Send(unsigned char a)
{
Select_DataRegister;                        // Declares information that follows as data and not instruction
Write_Lcd;                                  // Declared information is to be written
Data_Lcd(a);                                // Send the character passed to the function to LCD to write
Set_Enable;                                 // Sets enable,
delay(10);                                   // and then
Clear_Enable;                               // Clears it,
delay(10);                                   // to be ready for next character.
}


void Init_Lcd(void)
{
char init[10];
int i;
init[0] = 0x01;                          // Initialises the display
init[1] = 0x38;                          // 8 - Bit Operation, 2 Line Display,5*8 Dot Character Font
init[2] = 0x0e;                          // Turns on display and cursor
init[3] = 0x06;   // Entry Mode Set 
init[4] = 0x80;                          // Sets DDRAM address to beginning of screen
Select_InstructionRegister;

Write_Lcd;
delay(15);

for(i=0;i<5;i++)
{
Data_Lcd(init[i]);
Set_Enable;
delay(10);
Clear_Enable;
}
}


void newline(void)
{
delay(5);
Select_InstructionRegister;                  // Declares information to follow as instruction
Data_Lcd(0xc0);                              // Code to go to next line of LCD
Set_Enable;                                  //  --  Enable cycle  --
delay(10);                                    // |                    |
Clear_Enable;                                // |                    |  
delay(10);                                    //  --                --
}

void clear(void)
{
delay(5);
Select_InstructionRegister;                  // Declares information to follow as instruction
Data_Lcd(0x10);                              // Code to go to next line of LCD
Set_Enable;                                  //  --  Enable cycle  --
delay(5);                                    // |                    |
Clear_Enable;                                // |                    |  
delay(5);                                    //  --                --
}

void interpret(void)
{
unsigned char i;
for(i = 0; unshifted[i][0]!=data && unshifted[i][0]; i++);    // Traverse the array and stop at 'i' where 
                                                                                     // a match is found
if (unshifted[i][0] == data)
{
decoded=unshifted[i][1];
delay(10);                                                     // Decode the keypress and send to LCD
}
}

void interpret1(void)
{
unsigned char i;
for(i = 0;shifted[i][0]!=data && shifted[i][0]; i++);       // Traverse the array and stop at 'i' where 
                                                                            // a match is found
if (shifted[i][0] == data)
{
decoded=shifted[i][1];                                       // Decode the keypress and send to LCD
}

}

void Init_Ports(void)
{
DATA_DDR=0XFF;                                        //  Setting data port for output
CONTROL_DDR=CONTROL_MASK;
//CONTROL_PORT=0x7A;                            //   Setting selected pins of control port for output
CONTROL_PORT&=~(_BV(Enable)|_BV(RS )|_BV(RW));       //   Setting values to 0 at start
DDRB=0X00;                                            //PIND0=RXD(DATA) AND PINB0=XCK(CLOCK)
PORTB=0XFF;
}







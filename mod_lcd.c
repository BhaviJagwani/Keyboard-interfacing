/*----------------------------------------------------------------
-------------------- HEADER FILES -------------------------------
-----------------------------------------------------------------*/

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <scans.c>
#include <stdlib.h>
#include <compat/deprecated.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>


/*----------------------------------------------------------------
------------- DEFINITIONS -------------------------------------
-----------------------------------------------------------------*/



#define DATA_DDR	 DDRA     // ENTER THE PORT WHICH CONTROLS
#define DATA_PORT 	 PORTA    //  THE DATA PINS D0 TO D7 


#define CONTROL_DDR	 DDRD     // ENTER THE PORT WHICH CONTROLS
#define CONTROL_PORT 	 PORTD    // THE CONTROL PINS
#define Enable		    2     // CoNNECTION OF ENABLE TO PIN OF ABOVE PORT
#define RS              0         // CoNNECTION OF RegisterSelect TO PIN OF ABOVE PORT
#define RW              1        // CoNNECTION OF Read/~Write TO PIN OF ABOVE PORT
#define CONTROL_MASK     0X07     // CHANGE THIS VALUE CONSIDERING THE PINS ABOVE AS HIGH


/*----------------------------------------------------------------
-------------CONTROL BITS OF LCD --------------------------------
-----------------------------------------------------------------*/

#define Set_Enable  				CONTROL_PORT |=_BV(Enable)           // THE MACROS HERE ARE
#define Clear_Enable 				CONTROL_PORT &=~_BV(Enable)          //SELF EXPLANATORY
#define Write_Lcd			    	CONTROL_PORT &=~_BV(RW)              //
#define Read_Lcd			   		CONTROL_PORT |=_BV(RW)       //
#define Select_InstructionRegister 	CONTROL_PORT &=~_BV(RS)                      //
#define Select_DataRegister	    	CONTROL_PORT |=_BV(RS)                       //
#define Data_Lcd(b)					DATA_PORT=b                  //
#define delay(b)					_delay_ms(b)                  //



/*----------------------------------------------------------------
-----------------FUNCTION DECLARATIONS ---------------------------
-----------------------------------------------------------------*/

void Init_Ports(void);                // Initialise Ports , Necessary for selecting input output pins in the MCU
void Init_Lcd(void);                  // Initialise LCD   , Necessary for starting and clearing LCD screen
void int2init(void);				  //  Initialise INT2

void Lcd_Send(unsigned char a);       // For sending a character to the LCD                 
void newline(void);                   
void clear(void);                     //Backspace         
void clearall(void);                  //Clear lcd screen
void clear_word(void);                 // For printing characters on the next line in the LCD
void interpret(void);				  //  Interpret the scan code received
void interpret1(void);
void eeprom_erase(void);
void retrieve_eeprom(void);
void spl_case(unsigned char x);

/*----------------------------------------------------------------
-----------------GLOBAL DECLARATIONS ---------------------------
-----------------------------------------------------------------*/

static unsigned int count=0;
static unsigned char data=0x00;
static unsigned char decoded=0x00;
static unsigned int flag=0;
unsigned int cline=0;                      //counts the number of characters on a line
unsigned char nline=0x0a;
unsigned int eflag=0;                      //counts the number of times we go to the next line
unsigned int countl=0;                     //counts the letter of half-written word
unsigned char temp=0;

eeprom_addr=0x0000;
/*----------------------------------------------------------------
----------------MAIN FUNCTION--------------------------------------
-----------------------------------------------------------------*/
int main(void)
{

Init_Ports();
int2init();    
Init_Lcd();                           

while(1)
{
}

}
	

/*----------------------------------------------------------------
------------FUNCTION TO INITIATE LCD -----------------------------
-----------------------------------------------------------------*/
void Init_Lcd(void)
{
char init[10];
 int i;
 init[0] = 0x01;                          // Initialises the display
 init[1] = 0x38;                          // 8 - Bit Operation, 2 Line Display,5*8 Dot Character Font
 init[2] = 0x0e;                          // Turns on display and cursor
 init[3] = 0x06; 						  // Entry Mode Set 
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



/*----------------------------------------------------------------
------------FUNCTION TO INITIALISE INT2 --------------------
-----------------------------------------------------------------*/

void int2init(void)
{
cbi(GICR,5);
cbi(MCUCSR,6);
cbi(MCUCSR,6);
sbi(GIFR,5);
sbi(GICR,5);
sei();
}


/*----------------------------------------------------------------
------------FUNCTION TO OBTAIN DATA USING INT2 --------------------
-----------------------------------------------------------------*/

ISR(INT2_vect)
{
count++;

if (count > 1 && count < 10)
{
data = (data >> 1);   // LSB first
if(PINB & 0x01) {data = data | 0x80; } 	// Store a '1'
}


if (count==33) 
{   if(data==0x66){clear();cline--;eeprom_addr--;countl--;eflag=0;}
else if(data==0x5a){newline();eflag++;if(cline%16!=0) cline=cline+(16-(cline%16));}
else if(data==0x58){flag=!flag;}
else if(data==0x76){clearall();}
else if(flag)
	{interpret();
	eeprom_write_byte(eeprom_addr,decoded);
    eeprom_addr++;
    countl++;
	Lcd_Send(decoded);
	spl_case(decoded);
    }
else {interpret1();
     eeprom_write_byte(eeprom_addr,decoded);
     eeprom_addr++;
     countl++;
	 Lcd_Send(decoded);
	 spl_case(decoded);
	 }
    	
	count=0;
	PORTC=count;
	
}

}


/*----------------------------------------------------------------
------------FUNCTION TO INTERPRET THE SCAN CODE --------------------
-----------------------------------------------------------------*/

void interpret(void)
{
unsigned char i;
for(i = 0; unshifted[i][0]!=data && unshifted[i][0]; i++);
if (unshifted[i][0] == data)
{
	decoded=unshifted[i][1];
}
}

void interpret1(void)
{
unsigned char i;
for(i = 0;shifted[i][0]!=data && shifted[i][0]; i++);
if (shifted[i][0] == data)
{
	decoded=shifted[i][1];
}

}	

/*----------------------------------------------------------------
-----------------SEND A CHARACTER TO LCD-------------------------
-----------------------------------------------------------------*/
void Lcd_Send(unsigned char a)
{

//PORTC=countl;
Select_DataRegister;                        // Declares information that follows as data and not instruction
Write_Lcd;                                  // Declared information is to be written
Data_Lcd(a);                                // Send the character passed to the function to LCD to write
Set_Enable;                                 // Sets enable,
delay(10);                                   // and then
Clear_Enable;                               // Clears it,
delay(10);                                   // to be ready for next character.
cline++;
}

/*----------------------------------------------------------------
-----------------INSTRUCTION FUNCTIONS-----------------------------
-----------------------------------------------------------------*/	

//CLEAR LCD SCREEN
void clearall()
{
delay(5);
Select_InstructionRegister;                  // Declares information to follow as instruction
Data_Lcd(0x01);                              // Code to go to next line of LCD
Set_Enable;                                  //  --  Enable cycle  --
delay(10);                                    // |                    |
Clear_Enable;                                // |                    |  
delay(10); 
cline=0;
//countl=0; 
}


//GO TO NEXT LINE
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


//BACKSPACE
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


//CLEAR HALF-WRITTEN WORD
void clear_word()
{
for(int i=0;i<countl;i++)
{
clear();
}
for(int i=0;i<countl;i++)
{Lcd_Send(' ');
}
newline();
}


/*----------------------------------------------------------------
-----------------EEPROM FUNCTIONS---------------------------------
-----------------------------------------------------------------*/	
void eeprom_erase()
{
for(int i=0;i<countl;i++)
{
eeprom_write_byte(eeprom_addr,0xff);
eeprom_addr--;
}
eeprom_addr=0x0000;
countl=0;
}


void retrieve_eeprom()
{
eeprom_addr=0x0000;
temp=eeprom_read_byte(eeprom_addr);
for(int i=0;i<countl;i++)
{
Select_DataRegister;                        // Declares information that follows as data and not instruction
Write_Lcd;                                  // Declared information is to be written
Data_Lcd(temp);                                // Send the character passed to the function to LCD to write
Set_Enable;                                 // Sets enable,
delay(10);                                   // and then
Clear_Enable;                               // Clears it,
delay(10);
eeprom_addr++;
temp=eeprom_read_byte(eeprom_addr);
}
countl=0;
}


/*----------------------------------------------------------------
-----------------SPL CASE FUNCTION---------------------------------
-----------------------------------------------------------------*/	
void spl_case(unsigned char x)
{
if(x==' ')
{
eeprom_erase();
}

if(cline%16==0)
{
eflag++;
clear_word();
if(eflag%2==0&&eflag!=0)
clearall();
retrieve_eeprom();
}
}
/*----------------------------------------------------------------
-----------------FUNCTIONS TO INITIALIZE PORTS--------------------
-----------------------------------------------------------------*/
void Init_Ports(void)
{
DATA_DDR=0XFF;                                        //  Setting data port for output
CONTROL_DDR=CONTROL_MASK;
CONTROL_PORT=0x7A;                            			//   Setting selected pins of control port for output
CONTROL_PORT&=~(_BV(Enable)|_BV(RS )|_BV(RW));       //   Setting values to 0 at start
DDRB=0xfa;												// Set pins B0(data line) and B2(clock line of keyboard going to INT2) as input
PORTB=0xff;
}






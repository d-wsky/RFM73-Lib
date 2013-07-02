/*********************************************************
copyright(c) 2012
Title: 				RFM73 simple example based on PIC c
Current 			version: v1.0
Function:			RFM73 demo
processor: 			PIC16F690
Clock:				internal RC 8M
Author:				baiguang(ISA)
Company:			Hope microelectronic Co.,Ltd.
Contact:			+86-0755-82973805-846
E-MAIL:				rfeng@hoperf.com
Data:				2012-11-10
**********************************************************/
/*********************************************************
                        ---------------
                       |VDD         VSS|
 		    IRQ----    |RA5         RA0|	----CE
           MOSI----    |RA4         RA1|	----CSN
    	   MISO----    |RA3         RA2|    ----SCK
     		   ----    |RC5         RC0|    ----
     		   ----    |RC4         RC1|	----
 			   ----    |RC3         RC2|    ----
               ----    |RC6         RB4|    ----GREEN_LED
     		   ----    |RC7         RB5|	----RED_LED
               ----    |RB7         RB6|	----sensitive_LED
			            ---------------
					       pic16F690
*********************************************************/
#include "RFM73.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <inttypes.h>
#include "lcd.h"
#include "uart.h"
#include "spi.h"

#define Sensitive_LED	   PA2

#define Sensitive_LED_SET  PORTA |= (1 << Sensitive_LED)
#define Sensitive_LED_CLR  PORTA &=~(1 << Sensitive_LED)


unsigned char t1 = 0;

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                            _FDEV_SETUP_WRITE);


uint8_t  count_50hz;

typedef struct 
{
	unsigned char reach_1s				: 1;
	unsigned char reach_5hz				: 1;	
}	FlagType;

FlagType	                Flag;

const uint8_t tx_buf[17]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x78};
uint8_t rx_buf[RFM73_MAX_PACKET_LEN];

extern const uint8_t RX0_Address[];
extern const unsigned long Bank1_Reg0_13[];

/*********************************************************
Function: init_port();                                         
                                                            
Description:                                                
	initialize port. 
*********************************************************/
void init_port(void)
{
	RFM73_CSN_DIR |= (1 << RFM73_CSN_PIN);
	RFM73_CE_DIR  |= (1 << RFM73_CE_PIN);
	RFM73_IRQ_DIR &=~(1 << RFM73_IRQ_PIN);
	RFM73_CE_LOW;
	DDRA |= 7;
}

/*********************************************************
Function: timer2_init();                                         
                                                            
Description:                                                
	initialize timer. 
*********************************************************/
void timer2_init(void)
{
	TCNT1 = 65535-7250;
	TCCR1B |=  (1 << CS12) | (1 << CS10);
	TIMSK |= (1 << TOIE1);
}

/*********************************************************
Function: init_mcu();                                         
                                                            
Description:                                                
	initialize mcu. 
*********************************************************/
void init_mcu(void)
{
	char lcd_buf[16];
	init_port();
	spi_init();
	timer2_init();
	lcd_init();
	uart_init(0, 38400);
	stdout = &mystdout;
	lcd_clear();
	sprintf_P(lcd_buf, PSTR("Hello from RX...")); // Change to RX
	printf_P(PSTR("\033[2JHello from RFM73...\n"));
	lcd_puts(lcd_buf);
}


/*********************************************************
Function:  interrupt ISR_timer()                                        
                                                            
Description:                                                
 
*********************************************************/
ISR(TIMER1_OVF_vect) {
    Flag.reach_1s = 1;
	PORTA ^= 4;
	TCNT1 = 65535-7250;
}

/*********************************************************
Function:      power_on_delay()                                    
                                                            
Description:                                                
 
*********************************************************/
void power_on_delay(void)
{
	_delay_ms(1000);
}

/*********************************************************
Function:  sub_program_1hz()                                        
                                                            
Description:                                                
 
*********************************************************/
void sub_program_1hz(void)
{
	uint8_t i;
	uint8_t temp_buf[32];
	static uint16_t cnt = 0;
	char lcd_buf[16];

	if(Flag.reach_1s)
	{
		Flag.reach_1s = 0;
		
		for(i=0;i<17;i++)
		{
			temp_buf[i]=tx_buf[i];
		}
		
		uint8_t res = rfm73_send_packet(RFM73_TX_WITH_NOACK, temp_buf, 17);
		sprintf_P(lcd_buf, PSTR("S=%d;"), cnt++);
		lcd_gotoxy(0, 1);
		lcd_puts(lcd_buf);
		temp_buf[RFM73_MAX_PACKET_LEN-1]=0;
		printf_P(PSTR("Sent packet %s\nSent %d times\n"), temp_buf, cnt);
		rfm73_rx_mode();  //switch to Rx mode
	}	
}

void repaint(uint8_t pwr, uint8_t gain, uint8_t dr) {
	char lcd_buf[19];
	char lcd1[7], lcd2[7], lcd3[7];
	if (pwr==0)  sprintf_P(lcd1, PSTR("P=-10;"));
	if (pwr==1)  sprintf_P(lcd1, PSTR("P=-5; "));
	if (pwr==2)  sprintf_P(lcd1, PSTR("P= 0; "));
	if (pwr==3)  sprintf_P(lcd1, PSTR("P=+5; "));
	if (gain==0) sprintf_P(lcd2, PSTR("G=-20;"));
	if (gain==1) sprintf_P(lcd2, PSTR("G=  0;"));
	if (dr==0)   sprintf_P(lcd3, PSTR("D= 1"));
	if (dr==1)   sprintf_P(lcd3, PSTR("D=.3"));
	if (dr==2)   sprintf_P(lcd3, PSTR("D= 2"));
	for (uint8_t i=0; i<6; i++) {
		lcd_buf[i]=lcd1[i];
		lcd_buf[i+6]=lcd2[i];
		lcd_buf[i+12]=lcd3[i];
	}
	lcd_gotoxy(0, 0);
	lcd_puts(lcd_buf);
	rfm73_init(pwr, gain, dr);
}

int main(void)
{
	power_on_delay();  
 	init_mcu();
	
	sei();
	
	count_50hz = 0;
	Flag.reach_1s = 0;
	uint16_t cnt = 0, cnt2 = 0;
	uint8_t rx_buf[RFM73_MAX_PACKET_LEN];
	char lcd_buf[16];
	uint8_t pwr = RFM73_OUT_PWR_5DBM;
	uint8_t gain = RFM73_LNA_GAIN_HIGH;
	uint8_t dr = RFM73_DATA_RATE_2MBPS;
	uint8_t b = 0;

	repaint(pwr, gain, dr);
	while(1)
	{
		//sub_program_1hz(); // Comment to use in RX
		uint8_t res = rfm73_receive_packet(RFM73_TX_WITH_ACK, rx_buf); // 1 to RX, 0 to TX
		if (res == 2) {
			sprintf_P(lcd_buf, PSTR("R =%4d;"), cnt++);
			lcd_gotoxy(0, 1);
			lcd_puts(lcd_buf);
			rx_buf[RFM73_MAX_PACKET_LEN-1] = 0;
			printf_P(PSTR("Received packet: %s\nTotal packets: %d\n"), rx_buf, cnt);
		};
		if (res == 1) {
			sprintf_P(lcd_buf, PSTR("RC=%4d;"), cnt2++);
			lcd_gotoxy(0, 1);
			lcd_puts(lcd_buf);
			printf_P(PSTR("Received packet: %s with correct CRC\nTotal correct packets: %d\n"), rx_buf, cnt2);
		}
		
		uint8_t a = PINA & 0xE0;
		
		if (a == 0x80) {
			pwr ++;
			if (pwr == 4) pwr = 0;
			b = 1;
			repaint(pwr, gain, dr);
		}
		if (a == 0x40) {
			gain++;
			if (gain == 2) gain = 0;
			b = 1;
			repaint(pwr, gain, dr);
		}
		if (a == 0x20) {
			dr++;
			if (dr == 3) dr = 0;
			b = 1;
			repaint(pwr, gain, dr);
		}
	}	
}
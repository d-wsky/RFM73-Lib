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

#define GREEN_LED		   PA0
#define RED_LED 		   PA1
#define Sensitive_LED	   PA2

#define GREEN_LED_SET 	   PORTA |= (1 << GREEN_LED)
#define GREEN_LED_CLR      PORTA &=~(1 << GREEN_LED)
#define RED_LED_SET        PORTA |= (1 << RED_LED)
#define RED_LED_CLR        PORTA &=~(1 << RED_LED)
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

void init_mcu(void);
void init_port(void);
void timer2_init(void);

void power_on_delay(void);
void sub_program_1hz(void);

void Send_Packet(uint8_t type,uint8_t* pbuf,uint8_t len);
void Send_NACK_Packet(void);
void Receive_Packet(void);
void SPI_Bank1_Write_Reg(uint8_t reg, uint8_t *pBuf);
void SPI_Bank1_Read_Reg(uint8_t reg, uint8_t *pBuf);
void Carrier_Test(uint8_t b_enable); //carrier test

extern void RFM73_Initialize(void);
extern void SwitchToTxMode(void);
extern void SwitchToRxMode(void);

const uint8_t tx_buf[17]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x78};
uint8_t rx_buf[MAX_PACKET_LEN];

extern const uint8_t RX0_Address[];
extern const unsigned long Bank1_Reg0_13[];

uint8_t test_data;

int main(void)
{
	unsigned char  i, j, chksum;
	
	
	power_on_delay();  
 	init_mcu();
	
	sei();
	
	count_50hz = 0;
	Flag.reach_1s = 0;

	RFM73_Initialize();
	while(1)
	{
		//sub_program_1hz();
		Receive_Packet();
	}
}

#define DD_MOSI           PB2
#define DD_SCK            PB1

#define SPI_DORD_LSB_TO_MSB   SPCR |= (1 << DORD)
#define SPI_DORD_MSB_TO_LSB   SPCR &=~(1 << DORD)

void spi_init() {
	/* Set MOSI and SCK output, all others input */
	DDRB |= (1<<DD_MOSI)|(1<<DD_SCK);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	SPI_DORD_MSB_TO_LSB;
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
	sprintf_P(lcd_buf, PSTR("Hello from RX..."));
	lcd_puts(lcd_buf);
}


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
	static uint8_t cnt = 0;
	char lcd_buf[16];

	if(Flag.reach_1s)
	{
		Flag.reach_1s = 0;
		
		for(i=0;i<17;i++)
		{
			temp_buf[i]=tx_buf[i];
		}
		
		Send_Packet(W_TX_PAYLOAD_NOACK_CMD,temp_buf,17);
		sprintf_P(lcd_buf, PSTR("Sent %d times "), cnt++);
		lcd_gotoxy(0, 1);
		lcd_puts(lcd_buf);
		SwitchToRxMode();  //switch to Rx mode
	}		
}
/**************************************************
Function: Send_Packet
Description:
	fill FIFO to send a packet
Parameter:
	type: WR_TX_PLOAD or  W_TX_PAYLOAD_NOACK_CMD
	pbuf: a buffer pointer
	len: packet length
Return:
	None
**************************************************/
void Send_Packet(uint8_t type,uint8_t* pbuf,uint8_t len)
{
	uint8_t fifo_sta;
	
	SwitchToTxMode();  //switch to tx mode

	fifo_sta=SPI_Read_Reg(FIFO_STATUS);	// read register FIFO_STATUS's value
	if((fifo_sta&FIFO_STATUS_TX_FULL)==0)//if not full, send data (write buff)
	{ 
	  	RED_LED_SET;
		
		SPI_Write_Buf(type, pbuf, len); // Writes data to buffer
		
		_delay_ms(50);
		RED_LED_CLR;
		_delay_ms(50);
	}	  	 	
}
/**************************************************
Function: Receive_Packet
Description:
	read FIFO to read a packet
Parameter:
	None
Return:
	None
**************************************************/
void Receive_Packet(void)
{
	uint8_t len,i,sta,fifo_sta,value,chksum,aa;
	uint8_t rx_buf[MAX_PACKET_LEN];
	static uint16_t cnt = 0;
	static uint16_t cnt2 = 0;
	char lcd_buf[16];

	sta=SPI_Read_Reg(STATUS);	// read register STATUS's value

	if((STATUS_RX_DR&sta) == 0x40)				// if receive data ready (RX_DR) interrupt
	{
		do
		{
			len=SPI_Read_Reg(R_RX_PL_WID_CMD);	// read len

			if(len<=MAX_PACKET_LEN)
			{
				SPI_Read_Buf(RD_RX_PLOAD,rx_buf,len);// read receive payload from RX_FIFO buffer
				sprintf_P(lcd_buf, PSTR("Received %d times"), cnt++);
				lcd_gotoxy(0, 1);
				lcd_puts(lcd_buf);
			}
			else
			{
				SPI_Write_Reg(FLUSH_RX,0);//flush Rx
			}

			fifo_sta=SPI_Read_Reg(FIFO_STATUS);	// read register FIFO_STATUS's value
							
		}while((fifo_sta&FIFO_STATUS_RX_EMPTY)==0); //while not empty
		
		chksum = 0;
		for(i=0;i<16;i++)
		{
			chksum +=rx_buf[i]; 
		}
		if(chksum==rx_buf[16]&&rx_buf[0]==0x30)
		{
			GREEN_LED_SET;
			_delay_ms(50);
			_delay_ms(50);
			GREEN_LED_CLR;

			Send_Packet(W_TX_PAYLOAD_NOACK_CMD,rx_buf,17);
			SwitchToRxMode();//switch to RX mode
			sprintf_P(lcd_buf, PSTR("Received %d CRC"), cnt2);
			lcd_gotoxy(0, 1);
			lcd_puts(lcd_buf);
		}	
	}
	SPI_Write_Reg(WRITE_REG|STATUS,sta);// clear RX_DR or TX_DS or MAX_RT interrupt flag	
}
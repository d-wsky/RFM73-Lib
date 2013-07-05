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

#define CS_LED	   PA2

#define CS_LED_SET  PORTA |= (1 << CS_LED)
#define CS_LED_CLR  PORTA &=~(1 << CS_LED)


unsigned char t1 = 0;

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                            _FDEV_SETUP_WRITE);


const uint8_t tx_buf[17]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x78};
uint8_t rx_buf[RFM73_MAX_PACKET_LEN];

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
    t1 = 1;
	//PORTA ^= 4;
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
	static uint16_t cnt = 0, cnt_good = 0;
	char lcd_buf[16];

	if(t1)
	{
		t1 = 0;
		
		for(i=0;i<17;i++)
		{
			temp_buf[i]=tx_buf[i];
		}
		
		uint8_t res = rfm73_send_packet(RFM73_TX_WITH_ACK, temp_buf, 17);
		if (!res) cnt_good++;
		if (cnt_good==1000) cnt_good=0;
		uint8_t pl=0, rc=0;
		rfm73_observe(&pl, &rc);
		uint8_t ch=rfm73_get_channel();
		sprintf_P(lcd_buf, PSTR("R=%3d;L=%2d;C=%d "), cnt_good, pl, ch);
		lcd_gotoxy(0, 1);
		lcd_puts(lcd_buf);
		temp_buf[RFM73_MAX_PACKET_LEN-1]=0;
		printf_P(PSTR("Sent and received packet %s\nTotal transmittions: %d, no errors for %d times"), temp_buf, cnt, cnt_good);
		rfm73_rx_mode();  //switch to Rx mode
	}	
}

void repaint(uint8_t pwr, uint8_t gain, uint8_t dr) {
	rfm73_set_rf_params(pwr, gain, dr);
	char lcd_buf[19];
	switch (pwr) {
		case 0: 
			sprintf_P(lcd_buf, PSTR("P=-10;"));
			printf_P(PSTR("Out power is -10dBm; "));
			break;
		case 1:
			sprintf_P(lcd_buf, PSTR("P=-5; "));
			printf_P(PSTR("Out power is -5 dBm; "));
			break;
		case 2:
			sprintf_P(lcd_buf, PSTR("P= 0; "));
			printf_P(PSTR("Out power is 0 dBm; "));
			break;
		case 3:
			sprintf_P(lcd_buf, PSTR("P=+5; "));
			printf_P(PSTR("Out power is +5 dBm; "));
			break;
	}
	switch (gain) {
		case 0:
			sprintf_P(lcd_buf+6, PSTR("G=-20;"));
			printf_P(PSTR("LNA gain is -20 dBm; "));
			break;
		case 1:
			sprintf_P(lcd_buf+6, PSTR("G=  0;"));;
			printf_P(PSTR("LNA gain is 0 dBm; "));
			break;
	}
	switch (dr) {
		case 0:
			sprintf_P(lcd_buf+12, PSTR("D= 1"));
			printf_P(PSTR("Datarate is 1 Mbps\n"));
			break;
		case 1:
			sprintf_P(lcd_buf+12, PSTR("D=.3"));
			printf_P(PSTR("Datarate is 0.25 Mbps\n"));
			break;
		case 2:
			sprintf_P(lcd_buf+12, PSTR("D= 2"));
			printf_P(PSTR("Datarate is 2 Mbps"));
			break;
	}	
	lcd_gotoxy(0, 0);
	lcd_puts(lcd_buf);
}

int main(void)
{
	power_on_delay();  
 	init_mcu();
	
	sei();
	
	uint16_t cnt = 0, cnt2 = 0;
	static char lcd_buf[20];
	uint8_t rx_buf[RFM73_MAX_PACKET_LEN];
	uint8_t pwr = RFM73_OUT_PWR_PLUS5DBM;
	uint8_t gain = RFM73_LNA_GAIN_HIGH;
	uint8_t dr = RFM73_DATA_RATE_2MBPS;
	uint8_t pl = 0, rc = 0, cs = 0, len = 0;
	uint8_t ch = 0;
	uint8_t b = 0;

	rfm73_init(pwr, gain, dr, 0x23);
	#ifdef TX_DEVICE
		sprintf_P(lcd_buf, PSTR("Finding receiver"));
		lcd_gotoxy(0, 1);
		lcd_puts(lcd_buf);
		// auto-find first receiver
		RFM73_CE_HIGH;
		rfm73_find_receiver(&ch, &dr);
	#endif
	repaint(pwr, gain, dr);
	while(1)
	{
		RFM73_CE_HIGH;
		// sensing the carrier
		_delay_us(400);
		uint8_t cd = rfm73_carrier_detect();
		if (cd) CS_LED_SET;
		else    CS_LED_CLR;
	#ifdef TX_DEVICE
		sub_program_1hz(); // comment to use in RX
		RFM73_CE_LOW;
	#endif
	#ifdef RX_DEVICE
		_delay_ms(50);
		uint8_t res = rfm73_receive_packet(RFM73_RX_WITH_ACK, rx_buf, &len); // 1 to RX, 0 to TX
		RFM73_CE_LOW;
		// new correct data
		if (res == 0) {
			cs=rfm73_carrier_detect();
			ch=rfm73_get_channel();
			sprintf_P(lcd_buf, PSTR("R=%3d;CS=%1d;C=%d"), ++cnt, cs, ch);
			lcd_gotoxy(0, 1);
			lcd_puts(lcd_buf);
			rx_buf[len] = 0;
			printf_P(PSTR("Received packet: %s\nTotal packets: %d\nCurrent channel: %d\n"), rx_buf, cnt, ch);
			if (cnt==999) cnt = 0;
		};
		// input fifo was flushed
		if (res == 1) {
			rfm73_observe(&pl, &rc);
			sprintf_P(lcd_buf, PSTR("R=%3d FLUSHED!"), ++cnt2);
			lcd_gotoxy(0, 1);
			lcd_puts(lcd_buf);
			rx_buf[len] = 0;
			printf_P(PSTR("Received packet flushed!\nTotal correct packets: %d\n"), cnt);
			if (cnt2==999) cnt2 = 0;
		}
		// no data received
		if (res == 2) {
			cs=rfm73_carrier_detect();
			sprintf_P(lcd_buf, PSTR("R=%3d;CS=%1d;C=%d"), cnt, cs, ch);
			lcd_gotoxy(0, 1);
			lcd_puts(lcd_buf);
			//printf_P(PSTR("Received packet: %s with correct CRC\nTotal correct packets: %d\n"), rx_buf, cnt2);	
		}
	#endif
		uint8_t a = PINA & 0xF8;
		
		if ((a == 0x80) && (b==0)) {
			pwr ++;
			if (pwr == 4) pwr = 0;
			repaint(pwr, gain, dr);
			b = 1;
		}
		if ((a == 0x40) && (b==0)) {
			gain++;
			if (gain == 2) gain = 0;
			repaint(pwr, gain, dr);
			b = 1;
		}
		if ((a == 0x20) && (b==0)) {
			dr++;
			if (dr == 3) dr = 0;
			repaint(pwr, gain, dr);
			b = 1;
		}
		if ((a == 0x10) && (b==0)) {
			ch--;
			if (ch==0xFF) ch=0x7F;
			rfm73_set_channel(ch);
			printf_P(PSTR("Set channel %d\n"), ch);
			b = 1;
		}
		if ((a == 0x08) && (b==0)) {
			ch++;
			if (ch>0x7F) ch=0;
			rfm73_set_channel(ch);
			printf_P(PSTR("Set channel %d\n"), ch);
			b = 1;
		}
		if (a==0) b=0;
	}	
}
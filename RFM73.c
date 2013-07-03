#include "RFM73.h"
#include "spi.h"

#include <util/delay.h>

#define GREEN_LED		   PA0
#define GREEN_LED_SET 	   PORTA |= (1 << GREEN_LED)
#define GREEN_LED_CLR      PORTA &=~(1 << GREEN_LED)

#define RED_LED 		   PA1
#define RED_LED_SET        PORTA |= (1 << RED_LED)
#define RED_LED_CLR        PORTA &=~(1 << RED_LED)


/******************************************************************************
** INTERNAL REGISTER ADDRESSES AND STRUCTRURE                                **
******************************************************************************/

/*****************************************************************************/
/*! \brief Address of configuration register */
#define RFM73_RADR_CONFIG       0x00
/* Configuration register */
/*!< \brief RX/TX control.
	1: PRX
	0: PTX*/
#define CF_PRIM_RX_bf           (0)
#define CF_PRIM_RX_bm           (0x01)
/*!< \brief Power control.
	1: POWER UP
	0:POWER  DOWN*/
#define CF_PWR_UP_bf            (1)
#define CF_PWR_UP_bm            (0x02)
/*!< \brief CRC encoding scheme.
	0 - 1 byte 
	1 - 2 bytes*/
#define CF_CRCO_bf              (2)
#define CF_CRCO_bm              (0x04)
/*!< \brief Enable CRC. Forced high if one of the bits in the EN_AA is high*/
#define CF_EN_CRC_bf            (3)
#define CF_EN_CRC_bm            (0x08)
/*!< \brief Mask interrupt caused by MAX_RT.
	1: Interrupt not reflected on the IRQ pin 
	0: Reflect MAX_RT as active low interrupt on the IRQ pin*/
#define CF_MASK_MAX_RT_bf       (4)
#define CF_MASK_MAX_RT_bm       (0x10)
/*!< \brief Mask interrupt caused by TX_DS.
	1: Interrupt not reflected on the IRQ pin 
	0: Reflect TX_DS as active low interrupt on the IRQ pin */
#define CF_MASK_TX_DS_bf        (5)
#define CF_MASK_TX_DS_bm        (0x20)
/*!< \brief Mask interrupt caused by RX_DR.
	1: Interrupt not reflected on the IRQ pin 
	0: Reflect RX_DR as active low interrupt on the IRQ pin */
#define CF_MASK_RX_DR_bf        (6)
#define CF_MASK_RX_DR_bm        (0x40)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of enable "Auto Acknowledgment" function register */
#define RFM73_RADR_ENAA         0x01
/* Enable "Auto Acknowledgment"  Function */
/*!< \brief Enable auto acknowledgement data pipe 0.*/
#define ENAA_P0_bf              (0)
#define ENAA_P0_bm              (1 << ENAA_P0_bf)
/*!< \brief Enable auto acknowledgement data pipe 1.*/
#define ENAA_P1_bf              (1)
#define ENAA_P1_bm              (1 << ENAA_P1_bf)
/*!< \brief Enable auto acknowledgement data pipe 2.*/
#define ENAA_P2_bf              (2)
#define ENAA_P2_bm              (1 << ENAA_P2_bf)
/*!< \brief Enable auto acknowledgement data pipe 3.*/
#define ENAA_P3_bf              (3)
#define ENAA_P3_bm              (1 << ENAA_P3_bf)
/*!< \brief Enable auto acknowledgement data pipe 4.*/
#define ENAA_P4_bf              (4)
#define ENAA_P4_bm              (1 << ENAA_P4_bf)
/*!< \brief Enable auto acknowledgement data pipe 5.*/
#define ENAA_P5_bf              (5)
#define ENAA_P5_bm              (1 << ENAA_P5_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of enabled RX addresses register */
#define RFM73_RADR_EN_RX_ADDR   0x02
/* Enabled RX Addresses */
/*!< \brief Enable data pipe 0.*/
#define ERX_P0_bf               (0)
#define ERX_P0_bm               (1 << ERX_P0_bf)
/*!< \brief Enable data pipe 0.*/
#define ERX_P1_bf               (1)
#define ERX_P1_bm               (1 << ERX_P0_bf)
/*!< \brief Enable data pipe 2.*/
#define ERX_P2_bf               (2)
#define ERX_P2_bm               (1 << ERX_P0_bf)
/*!< \brief Enable data pipe 3.*/
#define ERX_P3_bf               (3)
#define ERX_P3_bm               (1 << ERX_P0_bf)
/*!< \brief Enable data pipe 4.*/
#define ERX_P4_bf               (4)
#define ERX_P4_bm               (1 << ERX_P0_bf)
/*!< \brief Enable data pipe 5.*/
#define ERX_P5_bf               (5)
#define ERX_P5_bm               (1 << ERX_P0_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of setup of address widths (common for all data pipes)
register */
#define RFM73_RADR_SETUP_AW     0x03
/* Setup of Address Widths (common for all data pipes) */
/*!< \brief RX/TX Address field width. LSB bytes are used if address width is 
below 5 bytes:
	'00' - Illegal
	'01' - 3 bytes
	'10' - 4 bytes
	'11' - 5 bytes (default) */
#define SA_AW_bf                (0)
#define SA_AW_bm                (1 << SA_AW_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of setup of automatic retransmission register */
#define RFM73_RADR_SETUP_RETR   0x04
/* Setup of Automatic Retransmission */
/*!< \brief Auto Retransmission Count.
	"0000" – Re-Transmit disabled
	"0001" – Up to 1 Re-Transmission on fail of AA
	...
	"1111" – Up to 15 Re-Transmission on fail of AA */
#define SR_ARC_bf               (0)
#define SR_ARC_bm               (1 << SR_ARC_bf)
/*!< \brief Auto Retransmission Delay.
	"0000" – Wait 250 us
	"0001" – Wait 500 us
	"0010" – Wait 750 us
	...
	"1111" – Wait 4000 us
    (Delay defined from end of transmission to start of next transmission)*/
#define SR_ARD_bf               (4)
#define SR_ARD_bm               (1 << SR_ARD_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of RF channel register */
#define RFM73_RADR_RF_CH        0x05
/* RF Channel */
/*!< \brief Sets the frequency channel (0b000010 default).*/
#define RF_CH_bf                (0)
#define RF_CH_bm                (1 << RF_CH_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of RF setup register */
#define RFM73_RADR_RF_SETUP     0x06
/*  RF Setup Register */
/*!< \brief Setup LNA gain
	0: Low gain(20dB down)
	1: High gain.*/
#define RS_LNA_HCURR_bf         (0)
#define RS_LNA_HCURR_bm         (0x01)
/*!< \brief Set RF output power in TX mode.
	00: -10 dBm
	01: -5 dBm
    10: 0 dBm
	11: 5 dBm */
#define RS_RF_PWR_bf            (1)
#define RS_RF_PWR_bm            (0x06)
/*!< \brief Set Air Data Rate. Encoding:
	RF_DR_HIGH, RF_DR_LOW:
	00: 1Mbps
	01: 250Kbps
	10: 2Mbps (default)
	11: 2Mbps */
#define RS_RF_DR_HIGH_bf        (3)
#define RS_RF_DR_HIGH_bm        (0x08)
/*!< \brief Force PLL lock signal. Only used in test (default is 0).*/
#define RS_PLL_LOCK_bf          (4)
#define RS_PLL_LOCK_bm          (0x10)
/*!< \brief Set Air Data Rate. See RF_DR_HIGH for encoding */
#define RS_RF_DR_LOW_bf         (5)
#define RS_RF_DR_LOW_bm         (0x20)
#define RS_RF_DR_bm             (0x28)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of status register (In parallel to the SPI command word
applied on the MOSI pin, the STATUS register is shifted serially out 
on the MISO pin) */
#define RFM73_RADR_STATUS       0x07
/*!< \brief TX FIFO full flag. 
	1: TX FIFO full 
	0: Available locations in TX FIFO*/
#define ST_TX_FULL_bf           (0)
#define ST_TX_FULL_bm           (0x01)
/*!< \brief Data pipe number for the payload available for reading from
RX_FIFO:
	000-101: Data Pipe Number 
	110: Not used 
	111: RX FIFO Empty*/
#define ST_RX_P_NO_bf           (1)
#define ST_RX_P_NO_bm           (0x0E)
/*!< \brief  Maximum number of TX retransmits interrupt. Write 1 to clear bit.
If MAX_RT is asserted it must be cleared to enable further communication.*/
#define ST_MAX_RT_bf            (4)
#define ST_MAX_RT_bm            (0x10)
/*!< \brief   Data Sent TX FIFO interrupt. Asserted when packet transmitted on
TX. If AUTO_ACK is activated, this bit is set high only when ACK is received.
Write 1 to clear bit.*/
#define ST_TX_DS_bf             (5)
#define ST_TX_DS_bm             (0x20)
/*!< \brief   Data Ready RX FIFO interrupt. Asserted when new data arrives RX
FIFO. Write 1 to clear bit.*/
#define ST_RX_DR_bf             (6)
#define ST_RX_DR_bm             (0x40)
 /*!< \brief   Register bank selection states. Switch register bank is done by
SPI command "ACTIVATE"  followed by 0x53
	0: Register bank 0
	1: Register bank 1*/
#define ST_RBANK_bf             (7)
#define ST_RBANK_bm             (0x80)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of transmit observe register */
#define RFM73_RADR_OBSERVE_TX   0x08
/* Transmit observe register */
/*!< \brief Count retransmitted packets. The counter is reset when
transmission of a new packet starts. */
#define OT_ARC_CNT_bf           (0)
#define OT_ARC_CNT_bm           (1 << OT_ARC_CNT_bf)
/*!< \brief Count lost packets. The counter is overflow protected to 15, and
discontinues at max until reset. The counter is reset by writing to RF_CH.*/
#define OT_PLOS_CNT_bf          (4)
#define OT_PLOS_CNT_bm          (1 << OT_PLOS_CNT_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of carrier detect register */
#define RFM73_RADR_CD           0x09
/* CD - carrier detect */
/*!< \brief Carrier detect */
#define CD_bf                   (0)
#define CD_bm                   (1 << CD_bf)
/*****************************************************************************/

/*! \brief Address of receive address data pipe 0 register. 5 Bytes maximum
length, default is 0xE7E7E7E7E7. LSB byte is written first. Write the number
of bytes defined by SETUP_AW).*/
#define RFM73_RADR_RX_ADDR_P0   0x0A
/*! \brief Address of receive address data pipe 1 register. 5 Bytes maximum
length, default is 0xC2C2C2C2C2. LSB byte is written first. Write the number
of bytes defined by SETUP_AW).*/
#define RFM73_RADR_RX_ADDR_P1   0x0B
/*! \brief Address of receive address data pipe 2 register. 1 byte maximum,
default is 0xC3. Only LSB, MSB bytes is equal to RX_ADDR_P1[39:8]*/
#define RFM73_RADR_RX_ADDR_P2   0x0C
/*! \brief Address of receive address data pipe 3 register. 1 byte maximum,
default is 0xC3. Only LSB, MSB bytes is equal to RX_ADDR_P1[39:8]*/
#define RFM73_RADR_RX_ADDR_P3   0x0D
/*! \brief Address of receive address data pipe 4 register. 1 byte maximum,
default is 0xC3. Only LSB, MSB bytes is equal to RX_ADDR_P1[39:8]*/
#define RFM73_RADR_RX_ADDR_P4   0x0E
/*! \brief Address of receive address data pipe 5 register. 1 byte maximum,
default is 0xC3. Only LSB, MSB bytes is equal to RX_ADDR_P1[39:8]*/
#define RFM73_RADR_RX_ADDR_P5   0x0F

/*! \brief Address of transmit address register */
#define RFM73_RADR_TX_ADDR      0x10

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 0 (1 to 32 bytes), 0 is default. */
#define RFM73_RX_PW_P0          0x11
#define RX_PW_P0_bf             (0)
#define RX_PW_P0_bm             (1 << RX_PW_P0_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 1 (1 to 32 bytes), 0 is default. */
#define RFM73_RX_PW_P1          0x12
#define RX_PW_P1_bf             (0)
#define RX_PW_P1_bm             (1 << RX_PW_P1_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 2 (1 to 32 bytes), 0 is default. */
#define RFM73_RX_PW_P2          0x13
#define RX_PW_P2_bf             (0)
#define RX_PW_P2_bm             (1 << RX_PW_P2_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 3 (1 to 32 bytes), 0 is default. */
#define RFM73_RX_PW_P3          0x14
#define RX_PW_P3_bf             (0)
#define RX_PW_P3_bm             (1 << RX_PW_P3_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 0 (1 to 32 bytes), 0 is default. */
#define RFM73_RX_PW_P4          0x15
#define RX_PW_P4_bf             (0)
#define RX_PW_P4_bm             (1 << RX_PW_P4_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 5 (1 to 32 bytes), 0 is default. */
#define RFM73_RX_PW_P5          0x16
#define RX_PW_P5_bf             (0)
#define RX_PW_P5_bm             (1 << RX_PW_P5_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of FIFO status register */
#define RFM73_RADR_FIFO_STATUS  0x17

/*!< \brief RX FIFO empty flag.
	1: RX FIFO empty 
	0: Data in RX FIFO */
#define FS_RX_EMPTY_bf          (0)
#define FS_RX_EMPTY_bm          (1 << FS_RX_EMPTY_bf)
/*!< \brief RX FIFO full flag.
	1: RX FIFO full 
	0: Available locations in RX FIFO */
#define FS_RX_FULL_bf           (1)
#define FS_RX_FULL_bm           (1 << FS_RX_FULL_bf)
/*!< \brief TX FIFO empty flag.
	1: TX FIFO empty 
    0: Data in TX FIFO */
#define FS_TX_EMPTY_bf          (4)
#define FS_TX_EMPTY_bm          (1 << FS_TX_EMPTY_bf)
/*!< \brief TX FIFO full flag.
	1: TX FIFO full;
	0: Available locations in TX FIFO */
#define FS_TX_FULL_bf           (5)
#define FS_TX_FULL_bm           (1 << FS_TX_FULL_bf)
/*!< \brief Reuse last transmitted data packet if set high.
The packet is repeatedly retransmitted  as long as CE is high.
TX_REUSE is set by the SPI command REUSE_TX_PL, and is reset
by the SPI command W_TX_PAYLOAD or FLUSH TX */
#define FS_TX_REUSE_bf          (6)
#define FS_TX_REUSE_bm          (1 << FS_TX_REUSE_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of enable dynamic payload length register.*/
#define RFM73_RADR_DYNPD        0x1C
/*! \brief Enable dynamic payload length data pipe 0. (Requires EN_DPL and
ENAA_P0).*/
#define DPL_P0_bf               (0)
#define DPL_P0_bm               (1 << DPL_P0_bf)
/*! \brief Enable dynamic payload length data pipe 1. (Requires EN_DPL and
ENAA_P1).*/
#define DPL_P1_bf               (1)
#define DPL_P1_bm               (1 << DPL_P1_bf)
/*! \brief Enable dynamic payload length data pipe 2. (Requires EN_DPL and
ENAA_P2).*/
#define DPL_P2_bf               (2)
#define DPL_P2_bm               (1 << DPL_P2_bf)
/*! \brief Enable dynamic payload length data pipe 3. (Requires EN_DPL and
ENAA_P3).*/
#define DPL_P3_bf               (3)
#define DPL_P3_bm               (1 << DPL_P3_bf)
/*! \brief Enable dynamic payload length data pipe 4. (Requires EN_DPL and
ENAA_P4).*/
#define DPL_P4_bf               (4)
#define DPL_P4_bm               (1 << DPL_P4_bf)
/*! \brief Enable dynamic payload length data pipe 5. (Requires EN_DPL and
ENAA_P5).*/
#define DPL_P5_bf               (5)
#define DPL_P5_bm               (1 << DPL_P5_bf)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of feature register */
#define RFM73_RADR_FEATURE      0x1D
/* Feature Register */
/*!< \brief Enables the W_TX_PAYLOAD_NOACK command.*/
#define FE_EN_DYN_ACK_bf        0
#define FE_EN_DYN_ACK_bm        (1 << FE_EN_DYN_ACK_bf)
/*!< \brief Enables Payload with ACK */
#define FE_EN_ACK_PAY_bf        1
#define FE_EN_ACK_PAY_bm        (1 << FE_EN_ACK_PAY_bf)
/*!< \brief Enables Dynamic Payload Length */
#define FE_EN_DPL_bf            2
#define FE_EN_DPL_bm            (1 << FE_EN_DPL_bf)
/*****************************************************************************/

/******************************************************************************
**  COMMANDS                                                                 **
******************************************************************************/

/*! \brief Read command and status registers in format 000AAAAA. AAAAA = 
5 bit Register Map Address */
#define RFM73_CMD_R_REGISTER          0b00000000
/*! \brief Write command and status registers. AAAAA = 5 bit Register Map
Address. Executable in power down or standby modes only.*/
#define RFM73_CMD_W_REGISTER          0b00100000
/*! \brief Activate transmit?

This write command  followed by data 0x73 activates the following
features: 
    
	- R_RX_PL_WID 
    - W_ACK_PAYLOAD 
    - W_TX_PAYLOAD_NOACK 
 
A new ACTIVATE command with the same data deactivates them again.
This is executable in power down or stand by modes only. 
 
The R_RX_PL_WID,  W_ACK_PAYLOAD, and W_TX_PAYLOAD_NOACK features
registers are initially in a deactivated state; a write has no
effect, a read only results in zeros on MISO. To activate these 
registers, use the ACTIVATE command followed by data 0x73.
Then they can be accessed as any other register. Use the same
command and data to deactivate the registers again. 
 
This write command followed by data 0x53 toggles the register
bank, and the current register bank number can be read out from
STATUS register.*/
#define RFM73_CMD_ACTIVATE            0b01010000
/* Read RX-payload  width for the top R_RX_PAYLOAD in the RX FIFO. */
#define RFM73_CMD_R_RX_PL_WID         0b01100000  
/* Write TX-payload: 1 – 32 bytes. A write operation always starts
at byte 0 used in TX payload.*/
#define RFM73_CMD_W_TX_PAYLOAD        0b10100000
/* Read RX-payload:  1 – 32 bytes. A read operation always starts
at byte 0. Payload is deleted from FIFO after it is read. Used in
RX mode. 1 to 32 byte, LSB first */
#define RFM73_CMD_R_RX_PAYLOAD        0b01100001  
/* Used in TX mode. Transmits packet with disabled AUTOACK. */
#define RFM73_CMD_W_TX_PAYLOAD_NOACK  0b10110000  
/* Flush TX FIFO, used in TX mode */
#define RFM73_CMD_FLUSH_TX            0b11100001
/* Flush RX FIFO, used in RX mode. Should not be executed during
transmission of acknowledge because acknowledge package will not 
be completed. */
#define RFM73_CMD_FLUSH_RX            0b11100010
/* Used for a PTX device. Reuse last transmitted payload. Packets
are repeatedly retransmitted as long as CE is high. TX payload reuse is
active until W_TX_PAYLOAD or FLUSH TX is executed. TX payload reuse must
not be activated or deactivated during package transmission  */
#define RFM73_CMD_REUSE_TX_PL         0b11100011  
/* No Operation. Might be used to read the STATUS 
register. */
#define RFM73_CMD_NOP                 0b11111111


/* Alternative API
#define UINT40_T(regname) \ 
	union { \ 
		unsigned char regname[5]; \ 
		struct { \ 
			unsigned char byte0; \ 
			unsigned char byte1; \ 
			unsigned char byte2; \ 
			unsigned char byte3; \ 
			unsigned char byte4; \
		}; \ 
	}



typedef struct rfm73_conf1 {
	uint8_t  CONFIG;
	uint8_t  EN_AA;
	uint8_t  EN_RX_ADDR;
	uint8_t  SETUP_AW;
	uint8_t  SETUP_RETR;
	uint8_t  RF_CH;
	uint8_t  RF_SETUP;
	uint8_t  STATUS;
	uint8_t  OBSERVE_TX;
	uint8_t  CD;
	UINT40_T RX_ADDR_P0;
	UINT40_T RX_ADDR_P1;
} rfm73_conf_t;
*/

//Bank1 register initialization value

//In the array RegArrFSKAnalog,all the register value is the byte reversed!!!!!!!!!!!!!!!!!!!!!
const unsigned long Bank1_Reg0_13[]={       //latest config txt
0xE2014B40,
0x00004BC0,
0x028CFCD0,
0x41390099,
0x1B8296d9,
0xA67F0224,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00127300,
0x46B48000,
};

uint8_t Bank1_Reg14[]=
{
	0x41,0x20,0x08,0x04,0x81,0x20,0xCF,0xF7,0xFE,0xFF,0xFF
};

//Bank0 register initialization value
uint8_t Bank0_Reg[][2]={

	//reflect RX_DR\TX_DS\MAX_RT,Enable CRC ,2byte,POWER UP,PRX	
	{RFM73_RADR_CONFIG, (CF_EN_CRC_bm | CF_CRCO_bm | CF_PWR_UP_bm |
						 CF_PRIM_RX_bm)},
	//Enable auto acknowledgement data pipe5\4\3\2\1\0
	{RFM73_RADR_ENAA,   (ENAA_P5_bm | ENAA_P4_bm | ENAA_P3_bm |
						 ENAA_P2_bm | ENAA_P1_bm | ENAA_P0_bm)},
	//Enable RX Addresses pipe5\4\3\2\1\0
	{RFM73_RADR_EN_RX_ADDR, (ERX_P5_bm | ERX_P4_bm | ERX_P3_bm |
							 ERX_P2_bm | ERX_P1_bm | ERX_P0_bm)},
	//RX/TX address field width 5byte
	{RFM73_RADR_SETUP_AW, (3 << SA_AW_bf)},
	//auto retransmission delay (4000us),auto retransmission count(15)
	{RFM73_RADR_SETUP_RETR, (0xF << SR_ARD_bf) | (0xF << SR_ARC_bf)},
	//23 channel
	{RFM73_RADR_RF_CH, (0x17 << RF_CH_bf)},
	//air data rate-1M,out power 0dbm,setup LNA gain \bit4 must set up to 0
	{RFM73_RADR_RF_SETUP, ((3 << RS_RF_PWR_bf) | RS_LNA_HCURR_bm)},
	// data pipe number = 3
	{RFM73_RADR_STATUS, ((3 << ST_RX_P_NO_bf) | ST_TX_FULL_bm)},
	//
	{RFM73_RADR_OBSERVE_TX, 0x00},
	{RFM73_RADR_CD, 0x00},
	//only LSB Receive address data pipe 2, MSB bytes is equal to RX_ADDR_P1
	{RFM73_RADR_RX_ADDR_P2, 0xc3},
	//only LSB Receive address data pipe 3, MSB bytes is equal to RX_ADDR_P1
	{RFM73_RADR_RX_ADDR_P3, 0xc4},
	//only LSB Receive address data pipe 4, MSB bytes is equal to RX_ADDR_P1
	{RFM73_RADR_RX_ADDR_P4, 0xc5},
	//only LSB Receive address data pipe 5, MSB bytes is equal to RX_ADDR_P1
	{RFM73_RADR_RX_ADDR_P5, 0xc6},
	//Number of bytes in RX payload in data pipe0(32 byte)
	{RFM73_RX_PW_P0,0x20}, 
	//Number of bytes in RX payload in data pipe1(32 byte)
	{RFM73_RX_PW_P1,0x20},
	//Number of bytes in RX payload in data pipe2(32 byte)
	{RFM73_RX_PW_P2,0x20},
	//Number of bytes in RX payload in data pipe3(32 byte)
	{RFM73_RX_PW_P3,0x20},
	//Number of bytes in RX payload in data pipe4(32 byte)
	{RFM73_RX_PW_P4,0x20},
	//Number of bytes in RX payload in data pipe5(32 byte)
	{RFM73_RX_PW_P5,0x20},
	//fifo status
	{RFM73_RADR_FIFO_STATUS,0x00},
	//Enable dynamic payload length data pipe5\4\3\2\1\0
	{RFM73_RADR_DYNPD, (DPL_P5_bm | DPL_P4_bm | DPL_P3_bm | DPL_P2_bm |
                        DPL_P1_bm |	DPL_P0_bm)},
	//Enables Dynamic Payload Length,Enables Payload with ACK,Enables the
	//W_TX_PAYLOAD_NOACK command
	{RFM73_RADR_FEATURE, (FE_EN_DYN_ACK_bm | FE_EN_ACK_PAY_bm | FE_EN_DPL_bm)}
};

//Receive address data pipe 0
const uint8_t RX0_Address[]={0x34,0x43,0x10,0x10,0x01};
//Receive address data pipe 1
const uint8_t RX1_Address[]={0x39,0x38,0x37,0x36,0xc2};

/**************************************************         
Function: _rfm73_write_reg();                                  
                                                            
Description:                                                
	Writes value 'value' to register 'reg'              
**************************************************/        
void _rfm73_write_reg(uint8_t reg, uint8_t value) {
	// CSN low, init SPI transaction
	RFM73_CSN_LOW;
	// select register
	spi_read(reg);
	// ..and write value to it..
	spi_read(value);
	// CSN high again
	RFM73_CSN_HIGH;
}
/**************************************************/        
                                                            
/**************************************************         
Function: _rfm73_read_reg();                                   
                                                            
Description:                                                
	Read one UINT8 from BK2421 register, 'reg'           
**************************************************/        
uint8_t _rfm73_read_reg(uint8_t reg) {        
	uint8_t value;
	// CSN low, initialize SPI communication...
	RFM73_CSN_LOW;
	// Select register to read from..
	spi_read(reg);
	// ..then read register value
	value = spi_read(0);
	// CSN high, terminate SPI communication
	RFM73_CSN_HIGH;
    // return register value
	return(value);
}
                                                            
/**************************************************         
Function: _rfm73_read_buf();                                   
                                                            
Description:                                                
	Reads 'length' #of length from register 'reg'         
**************************************************/        
void _rfm73_read_buf(uint8_t reg, uint8_t *pBuf, uint8_t length) {                                
	uint8_t status,byte_ctr;
	// Set CSN low
	RFM73_CSN_LOW;
	// Select register to write, and read status UINT8
	status = spi_read(reg);
	      
	for(byte_ctr=0;byte_ctr<length;byte_ctr++)
		// Perform SPI_RW to read UINT8 from RFM73 
		pBuf[byte_ctr] = spi_read(0);
	
	// Set CSN high again
	RFM73_CSN_HIGH;
}
                                                            
/**************************************************         
Function: _rfm73_write_buf();                                  
                                                            
Description:                                                
	Writes contents of buffer '*pBuf' to RFM73         
**************************************************/        
void _rfm73_write_buf(uint8_t reg, uint8_t *pBuf, uint8_t length) {
	uint8_t byte_ctr;
	
	// Set CSN low, init SPI tranaction
	RFM73_CSN_LOW;
	// Select register to write to and read status UINT8
	spi_read(reg);
	// then write all UINT8 in buffer(*pBuf) 
	for(byte_ctr=0; byte_ctr<length; byte_ctr++)
		spi_read(*(pBuf++));
	// Set CSN high again
	RFM73_CSN_HIGH;
}

/**************************************************
Function: rfm73_rx_mode();
Description:
	switch to Rx mode
**************************************************/
void rfm73_rx_mode()
{
	uint8_t value;
	//flush Rx
	_rfm73_write_reg(RFM73_CMD_FLUSH_RX,0);
	// read register STATUS's value
	value=_rfm73_read_reg(RFM73_RADR_STATUS);
	// clear RX_DR or TX_DS or MAX_RT interrupt flag
	_rfm73_write_reg(RFM73_CMD_W_REGISTER|RFM73_RADR_STATUS,value);

	RFM73_CE_LOW;
	// read register CONFIG's value
	value=_rfm73_read_reg(RFM73_RADR_CONFIG);
	//PRX set bit 1
	value=value|0x01;
	// Set PWR_UP bit, enable CRC(2 length) & Prim:RX. RX_DR enabled..
  	_rfm73_write_reg(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, value); 

	RFM73_CE_HIGH;
}

/**************************************************
Function: rfm73_tx_mode();
Description:
	switch to Tx mode
**************************************************/
void rfm73_tx_mode()
{
	uint8_t value;
	//flush Tx
	_rfm73_write_reg(RFM73_CMD_FLUSH_TX,0);

	RFM73_CE_LOW;
	// read register CONFIG's value
	value=_rfm73_read_reg(RFM73_RADR_CONFIG);	
    //PTX set bit 0
	value=value&0xfe;
	// Set PWR_UP bit, enable CRC(2 length) & Prim:RX. RX_DR enabled.
  	_rfm73_write_reg(RFM73_CMD_W_REGISTER|RFM73_RADR_CONFIG, value); 
	
	RFM73_CE_HIGH;
}

/**************************************************
Function: _rfm73_toggle_reg_bank();
                                                            
Description:
	 access switch between Bank1 and Bank0 

Parameter:
	rbank      1:register bank1
	          0:register bank0
Return:
     None
**************************************************/
void _rfm73_toggle_reg_bank(uint8_t rbank) {
	uint8_t temp;

	temp=_rfm73_read_reg(7);
	temp=temp&0x80;

	if( ( (temp)&&(rbank==0) )
	||( ((temp)==0)&&(rbank) ) ) {
		_rfm73_write_reg(RFM73_CMD_ACTIVATE,0x53);
	}
}

void rfm73_power_up() {
	uint8_t conf = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	// set CF_PWR_UP bit high
	conf |= CF_PWR_UP_bm;
	_rfm73_write_reg(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, conf);
	// power up delay
	_delay_ms(3);
}

void rfm73_power_down() {
	uint8_t conf = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	// set CF_PWR_UP bit low
	conf &=~CF_PWR_UP_bm;
	_rfm73_write_reg(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, conf);
}

/**************************************************
Function: rfm73_set_channel();
Description:
	set channel number

**************************************************/
void rfm73_set_channel(uint8_t ch)
{
	_rfm73_write_reg((uint8_t)(RFM73_CMD_W_REGISTER|RFM73_RADR_RF_CH),
	                (uint8_t)(ch));
}

void rfm73_set_rf_params(uint8_t out_pwr, uint8_t lna_gain, uint8_t data_rate) {
	volatile uint8_t c;
	// read config
	c = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_RF_SETUP);
	// clear all used bits
	c &=~(RS_RF_DR_bm | RS_LNA_HCURR_bm | RS_RF_PWR_bm);
	// set appropriate bits
	c |= (out_pwr & 3) << RS_RF_PWR_bf;
	c |= (lna_gain & 1) << RS_LNA_HCURR_bf;
    c |= (((data_rate & 2) >> 1) << RS_RF_DR_HIGH_bf) |
	     ((data_rate & 1) << RS_RF_DR_LOW_bf);
	// write config
	_rfm73_write_reg(RFM73_CMD_W_REGISTER | RFM73_RADR_RF_SETUP, c);	
}

void rfm73_set_address_width(uint8_t aw) {
	uint8_t c = aw & 3;
	// '00' is illegal
	if (c==0) c = 1;
	_rfm73_write_reg(RFM73_CMD_W_REGISTER|RFM73_RADR_SETUP_AW, c);
}

void rfm73_set_autort(uint16_t rt_time, uint8_t rt_count) {
	uint8_t time = rt_time/250-1;
	uint8_t count = rt_count;
	// set up constrains
	if (rt_time<250) time = 0;
	if (rt_time>4000) time = 15;
	if (rt_count>15) count = 15;
	_rfm73_write_reg(RFM73_CMD_W_REGISTER | RFM73_RADR_SETUP_RETR, (time << 4) | count);
}

void rfm73_mask_int(uint8_t mask_rx_dr, uint8_t mask_tx_ds, uint8_t mask_max_rt) {
	// read current state
	uint8_t c = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	// clear mask bits
	c &=~(CF_MASK_MAX_RT_bm | CF_MASK_RX_DR_bm | CF_MASK_TX_DS_bm);
	// set appropriate mask bits
	c |= (((mask_rx_dr & 1) << CF_MASK_RX_DR_bf) |
	      ((mask_tx_ds & 1) << CF_MASK_TX_DS_bf) |
		  ((mask_max_rt& 1) << CF_MASK_MAX_RT_bf));
	// write new config
	_rfm73_write_reg(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, c);
}

void rfm73_set_tx_addr(uint8_t* addr) {
	// get address lenght
	uint8_t addr_len = 
	    _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_SETUP_AW);
	// set new addr
	_rfm73_write_buf((RFM73_CMD_W_REGISTER | RFM73_RADR_TX_ADDR),
	                 addr, addr_len+2);
}

void rfm73_set_rx_addr_p0(uint8_t* addr) {
	// get address lenght
	uint8_t addr_len = 
	    _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_SETUP_AW);
	// set new addr
	_rfm73_write_buf((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P0),
	                 addr, addr_len+2);
}

void rfm73_set_rx_addr_p1(uint8_t* addr) {
	// get address lenght
	uint8_t addr_len = 
	    _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_SETUP_AW);
	// set new addr
	_rfm73_write_buf((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P1),
	                 addr, addr_len+2);
}

void rfm73_set_rx_addr_p2(uint8_t addr) {
	_rfm73_write_reg((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P2), addr);
}

void rfm73_set_rx_addr_p3(uint8_t addr) {
	_rfm73_write_reg((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P3), addr);
}

void rfm73_set_rx_addr_p4(uint8_t addr) {
	_rfm73_write_reg((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P4), addr);
}

void rfm73_set_rx_addr_p5(uint8_t addr) {
	_rfm73_write_reg((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P5), addr);
}

/*! \brief This function is used to init RFM73 module and to set all parameters
to some values.*/
void rfm73_init(uint8_t out_pwr, uint8_t lna_gain, uint8_t data_rate) {
	uint8_t i,j;
 	uint8_t WriteArr[12];

	_delay_ms(200);
	
	_rfm73_toggle_reg_bank(0);

	// set output power										 
	for(i=0;i<20;i++) {
		_rfm73_write_reg((RFM73_CMD_W_REGISTER|Bank0_Reg[i][0]),Bank0_Reg[i][1]);
	}
	
	rfm73_set_rf_params(out_pwr, lna_gain, data_rate);
	rfm73_set_address_width(RFM73_ADR_WID_5BYTES);
	rfm73_set_autort(4000, 15);
	rfm73_mask_int(0, 0, 1);
	
	//reg 10 - Rx0 addr
	//_rfm73_write_buf((RFM73_CMD_W_REGISTER|RFM73_RADR_RX_ADDR_P0),(uint8_t*)(&RX0_Address),5);
	rfm73_set_rx_addr_p0((uint8_t*)(&RX0_Address));
	
	//REG 11 - Rx1 addr
	//_rfm73_write_buf((RFM73_CMD_W_REGISTER|RFM73_RADR_RX_ADDR_P1),(uint8_t*)(&RX1_Address),5);
	rfm73_set_rx_addr_p1((uint8_t*)(&RX1_Address));

	//REG 16 - TX addr
	//_rfm73_write_buf((RFM73_CMD_W_REGISTER|RFM73_RADR_TX_ADDR),(uint8_t*)(&RX0_Address),5);
	rfm73_set_tx_addr((uint8_t*)(&RX0_Address));

	//read Feature Register Payload With ACK ACTIVATE Payload With ACK (REG28,REG29).
	i = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_FEATURE);
	if (i==0) 
		_rfm73_write_reg(RFM73_CMD_ACTIVATE,0x73); // Active
	// i!=0 showed that chip has been actived.so do not active again.
	
	for(i=22;i>=21;i--) {
		_rfm73_write_reg((RFM73_CMD_W_REGISTER|Bank0_Reg[i][0]),Bank0_Reg[i][1]);
	}
	
	// write bank1 registers
	_rfm73_toggle_reg_bank(1);
	
	// reversed order of bytes
	for(i=0;i<=8;i++){
		for(j=0;j<4;j++)
			WriteArr[j]=(Bank1_Reg0_13[i]>>(8*(j) ) )&0xff;

		_rfm73_write_buf((RFM73_CMD_W_REGISTER|i),&(WriteArr[0]),4);
	}

	for(i=9;i<=13;i++) {
		for(j=0;j<4;j++)
			WriteArr[j]=(Bank1_Reg0_13[i]>>(8*(3-j) ) )&0xff;

		_rfm73_write_buf((RFM73_CMD_W_REGISTER|i),&(WriteArr[0]),4);
	}

	//_rfm73_write_buf((RFM73_CMD_W_REGISTER|14),&(Bank1_Reg14[0]),11);
	for(j=0;j<11;j++) {
		WriteArr[j]=Bank1_Reg14[j];
	}
	_rfm73_write_buf((RFM73_CMD_W_REGISTER|14),&(WriteArr[0]),11);

	//toggle REG4<25,26>
	for(j=0;j<4;j++)
		//WriteArr[j]=(RegArrFSKAnalog[4]>>(8*(j) ) )&0xff;
		WriteArr[j]=(Bank1_Reg0_13[4]>>(8*(j) ) )&0xff;

	WriteArr[0]=WriteArr[0]|0x06;
	_rfm73_write_buf((RFM73_CMD_W_REGISTER|4),&(WriteArr[0]),4);

	WriteArr[0]=WriteArr[0]&0xf9;
	_rfm73_write_buf((RFM73_CMD_W_REGISTER|4),&(WriteArr[0]),4);

	_delay_ms(50);
	
	_rfm73_toggle_reg_bank(0);
	rfm73_rx_mode();
}


/*! \brief This function is used to get new packet from FIFO buffer.

\param type - #RFM73_RX_WITH_ACK if explicit acknowledge of the packet is
              needed;
			  #RFM73_RX_WITH_NOACK to only get data from FIFO buffer.
\param data_buf - pointer to start of the input buffer;
\param len  - length of received packet (this data is read from RFM73,
              and does not exceed 32).
			  
\return 0 - if received data is correct;
        1 - if #RFM73_MAX_PACKET_LEN is exceeded, input FIFO buffer is flushed,
		    data_buf unchanged;
	    2 - if flag RX_DR (receive data ready) doesn't raised.
		
\warning Possible hangouts in certain conditions.*/
uint8_t rfm73_receive_packet(uint8_t type, uint8_t* data_buf, uint8_t* len) {
	uint8_t sta,fifo_sta;
	uint8_t result = 0;
	
	// read register STATUS's value
	sta=_rfm73_read_reg(RFM73_CMD_R_REGISTER|RFM73_RADR_STATUS);
	
	// if receive data ready (RX_DR) interrupt
	if(sta & ST_RX_DR_bm) {
		do {
			// read len
			*len=_rfm73_read_reg(RFM73_CMD_R_RX_PL_WID);	

			if(*len<=RFM73_MAX_PACKET_LEN) {
				// read receive payload from RX_FIFO buffer
				_rfm73_read_buf(RFM73_CMD_R_RX_PAYLOAD, data_buf, *len);
				// return "some data received"
				result = 0;
			}
			else {
				//flush Rx
				*len = 0;
				_rfm73_write_reg(RFM73_CMD_FLUSH_RX,0);
				// return "data was flushed"
				result = 1;
			}
			// read register FIFO_STATUS's value
			fifo_sta=_rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_FIFO_STATUS);			
		} while ((fifo_sta&FS_RX_EMPTY_bm)==0); //while not empty
		
		GREEN_LED_SET;
		if (type == RFM73_RX_WITH_ACK) {
			rfm73_send_packet(RFM73_CMD_W_TX_PAYLOAD_NOACK, data_buf, *len); // Uncomment to use in RX
		}
		GREEN_LED_CLR;
		//switch to RX mode
		rfm73_rx_mode();
	}
	else {
		// return "no data received"
		result = 2;
	}

	// clear RX_DR or TX_DS or MAX_RT interrupt flag
	_rfm73_write_reg(RFM73_CMD_W_REGISTER|RFM73_RADR_STATUS,sta);
	
	return result;
}

/*! \brief This function fill up transmit buffer and sends data.

Data is sent only if FIFO buffer is empty.

\param type - #RFM73_TX_WITH_ACK send package with auto-acknowledge, guarantee
              delivery status;
              #RFM73_TX_WITH_NOACK - send package without autoacknowledge,
			  delivery status is unknown;
\param pbuf - pointer to RAM-buffer to be sent;
\param len  - length of data to be sent. Mustn't exceed 32 (FIFO buffer length)

\return 0 - data sent successfully (acknowledge received if enabled);
        1 - no reply from receiver (delivery probably failed).

\warning Potential hang if acknowledge is used. It happens when RFM73 in
"power down" mode.*/
uint8_t rfm73_send_packet(uint8_t type, uint8_t* pbuf, uint8_t len) {
	uint8_t fifo_sta, result = 0;
	uint8_t stat;
	
	//switch to tx mode
	rfm73_tx_mode();
	_delay_us(200);
	// read register FIFO_STATUS's value
	fifo_sta=_rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_FIFO_STATUS);
	//if not full, send data (write buff)
	if((fifo_sta&FS_TX_FULL_bm)==0) {
	  	RED_LED_SET;
		// Writes data to buffer
		if (type==RFM73_TX_WITH_ACK) {
			_rfm73_write_buf(RFM73_CMD_W_TX_PAYLOAD, pbuf, len);
			// wait for MAX_RT or TX_DS flags
			do {
				stat = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_STATUS);
			} while (!(stat & (ST_MAX_RT_bm | ST_TX_DS_bm)));
			// error "no reply"
			if (stat & ST_MAX_RT_bm) result = 1;
		}		
		else {
			_rfm73_write_buf(RFM73_CMD_W_TX_PAYLOAD_NOACK, pbuf, len);
		}		
		RED_LED_CLR;
	}
	
	return result;
}

/*! \brief This function returns some parameters used to observe quality of channel.

\param packet_lost   - number of packets lost from switching to current channel. The
                       counter is overflow protected to 15, and discontinues at max
					   until reset. The counter is reset by changing channel. For some
					   unknown reason this counter counts only to 1.
\param retrans_count - number of times last packet was retransmitted. The counter is
                       reset when transmission of a new packet starts.

\return 0.*/
uint8_t rfm73_observe(uint8_t* packet_lost, uint8_t* retrans_count) {
	uint8_t res = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_OBSERVE_TX);
	// return packet lost count
	*packet_lost = (res & 0xF0) >> 4;
	// return retransmit count
	*retrans_count = (res & 0x0F);
	return 0;
}

/*! \brief This function returns "carrier detect" status bit.

\return 1 if rf carrier is detected;
        0 if no rf carrier is detected.
		
\warning This bit is not reliable enough.*/
uint8_t rfm73_carrier_detect() {
	volatile uint8_t res = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_CD);
	return (res & 1);
}

/*! \brief This function returns current rf channel of the module.

\return 0-127 - channel number.*/
uint8_t rfm73_get_channel() {
	uint8_t res = _rfm73_read_reg(RFM73_CMD_R_REGISTER | RFM73_RADR_RF_CH);
	return (res & 0x7F);
}

/*! \brief This function scans air with auto-acknowledge message and returns
channel and datarate of the first answer.

\param ch - starting channel of the scan; in this variable channel number would be
            returned in case of answer;
\param dr - in this variable would be returned datarate;

\return 1 (and change #ch and #dr variables) if acknowledge received;
        0 if nothing received.*/
uint8_t rfm73_find_receiver(uint8_t* ch, uint8_t* dr) {
	uint8_t pl = 0xAA;
	uint8_t res;
	volatile uint8_t cur_ch = *ch;
	volatile uint8_t cur_dr = *dr;
	rfm73_set_autort(4000, 15);
	// scan every channel
	for (; cur_ch<0x80; cur_ch++) {
		rfm73_set_channel(cur_ch);
		// scan every data-rate
		for (cur_dr=0; cur_dr<3; cur_dr++) {
			rfm73_set_rf_params(RFM73_OUT_PWR_5DBM, RFM73_LNA_GAIN_HIGH, cur_dr);
			// clear RX_DR, TX_DS, MAX_RT
			uint8_t stat = _rfm73_read_reg(RFM73_CMD_R_REGISTER|RFM73_RADR_STATUS);
			_rfm73_write_reg(RFM73_CMD_W_REGISTER|RFM73_RADR_STATUS, stat);
			res = rfm73_send_packet(RFM73_TX_WITH_ACK, (uint8_t*)(&pl), 1);
			// if autoack received twice then break
			if (!res) {
				*ch = cur_ch;
				*dr = cur_dr;
				return 1;
			}
		}		
	}
	return 0;
}
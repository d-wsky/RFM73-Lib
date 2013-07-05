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

/*! \defgroup internalreg Internal registers and their structure

\brief Internal registers and their structure

These macros include all register addresses and their bit structure. Prefix
RFM73_RADR_ is used to determine register address followed by register name,
e.g. #RFM73_RADR_EN_RX_ADDR defines address of EN_RX_ADDR register (see
datasheet for more info about this register). Bitfields inside registers are
defined with register specific prefixes (in order to not mess them between
different registers) and _bf postfixes, e.g. #RS_LNA_HCURR_bf defines LNA_HCURR
bitfield in RF_SETUP register. Bit-masks are also defined and have _bm
postfixes.

\addtogroup internalreg
 @{ */

/*! \brief Address of configuration register */
#define RFM73_RADR_CONFIG       0x00
/*! \brief RX/TX control.
	- 1: PRX
	- 0: PTX*/
#define CF_PRIM_RX_bf           (0)
#define CF_PRIM_RX_bm           (0x01)
/*! \brief Power control.
	- 1: POWER UP
	- 0: POWER  DOWN*/
#define CF_PWR_UP_bf            (1)
#define CF_PWR_UP_bm            (0x02)
/*! \brief CRC encoding scheme.
	- 0: 1 byte 
	- 1: 2 bytes*/
#define CF_CRCO_bf              (2)
#define CF_CRCO_bm              (0x04)
/*! \brief Enable CRC. Forced high if one of the bits in the EN_AA is high. */
#define CF_EN_CRC_bf            (3)
#define CF_EN_CRC_bm            (0x08)
/*! \brief Mask interrupt caused by MAX_RT.
	- 1: Interrupt not reflected on the IRQ pin;
	- 0: Reflect MAX_RT as active low interrupt on the IRQ pin.*/
#define CF_MASK_MAX_RT_bf       (4)
#define CF_MASK_MAX_RT_bm       (0x10)
/*! \brief Mask interrupt caused by TX_DS.
	- 1: Interrupt not reflected on the IRQ pin;
	- 0: Reflect TX_DS as active low interrupt on the IRQ pin.*/
#define CF_MASK_TX_DS_bf        (5)
#define CF_MASK_TX_DS_bm        (0x20)
/*! \brief Mask interrupt caused by RX_DR.
	- 1: Interrupt not reflected on the IRQ pin;
	- 0: Reflect RX_DR as active low interrupt on the IRQ pin.*/
#define CF_MASK_RX_DR_bf        (6)
#define CF_MASK_RX_DR_bm        (0x40)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of enable "Auto Acknowledgment" function register */
#define RFM73_RADR_ENAA         0x01
/*! \brief Enable auto acknowledgement data pipe 0.*/
#define ENAA_P0_bf              (0)
#define ENAA_P0_bm              (0x01)
/*! \brief Enable auto acknowledgement data pipe 1.*/
#define ENAA_P1_bf              (1)
#define ENAA_P1_bm              (0x02)
/*! \brief Enable auto acknowledgement data pipe 2.*/
#define ENAA_P2_bf              (2)
#define ENAA_P2_bm              (0x04)
/*! \brief Enable auto acknowledgement data pipe 3.*/
#define ENAA_P3_bf              (3)
#define ENAA_P3_bm              (0x08)
/*! \brief Enable auto acknowledgement data pipe 4.*/
#define ENAA_P4_bf              (4)
#define ENAA_P4_bm              (0x10)
/*! \brief Enable auto acknowledgement data pipe 5.*/
#define ENAA_P5_bf              (5)
#define ENAA_P5_bm              (0x20)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of enabled RX addresses register */
#define RFM73_RADR_EN_RX_ADDR   0x02
/* Enabled RX Addresses */
/*! \brief Enable data pipe 0.*/
#define ERX_P0_bf               (0)
#define ERX_P0_bm               (0x01)
/*! \brief Enable data pipe 0.*/
#define ERX_P1_bf               (1)
#define ERX_P1_bm               (0x02)
/*! \brief Enable data pipe 2.*/
#define ERX_P2_bf               (2)
#define ERX_P2_bm               (0x04)
/*! \brief Enable data pipe 3.*/
#define ERX_P3_bf               (3)
#define ERX_P3_bm               (0x08)
/*! \brief Enable data pipe 4.*/
#define ERX_P4_bf               (4)
#define ERX_P4_bm               (0x10)
/*! \brief Enable data pipe 5.*/
#define ERX_P5_bf               (5)
#define ERX_P5_bm               (0x20)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of setup of address widths (common for all data pipes)
register */
#define RFM73_RADR_SETUP_AW     0x03
/* Setup of Address Widths (common for all data pipes) */
/*! \brief RX/TX Address field width. LSB bytes are used if address width is 
below 5 bytes:
	- '00': Illegal
	- '01': 3 bytes
	- '10': 4 bytes
	- '11': 5 bytes (default) */
#define SA_AW_bf                (0)
#define SA_AW_bm                (0x03)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of setup of automatic retransmission register */
#define RFM73_RADR_SETUP_RETR   0x04
/* Setup of Automatic Retransmission */
/*! \brief Auto Retransmission Count.

	- "0000": re-Transmit disabled;
	- "0001": up to 1 Re-Transmission on fail of AA;
	- ...
	- "1111": up to 15 Re-Transmission on fail of AA.*/
#define SR_ARC_bf               (0)
#define SR_ARC_bm               (0x0F)
/*! \brief Auto Retransmission Delay.

<ul>
<li>"0000": wait 250 us;
<li>"0001": wait 500 us;
<li>"0010": wait 750 us;
<li>...
<li>"1111": wait 4000 us.
</ul>
	
Delay defined from end of transmission to start of next transmission.*/
#define SR_ARD_bf               (4)
#define SR_ARD_bm               (0xF0)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of RF channel register */
#define RFM73_RADR_RF_CH        0x05
/* RF Channel */
/*! \brief Sets the frequency channel (0b000010 default).*/
#define RF_CH_bf                (0)
#define RF_CH_bm                (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of RF setup register */
#define RFM73_RADR_RF_SETUP     0x06
/*  RF Setup Register */
/*! \brief Setup LNA gain
	- 0: Low gain(20dB down);
	- 1: High gain.*/
#define RS_LNA_HCURR_bf         (0)
#define RS_LNA_HCURR_bm         (0x01)
/*! \brief Set RF output power in TX mode.
	- "00": -10 dBm;
	- "01": -5 dBm;
    - "10": 0 dBm;
	- "11": 5 dBm. */
#define RS_RF_PWR_bf            (1)
#define RS_RF_PWR_bm            (0x06)
/*! \brief Set Air Data Rate. Encoding (RF_DR_HIGH, RF_DR_LOW):
	- "00": 1Mbps;
	- "01": 250Kbps;
	- "10": 2Mbps (default);
	- "11": 2Mbps.*/
#define RS_RF_DR_HIGH_bf        (3)
#define RS_RF_DR_HIGH_bm        (0x08)
/*! \brief Force PLL lock signal. Only used in test (default is 0).*/
#define RS_PLL_LOCK_bf          (4)
#define RS_PLL_LOCK_bm          (0x10)
/*! \brief Set Air Data Rate. See RF_DR_HIGH for encoding */
#define RS_RF_DR_LOW_bf         (5)
#define RS_RF_DR_LOW_bm         (0x20)
#define RS_RF_DR_bm             (0x28)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of status register (In parallel to the SPI command word
applied on the MOSI pin, the STATUS register is shifted serially out 
on the MISO pin) */
#define RFM73_RADR_STATUS       0x07
/*! \brief TX FIFO full flag. 
	- 1: TX FIFO full;
	- 0: Available locations in TX FIFO.*/
#define ST_TX_FULL_bf           (0)
#define ST_TX_FULL_bm           (0x01)
/*! \brief Data pipe number for the payload available for reading from
RX_FIFO:
	- 000-101: Data Pipe Number;
	- 110: Not used;
	- 111: RX FIFO Empty.*/
#define ST_RX_P_NO_bf           (1)
#define ST_RX_P_NO_bm           (0x0E)
/*! \brief  Maximum number of TX retransmits interrupt. Write 1 to clear bit.
If MAX_RT is asserted it must be cleared to enable further communication.*/
#define ST_MAX_RT_bf            (4)
#define ST_MAX_RT_bm            (0x10)
/*! \brief   Data Sent TX FIFO interrupt. Asserted when packet transmitted on
TX. If AUTO_ACK is activated, this bit is set high only when ACK is received.
Write 1 to clear bit.*/
#define ST_TX_DS_bf             (5)
#define ST_TX_DS_bm             (0x20)
/*! \brief   Data Ready RX FIFO interrupt. Asserted when new data arrives RX
FIFO. Write 1 to clear bit.*/
#define ST_RX_DR_bf             (6)
#define ST_RX_DR_bm             (0x40)
 /*! \brief   Register bank selection states. Switch register bank is done by
SPI command "ACTIVATE"  followed by 0x53
	- 0: Register bank 0;
	- 1: Register bank 1.*/
#define ST_RBANK_bf             (7)
#define ST_RBANK_bm             (0x80)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of transmit observe register */
#define RFM73_RADR_OBSERVE_TX   0x08
/* Transmit observe register */
/*! \brief Count retransmitted packets. The counter is reset when
transmission of a new packet starts. */
#define OT_ARC_CNT_bf           (0)
#define OT_ARC_CNT_bm           (0x0F)
/*! \brief Count lost packets. The counter is overflow protected to 15, and
discontinues at max until reset. The counter is reset by writing to RF_CH.*/
#define OT_PLOS_CNT_bf          (4)
#define OT_PLOS_CNT_bm          (0xF0)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of carrier detect register */
#define RFM73_RADR_CD           0x09
/*! \brief Carrier detect */
#define CD_bf                   (0)
#define CD_bm                   (0x01)
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
#define RFM73_RADR_RX_PW_P0     0x11
#define RX_PW_P0_bf             (0)
#define RX_PW_P0_bm             (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 1 (1 to 32 bytes), 0 is default. */
#define RFM73_RADR_RX_PW_P1     0x12
#define RX_PW_P1_bf             (0)
#define RX_PW_P1_bm             (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 2 (1 to 32 bytes), 0 is default. */
#define RFM73_RADR_RX_PW_P2     0x13
#define RX_PW_P2_bf             (0)
#define RX_PW_P2_bm             (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 3 (1 to 32 bytes), 0 is default. */
#define RFM73_RADR_RX_PW_P3     0x14
#define RX_PW_P3_bf             (0)
#define RX_PW_P3_bm             (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 0 (1 to 32 bytes), 0 is default. */
#define RFM73_RADR_RX_PW_P4     0x15
#define RX_PW_P4_bf             (0)
#define RX_PW_P4_bm             (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of rx payload width register: number of bytes in RX payload
in data pipe 5 (1 to 32 bytes), 0 is default. */
#define RFM73_RADR_RX_PW_P5     0x16
#define RX_PW_P5_bf             (0)
#define RX_PW_P5_bm             (0x3F)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of FIFO status register */
#define RFM73_RADR_FIFO_STATUS  0x17

/*! \brief RX FIFO empty flag.
	- 1: RX FIFO empty;
	- 0: Data in RX FIFO.*/
#define FS_RX_EMPTY_bf          (0)
#define FS_RX_EMPTY_bm          (0x01)
/*!< \brief RX FIFO full flag.
	- 1: RX FIFO full;
	- 0: Available locations in RX FIFO. */
#define FS_RX_FULL_bf           (1)
#define FS_RX_FULL_bm           (0x02)
/*!< \brief TX FIFO empty flag.
	- 1: TX FIFO empty;
    - 0: Data in TX FIFO.*/
#define FS_TX_EMPTY_bf          (4)
#define FS_TX_EMPTY_bm          (0x10)
/*!< \brief TX FIFO full flag.
	- 1: TX FIFO full;
	- 0: Available locations in TX FIFO.*/
#define FS_TX_FULL_bf           (5)
#define FS_TX_FULL_bm           (0x20)
/*!< \brief Reuse last transmitted data packet if set high.
The packet is repeatedly retransmitted  as long as CE is high.
TX_REUSE is set by the SPI command REUSE_TX_PL, and is reset
by the SPI command W_TX_PAYLOAD or FLUSH TX */
#define FS_TX_REUSE_bf          (6)
#define FS_TX_REUSE_bm          (0x40)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of enable dynamic payload length register.*/
#define RFM73_RADR_DYNPD        0x1C
/*! \brief Enable dynamic payload length data pipe 0. (Requires EN_DPL and
ENAA_P0).*/
#define DPL_P0_bf               (0)
#define DPL_P0_bm               (0x01)
/*! \brief Enable dynamic payload length data pipe 1. (Requires EN_DPL and
ENAA_P1).*/
#define DPL_P1_bf               (1)
#define DPL_P1_bm               (0x02)
/*! \brief Enable dynamic payload length data pipe 2. (Requires EN_DPL and
ENAA_P2).*/
#define DPL_P2_bf               (2)
#define DPL_P2_bm               (0x04)
/*! \brief Enable dynamic payload length data pipe 3. (Requires EN_DPL and
ENAA_P3).*/
#define DPL_P3_bf               (3)
#define DPL_P3_bm               (0x08)
/*! \brief Enable dynamic payload length data pipe 4. (Requires EN_DPL and
ENAA_P4).*/
#define DPL_P4_bf               (4)
#define DPL_P4_bm               (0x10)
/*! \brief Enable dynamic payload length data pipe 5. (Requires EN_DPL and
ENAA_P5).*/
#define DPL_P5_bf               (5)
#define DPL_P5_bm               (0x20)
/*****************************************************************************/

/*****************************************************************************/
/*! \brief Address of feature register */
#define RFM73_RADR_FEATURE      0x1D
/* Feature Register */
/*!< \brief Enables the W_TX_PAYLOAD_NOACK command.*/
#define FE_EN_DYN_ACK_bf        0
#define FE_EN_DYN_ACK_bm        0x01
/*!< \brief Enables Payload with ACK */
#define FE_EN_ACK_PAY_bf        1
#define FE_EN_ACK_PAY_bm        0x02
/*!< \brief Enables Dynamic Payload Length */
#define FE_EN_DPL_bf            2
#define FE_EN_DPL_bm            0x04
/*****************************************************************************/

/*! @} */

/******************************************************************************
**  COMMANDS                                                                 **
******************************************************************************/

/*! \defgroup internalcmd Internal commands macros

\brief This macros define commands used to configure and communicate with RFM73
module. This macros are used as first argument in functions _rfm73_write_cmd,
_rfm73_read_cmd and some more.

\addtogroup internalcmd
 @{ */
	 
/*! \brief Read command and status registers in format 000AAAAA. AAAAA = 
5 bit Register Map Address */
#define RFM73_CMD_R_REGISTER          0b00000000
/*! \brief Write command and status registers. AAAAA = 5 bit Register Map
Address. Executable in power down or standby modes only.*/
#define RFM73_CMD_W_REGISTER          0b00100000
/*! \brief Activate some functions.

This write command  followed by data 0x73 activates the following
features: 
    
<ul>
<li>R_RX_PL_WID
<li>W_ACK_PAYLOAD
<li>W_TX_PAYLOAD_NOACK
</ul>

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
/*! \brief Read RX-payload  width for the top R_RX_PAYLOAD in the RX FIFO. */
#define RFM73_CMD_R_RX_PL_WID         0b01100000  
/*! \brief Write TX-payload: 1 – 32 bytes. A write operation always starts
at byte 0 used in TX payload.*/
#define RFM73_CMD_W_TX_PAYLOAD        0b10100000
/*! \brief Read RX-payload:  1 – 32 bytes. A read operation always starts
at byte 0. Payload is deleted from FIFO after it is read. Used in
RX mode. 1 to 32 byte, LSB first */
#define RFM73_CMD_R_RX_PAYLOAD        0b01100001  
/*! \brief Used in TX mode. Transmits packet with disabled AUTOACK. */
#define RFM73_CMD_W_TX_PAYLOAD_NOACK  0b10110000  
/*! \brief Flush TX FIFO, used in TX mode */
#define RFM73_CMD_FLUSH_TX            0b11100001
/*! \brief Flush RX FIFO, used in RX mode. Should not be executed during
transmission of acknowledge because acknowledge package will not 
be completed. */
#define RFM73_CMD_FLUSH_RX            0b11100010
/*! \brief  Used for a PTX device. Reuse last transmitted payload. Packets
are repeatedly retransmitted as long as CE is high. TX payload reuse is
active until W_TX_PAYLOAD or FLUSH TX is executed. TX payload reuse must
not be activated or deactivated during package transmission  */
#define RFM73_CMD_REUSE_TX_PL         0b11100011  
/*! \brief No Operation. Might be used to read the STATUS 
register. */
#define RFM73_CMD_NOP                 0b11111111

/*! @} */


/*! \brief Bank1 register initialization value. Some magic numbers here
duplicates data from datasheet, some of the byte reversed. DO NOT edit
this array. */
const unsigned long Bank1_Reg0_13[]={ 0xE2014B40, 0x00004BC0, 0x028CFCD0,
                                      0x41390099, 0x1B8296d9, 0xA67F0224,
                                      0x00000000, 0x00000000, 0x00000000,
                                      0x00000000, 0x00000000, 0x00000000,
                                      0x00127300, 0x46B48000, };
/*! \brief Bank1 register 14 initialization value. More magic numbers,
once again: DO NOT EDIT this array.*/
uint8_t Bank1_Reg14[]= { 0x41,0x20,0x08,0x04,0x81,0x20,0xCF,0xF7,
                         0xFE,0xFF,0xFF };


//Receive address data pipe 0
const uint8_t RX0_Address[]={0x34,0x43,0x10,0x10,0x01};
//Receive address data pipe 1
const uint8_t RX1_Address[]={0x39,0x38,0x37,0x36,0xc2};

/*! \defgroup lowlevelfunc Low level functions

\brief Low level functions that work directly with RFM73 using SPI
communication.

This functions are designed to work directly with RFM73 internal registers.
It's supposed that end user doesn't use them in his code, so they have an
underslash prefix and don't included in header file.

Low level functions include such actions as:

<ul>
<li>read and write one or multiply bytes from/to module: _rfm73_read_cmd,
    _rfm73_write_cmd for single byte, _rfm73_read_buf, _rfm73_write_buf for
	more than 1 byte. As it seen from function names these may be used to send
	commands to module. Trivial API example:
	\code
	    foo = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG)
	    _rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_EN_AA, bar);
	    // Flush RX buffer:
		_rfm73_write_cmd(RFM73_CMD_FLUSH_RX, 0);
	\endcode
<li>toggle internal register bank of RFM73 module: _rfm73_toggle_reg_bank.
	There are 2 register banks in RFM73 module and there is no necessity in
	switching to bank 2 in any time except initialization.
<li>send activate command to enable some features of the RFM73 module:
	_rfm73_activate. There is no deactivate function (but the module itself
	support this feature with the same code sequence). This function checks
	whether the module in active state and do not send anything in this case.
</ul>	  
	  
\addtogroup lowlevelfunc
 @{ */

/*! \brief Writes some value to some internal register or send specific command
to module.

\param reg - bitwise OR between register address and command, e.g.
             #RFM73_CMD_W_REGISTER | #RFM73_RADR_CONFIG, #RFM73_CMD_FLUSH_TX,
			 etc.
\param value - some value, that would be written if this function is used to
               write data to register.*/
void _rfm73_write_cmd(uint8_t reg, uint8_t value) {
	// CSN low, init SPI transaction
	RFM73_CSN_LOW;
	// select register
	spi_read(reg);
	// ..and write value to it..
	spi_read(value);
	// CSN high again
	RFM73_CSN_HIGH;
}

/*! \brief Read data from internal register of the RFM73 module.

\param reg - bitwise OR between command and a register address from which this
             operation would be perfomed, e.g.
			 \code
			     uint8_t foo = _rfm73_read_cmd(RFM73_CMD_R_REGISTER |
				                               RFM73_RADR_FIFO_STATUS);
			 \endcode
\return Value that was stored in this register.*/
uint8_t _rfm73_read_cmd(uint8_t reg) {        
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
                                                 
/*! \brief Read multi-byte data from some register, e.g.: read RX payload
buffer, get RX address of pipeline 0 or 1, etc.

\param reg - bitwise OR between read command and address of a register for
             which this command is being applied.
\param pBuf - RAM-buffer, where data would be stored.
\param length - number of bytes to be read.*/
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
                                             
/*! \brief Write multi-byte data to the RFM73 module.

\param reg - bitwise OR between read command and address of a register for
             which this command is being applied.
\param pBuf - RAM-buffer, where data is stored.
\param length - number of bytes to write.*/
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

uint8_t rfm73_get_power_state();
void rfm73_power_down();
void rfm73_power_up();

/*! \brief Send a command sequence that activate module commands R_RX_PL_WID,
W_ACK_PAYLOAD, W_TX_PAYLOAD_NOACK. If this commands are already activated, does
nothing.*/
void _rfm73_activate() {
	uint8_t is_powered = rfm73_get_power_state();
	if (is_powered) rfm73_power_down();
	uint8_t r = _rfm73_read_cmd(RFM73_CMD_R_REGISTER|RFM73_RADR_FEATURE);
	// if there are ones in answer then module is activated
	if (r==0) {
		// check activation by writing 1 to LSB
		_rfm73_write_cmd(RFM73_CMD_W_REGISTER|RFM73_RADR_FEATURE, 0x01);
		r = _rfm73_read_cmd(RFM73_CMD_R_REGISTER|RFM73_RADR_FEATURE);
		if (r!=1) {
			// module is not activated. activation sequence
			_rfm73_write_cmd(RFM73_CMD_ACTIVATE, 0x73);
		}
	}
	// return power up state
	if (is_powered) rfm73_power_up();
}

/*! \brief Toggles between 0 and 1 internal register bank of the module.

\param rbank - what rbank should be select.*/
void _rfm73_toggle_reg_bank(uint8_t rbank) {
	uint8_t temp;

	temp=_rfm73_read_cmd(7);
	temp=temp&0x80;

	if( ( (temp)&&(rbank==0) )
	||( ((temp)==0)&&(rbank) ) ) {
		_rfm73_write_cmd(RFM73_CMD_ACTIVATE,0x53);
	}
}

/*! @}*/

/*! \defgroup highlevelfunc High-level functions

\brief Functions that are used by end-user.

These functions could be roughly divided in following groups:

<ul>
<li>initialization function: rfm73_init;
<li>various configuration functions, all having rfm73_set_ prefix:
rfm73_set_en_pipelines, rfm73_set_crc_len, etc.
<li>various read functions, all having rfm73_get_ prefix: rfm73_get_channel,
rfm73_get_power_state, etc.
<li>various specific functions: rfm73_power_up, rfm73_power_down,
rfm73_rx_mode, etc.
<li>communication functions: rfm73_send_packet, rfm73_receive_packet.
<li>high-level function rfm73_find_receiver, which performs scan of air using
auto-ack packet and returns channel and datarate at which response had been
received.
</ul>

\addtogroup highlevelfunc
 @{ */

/*! \brief This function sets RFM73 module in RX mode (this mode is
characterized with high energy drain).*/
void rfm73_rx_mode()
{
	uint8_t value;
	//flush Rx
	_rfm73_write_cmd(RFM73_CMD_FLUSH_RX,0);
	// read register STATUS's value
	value=_rfm73_read_cmd(RFM73_RADR_STATUS);
	// clear RX_DR or TX_DS or MAX_RT interrupt flag
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER|RFM73_RADR_STATUS,value);

	RFM73_CE_LOW;
	// read register CONFIG's value
	value=_rfm73_read_cmd(RFM73_RADR_CONFIG);
	//PRX set bit 1
	value=value|0x01;
	// Set PWR_UP bit, enable CRC(2 length) & Prim:RX. RX_DR enabled..
  	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, value); 

	RFM73_CE_HIGH;
}

/*! \brief Set RFM73 module to transmit-state. In this state module will
remain until emptying the transmit buffer following by standby-2 state.

\warning As it sad in datasheet it is important to never leave the module in TX
state for more than 4 ms.*/
void rfm73_tx_mode()
{
	uint8_t value;
	//flush Tx
	_rfm73_write_cmd(RFM73_CMD_FLUSH_TX,0);

	RFM73_CE_LOW;
	// read register CONFIG's value
	value=_rfm73_read_cmd(RFM73_RADR_CONFIG);	
    //PTX set bit 0
	value=value&0xfe;
	// Set PWR_UP bit, enable CRC(2 length) & Prim:RX. RX_DR enabled.
  	_rfm73_write_cmd(RFM73_CMD_W_REGISTER|RFM73_RADR_CONFIG, value); 
	
	RFM73_CE_HIGH;
}

/*! \brief This function setup length of CRC field that is added by module to
sending data.

\param crc_len 
             - 0 - no CRC protection of data;
             - 1 - protect with CRC-8 code (the polynomial X^8 + X^2 + X + 1,
			       initial value is 0xFF);
			 - 2 - protect with CRC-16 code (the polynomia is
			       X^16 + X^12 + X^5 + 1, initial value is 0xFFFF).*/
void rfm73_set_crc_len(uint8_t crc_len) {
	uint8_t conf = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	switch (crc_len) {
		case 0:
			// no crc
			conf &=~CF_EN_CRC_bm;
			break;
		case 1:
			// crc-8
			conf |= CF_EN_CRC_bm;
			conf &=~CF_CRCO_bm;
			break;
		default:
			conf |= (CF_EN_CRC_bm | CF_CRCO_bm);
	}	
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, conf);
}

/*! \brief This function setup current RF channel within 2.4 GHz frequency
window.

\param ch - channel number between 0-127.*/
void rfm73_set_channel(uint8_t ch)
{
	_rfm73_write_cmd((uint8_t)(RFM73_CMD_W_REGISTER|RFM73_RADR_RF_CH),
	                (uint8_t)(ch));
}

/*! \brief This function sets main RF params of the module.

\param out_pwr - defines output power of the module. It tooks one of these
                 values: #RFM73_OUT_PWR_MINUS10DBM, #RFM73_OUT_PWR_MINUS5DBM,
				 #RFM73_OUT_PWR_0DBM, #RFM73_OUT_PWR_PLUS5DBM;
\param lna_gain - this parameter determines low noise amplifier gain and can
                  take one of two values: #RFM73_LNA_GAIN_HIGH,
				  #RFM73_LNA_GAIN_LOW;
\param data_rate - air data rate is set within these values:
                   #RFM73_DATA_RATE_1MBPS, #RFM73_DATA_RATE_2MBPS,
				   #RFM73_DATA_RATE_250KBPS.*/
void rfm73_set_rf_params(uint8_t out_pwr, uint8_t lna_gain,
                         uint8_t data_rate) {
	volatile uint8_t c;
	// read config
	c = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_RF_SETUP);
	// clear all used bits
	c &=~(RS_RF_DR_bm | RS_LNA_HCURR_bm | RS_RF_PWR_bm);
	// set appropriate bits
	c |= (out_pwr & 3) << RS_RF_PWR_bf;
	c |= (lna_gain & 1) << RS_LNA_HCURR_bf;
    c |= (((data_rate & 2) >> 1) << RS_RF_DR_HIGH_bf) |
	     ((data_rate & 1) << RS_RF_DR_LOW_bf);
	// write config
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_RF_SETUP, c);	
}

/*! \brief This function enables auto-acknowledge feature of specified receive
pipelines.

\param pipeline_mask - a positional value between 0x00 and 0x3F where certain
                       bit defines whether this feature is enabled for
					   respective pipeline. E.g. sending to this function a
					   value of 0x13 would lead to enabling auto-acknowledge to
					   pipelines 0, 1, and 4.*/
void rfm73_set_autoack(uint8_t pipeline_mask) {
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_ENAA,
	                 pipeline_mask & 0x3F);
}

/*! \brief This function enables specified receive pipelines therefore
enabling communication with addresses connected to these pipelines.

\param pipeline_mask - a positional value between 0x00 and 0x3F where certain
                       bit defines whether respective pipeline is enabled. E.g.
					   sending to this function a value of 0x13 would lead to
					   enabling pipelines 0, 1, and 4.*/
void rfm73_set_en_pipelines(uint8_t pipeline_mask) {
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_EN_RX_ADDR,
	                 pipeline_mask & 0x3F);
}

/*! \brief This function sets address width of all data pipelines.

\param aw - address width, one of these macros: #RFM73_ADR_WID_3BYTES,
            #RFM73_ADR_WID_4BYTES, #RFM73_ADR_WID_5BYTES, setting up 3, 4 and 5
			bytes address width respectively.*/
void rfm73_set_address_width(uint8_t aw) {
	uint8_t c = aw & 3;
	// '00' is illegal
	if (c==0) c = 1;
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER|RFM73_RADR_SETUP_AW, c);
}

/*! \brief This function sets auto re-trnasmition parameters.

\param rt_time - time in milliseconds (250-4000) between consequent
                 retransmitions. This value is set up with 250 ms steps (that
				 is e.g. 255 and 350 input values will result in the same
				 delay).
\param rt_count - number of re-transmit tries. When this value is reached
                  transmition would be broken (MAX_RT flag would be set).*/
void rfm73_set_autort(uint16_t rt_time, uint8_t rt_count) {
	uint8_t time = rt_time/250-1;
	uint8_t count = rt_count;
	// set up constrains
	if (rt_time<250) time = 0;
	if (rt_time>4000) time = 15;
	if (rt_count>15) count = 15;
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_SETUP_RETR,
	                 (time << 4) | count);
}

/*! \brief This function sets all bytes of the TX address. Number of bytes to
set is determined by reading SETUP_AW register.

\param addr - address value array with sufficient length.*/
void rfm73_set_tx_addr(uint8_t* addr) {
	// get address lenght
	uint8_t addr_len = 
	    _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_SETUP_AW);
	// set new addr
	_rfm73_write_buf((RFM73_CMD_W_REGISTER | RFM73_RADR_TX_ADDR),
	                 addr, addr_len+2);
}

/*! \brief This function sets all bytes of the RX pipeline 0 address. Number of
bytes to set is determined by reading SETUP_AW register.

\param addr - address value array with sufficient length.*/
void rfm73_set_rx_addr_p0(uint8_t* addr) {
	// get address lenght
	uint8_t addr_len = 
	    _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_SETUP_AW);
	// set new addr
	_rfm73_write_buf((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P0),
	                 addr, addr_len+2);
}

/*! \brief This function sets all bytes of the RX pipeline 1 address. Number of
bytes to set is determined by reading SETUP_AW register.

\param addr - address value array with sufficient length.*/
void rfm73_set_rx_addr_p1(uint8_t* addr) {
	// get address lenght
	uint8_t addr_len = 
	    _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_SETUP_AW);
	// set new addr
	_rfm73_write_buf((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P1),
	                 addr, addr_len+2);
}

/*! \brief This function sets LSB byte of the RX pipeline2 address (other
bytes are equal to MSB bytes of RX pipeline1.

\param addr - address value (any uint8_t).*/
void rfm73_set_rx_addr_p2(uint8_t addr) {
	_rfm73_write_cmd((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P2), addr);
}

/*! \brief This function sets LSB byte of the RX pipeline3 address (other
bytes are equal to MSB bytes of RX pipeline1.

\param addr - address value (any uint8_t).*/
void rfm73_set_rx_addr_p3(uint8_t addr) {
	_rfm73_write_cmd((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P3), addr);
}

/*! \brief This function sets LSB byte of the RX pipeline4 address (other
bytes are equal to MSB bytes of RX pipeline1.

\param addr - address value (any uint8_t).*/
void rfm73_set_rx_addr_p4(uint8_t addr) {
	_rfm73_write_cmd((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P4), addr);
}

/*! \brief This function sets LSB byte of the RX pipeline5 address (other
bytes are equal to MSB bytes of RX pipeline1.

\param addr - address value (any uint8_t).*/
void rfm73_set_rx_addr_p5(uint8_t addr) {
	_rfm73_write_cmd((RFM73_CMD_W_REGISTER | RFM73_RADR_RX_ADDR_P5), addr);
}

/*! \brief This function sets payload width of the specified receiving
pipeline.

\param pipeline - number of the pipeline (0-5);
\param wid      - width of the payload field in transmittion packet (0-32).*/
void rfm73_set_rx_payload_width(uint8_t pipeline, uint8_t wid) {
	if (pipeline>5) pipeline = 5;
	if (wid>32) wid = 32;
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | (RFM73_RADR_RX_PW_P0 + pipeline),
	                 wid);
}

/*! \brief This function enables dynamic payload feature for specific
pipelines. Pipeline numbers are defined in bit-mask manner and LSB respect to
pipeline0.

\param pipeline_mask - number from 0 to 0x3F specifing what pipelines would get
                       dynamic payload feature. E.g.: setting pipeline_mask to
					   0x13 would enable this feature to pipelines 0, 1 and 4.
*/
void rfm73_set_dyn_payload(uint8_t pipeline_mask) {
	pipeline_mask &= 0x3f;
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_DYNPD, pipeline_mask);
}

/*! \brief This function enables and disables features that described in
FEATURE register of the RFM73 module.

\param en_dyn_payload_len - enables dynamic payload length.
\param en_payload_ack - enables payload with acknowledge.
\param en_tx_with_noack - enables the W_TX_PAYLOAD_NOACK command.*/
void rfm73_set_features(uint8_t en_dyn_payload_len, uint8_t en_payload_ack,
                        uint8_t en_tx_with_noack) {
	uint8_t c = 0;
	// setting up bits
	c |= ((en_tx_with_noack & 1) << FE_EN_DYN_ACK_bf) |
		 ((en_payload_ack & 1) << FE_EN_ACK_PAY_bf) |
		 ((en_dyn_payload_len& 1) << FE_EN_DPL_bf);
	// write them to register
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_FEATURE, c);
}

/*! \brief This function returns whether dynamic payload feature is enabled.

\return Value (0-0x3F) - a positional code which shows whether dynamic payload
feature is enabled for respective pipeline. E.g: result = 0x13 means that this
feature is enabled for pipelines 0, 1 and 4.*/
uint8_t rfm73_get_dyn_payload() {
	return _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_DYNPD);
}

/*! \brief This function returns length of specified pipeline in bytes.

\param pipeline - number (0-5) of pipeline.

\return Length (0-32) of specified pipeline.*/
uint8_t rfm73_get_rx_payload_width(uint8_t pipeline) {
	if (pipeline>5) pipeline = 5;
	return _rfm73_read_cmd(RFM73_CMD_R_REGISTER | 
	                       (RFM73_RADR_RX_PW_P0 + pipeline));
}

/*! \brief This function returns current power up state of the module.

\return 
      - 0 if module is powered down;
      - 1 if module is powered up.*/
uint8_t rfm73_get_power_state() {
	// return value of PWR_UP bit in CONFIG register
	return ((_rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG)
	       & CF_PWR_UP_bm) >> CF_PWR_UP_bf);
}
/*! \brief This function returns current rf channel of the module.

\return 0-127 - channel number.*/
uint8_t rfm73_get_channel() {
	uint8_t res = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_RF_CH);
	return (res & 0x7F);
}

/*! \brief Set the RFM73 module to power up state. Module will go to standby-1
mode and after that to TX, RX or standby-2 mode depending on current
configuration.*/
void rfm73_power_up() {
	uint8_t conf = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	// set CF_PWR_UP bit high
	conf |= CF_PWR_UP_bm;
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, conf);
	// power up delay
	_delay_ms(3);
}

/*! \brief Set the RFM73 module to power down state, minimizing it power
consumption. Receiver and transmitter are disabled.*/
void rfm73_power_down() {
	uint8_t conf = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	// set CF_PWR_UP bit low
	conf &=~CF_PWR_UP_bm;
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, conf);
}

/*! \brief Masking interrupts, preventing events from affecting IRQ pin of the
RFM73 module.

\param mask_rx_dr if 0 - after receiving new packet RFM73 module will set low
                  level on IRQ pin;
\param mask_tx_ds if 0 - after successful transmittion of packet, RFM73 module
                  will set low level on IRQ pin;
\param mask_max_rt if 0 - after getting MAX_RT unsuccessful retransmittions
                   of current packet RFM73 module will set low level on IRQ
				   pin.*/
void rfm73_mask_int(uint8_t mask_rx_dr, uint8_t mask_tx_ds, 
                    uint8_t mask_max_rt) {
	// read current state
	uint8_t c = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_CONFIG);
	// clear mask bits
	c &=~(CF_MASK_MAX_RT_bm | CF_MASK_RX_DR_bm | CF_MASK_TX_DS_bm);
	// set appropriate mask bits
	c |= (((mask_rx_dr & 1) << CF_MASK_RX_DR_bf) |
	      ((mask_tx_ds & 1) << CF_MASK_TX_DS_bf) |
		  ((mask_max_rt& 1) << CF_MASK_MAX_RT_bf));
	// write new config
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_CONFIG, c);
}

/*! \brief This function is used to get new packet from FIFO buffer.

\param type - #RFM73_RX_WITH_ACK if explicit acknowledge of the packet is
              needed;
			  #RFM73_RX_WITH_NOACK to only get data from FIFO buffer.
\param data_buf - pointer to start of the input buffer;
\param len  - length of received packet (this data is read from RFM73,
              and does not exceed 32).
			  
\return 
        - 0 - if received data is correct;
        - 1 - if #RFM73_MAX_PACKET_LEN is exceeded, input FIFO buffer is 
		      flushed, data_buf unchanged;
	    - 2 - if flag RX_DR (receive data ready) doesn't raised.
		
\warning Possible hangouts in certain conditions.*/
uint8_t rfm73_receive_packet(uint8_t type, uint8_t* data_buf, uint8_t* len) {
	uint8_t sta,fifo_sta;
	uint8_t result = 0;
	
	// read register STATUS's value
	sta=_rfm73_read_cmd(RFM73_CMD_R_REGISTER|RFM73_RADR_STATUS);
	
	// if receive data ready (RX_DR) interrupt
	if(sta & ST_RX_DR_bm) {
		do {
			// read len
			*len=_rfm73_read_cmd(RFM73_CMD_R_RX_PL_WID);	

			if(*len<=RFM73_MAX_PACKET_LEN) {
				// read receive payload from RX_FIFO buffer
				_rfm73_read_buf(RFM73_CMD_R_RX_PAYLOAD, data_buf, *len);
				// return "some data received"
				result = 0;
			}
			else {
				//flush Rx
				*len = 0;
				_rfm73_write_cmd(RFM73_CMD_FLUSH_RX,0);
				// return "data was flushed"
				result = 1;
			}
			// read register FIFO_STATUS's value
			fifo_sta=_rfm73_read_cmd(RFM73_CMD_R_REGISTER | 
			                         RFM73_RADR_FIFO_STATUS);			
		} while ((fifo_sta&FS_RX_EMPTY_bm)==0); //while not empty
		
		GREEN_LED_SET;
		if (type == RFM73_RX_WITH_ACK) {
			rfm73_send_packet(RFM73_CMD_W_TX_PAYLOAD_NOACK, data_buf, *len);
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
	_rfm73_write_cmd(RFM73_CMD_W_REGISTER|RFM73_RADR_STATUS,sta);
	
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

\return 
        - 0 - data sent successfully (acknowledge received if enabled);
        - 1 - no reply from receiver (delivery probably failed).

\warning Potential hang if acknowledge is used. It happens when RFM73 in
"power down" mode.*/
uint8_t rfm73_send_packet(uint8_t type, uint8_t* pbuf, uint8_t len) {
	uint8_t fifo_sta, result = 0;
	uint8_t stat;
	
	//switch to tx mode
	rfm73_tx_mode();
	_delay_us(200);
	// read register FIFO_STATUS's value
	fifo_sta=_rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_FIFO_STATUS);
	//if not full, send data (write buff)
	if((fifo_sta&FS_TX_FULL_bm)==0) {
	  	RED_LED_SET;
		// Writes data to buffer
		if (type==RFM73_TX_WITH_ACK) {
			_rfm73_write_buf(RFM73_CMD_W_TX_PAYLOAD, pbuf, len);
			// wait for MAX_RT or TX_DS flags
			do {
				stat = _rfm73_read_cmd(RFM73_CMD_R_REGISTER |
				                       RFM73_RADR_STATUS);
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

/*! \brief This function returns some parameters used to observe quality of 
channel.

\param packet_lost   - number of packets lost from switching to current 
                       channel. The counter is overflow protected to 15, and
					   discontinues at max until reset. The counter is reset by
					   changing channel. For some unknown reason this counter
					   counts only to 1.
\param retrans_count - number of times last packet was retransmitted. The
                       counter is reset when transmission of a new packet
					   starts.

\return 0.*/
uint8_t rfm73_observe(uint8_t* packet_lost, uint8_t* retrans_count) {
	uint8_t res = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_OBSERVE_TX);
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
	volatile uint8_t res = _rfm73_read_cmd(RFM73_CMD_R_REGISTER |
	                                       RFM73_RADR_CD);
	return (res & 1);
}

/*! \brief This function scans air with auto-acknowledge message and returns
channel and datarate of the first answer.

\param *ch - starting channel of the scan; in this variable channel number
             would be returned in case of answer;
\param *dr - in this variable would be returned datarate;

\return 1 (and change ch and dr params) if acknowledge received;
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
			rfm73_set_rf_params(RFM73_OUT_PWR_PLUS5DBM, RFM73_LNA_GAIN_HIGH,
			                    cur_dr);
			// clear RX_DR, TX_DS, MAX_RT
			uint8_t stat = 
			    _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_STATUS);
			_rfm73_write_cmd(RFM73_CMD_W_REGISTER | RFM73_RADR_STATUS, stat);
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

/*! \brief This function is used to init RFM73 module and to set all parameters
to some default values.

Default configuration sequence includes calling all rfm73_set_functions with
following params:

<ul>
<li>out_pwr, lna_gain, data_rate are sent to rfm73_set_rf_params;
<li>ch is sent to rfm73_set_channel;
<li>crc length is set to 2;
<li>all pipelines are enabled, got auto-ack, width of 32 and all dynamic
    payload features are enabled;
<li>auto-ack period and re-transmition count are set to maximum;
<li>masking all interrupts except MAX_RT;
<li>RX address of pipeline0 and TX address are set to RX0_Address array,
	RX address of pipeline1 is set to RX1_Address array;
<li>bank1 is being intialized by the values stored in Bank1_Reg0_13 and
	Bank1_Reg14 arrays;
<li>module set to power up state, RX mode.
</ul>*/
void rfm73_init(uint8_t out_pwr, uint8_t lna_gain, uint8_t data_rate,
                uint8_t ch) {
	uint8_t i,j;
 	uint8_t WriteArr[12];

	_delay_ms(200);
	
	_rfm73_toggle_reg_bank(0);

	// set all registers at once										 
	/*for(i=0;i<20;i++) {
		_rfm73_write_cmd((RFM73_CMD_W_REGISTER|Bank0_Reg[i][0]),
		                 Bank0_Reg[i][1]);
	}*/
	
	rfm73_set_rf_params(out_pwr, lna_gain, data_rate);
	rfm73_set_crc_len(2);
	rfm73_set_autoack(0x3F);
	rfm73_set_en_pipelines(0x3F);
	rfm73_set_address_width(RFM73_ADR_WID_5BYTES);
	rfm73_set_autort(4000, 15);
	rfm73_mask_int(0, 0, 1);
	rfm73_set_channel(ch);
	
	
	// Rx0 addr
	rfm73_set_rx_addr_p0((uint8_t*)(&RX0_Address));
	
	// Rx1 addr
	rfm73_set_rx_addr_p1((uint8_t*)(&RX1_Address));

	// TX addr
	rfm73_set_tx_addr((uint8_t*)(&RX0_Address));
	
	// set pipelines width
	for (i = 0; i<5; i++)
		rfm73_set_rx_payload_width(i, 32);

	_rfm73_activate();
	/*read Feature Register Payload With ACK ACTIVATE
	  Payload With ACK (REG28,REG29).
		i = _rfm73_read_cmd(RFM73_CMD_R_REGISTER | RFM73_RADR_FEATURE);
		if (i==0) 
			_rfm73_write_cmd(RFM73_CMD_ACTIVATE, 0x73); // Active
		// i!=0 showed that chip has been actived.so do not active again.*/
	rfm73_set_features(1, 1, 1);
	rfm73_set_dyn_payload(0x3F);
	/*for(i=22;i>=21;i--) {
		_rfm73_write_cmd((RFM73_CMD_W_REGISTER|Bank0_Reg[i][0]),
		                 Bank0_Reg[i][1]);
	}*/
	
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
	rfm73_power_up();
	rfm73_rx_mode();
}

/*! @}*/
#ifndef _RFM73_H_
#define _RFM73_H_

#include <avr/io.h>
#include <inttypes.h>

/*! \mainpage RFM73 C-library documentation

C interface library for the HopeRF RFM73 2.4 GHz transceiver module

\image html RFM73-D.jpg
\image rtf RFM73-D.jpg

\author 
    - d.wsky (d.wsky@yandex.ru)
	- some words and pictures were stolen from Wouter van Ooijen (wouter@voti.nl)

\version
    V0.5 (2013-07-05)

\par Introduction

The RFM73 module from HopeRF (www.hoperf.com) is a small and cheap transceiver
(transmitter + receiver) module for the license-free 2.4 MHz frequency band.
This module is intended for short range communication, for instance within a
room. Typical applications are wires mouses and keyboards, and garage door
openers.

In open air a distance of 50 .. 100 m is possible, but this is strictly
line-of-sight: even the leaves of a single tree can obstruct the signal.

The two main files in this library, rfm73.h and rfm73.c, are almost target
independent. However, rfm73.c included by a file that must be provided by the
user.  This file must provide macro's for initializing and accessing the I/O
pins that connect to the RFM73 module, and for delays of a specified number of
milliseconds and microseconds. A template for this file is provided.

\par Files
 - rfm73.h
 - rfm73.c
 - main.c (some rough avr example of using this module).

\par ToDo: bugs, notes, pitfalls, todo, known problems, etc

 - pictures for power;
 - further development.
*/

//***************************************************************************//
//
//! \page overview RFM73 overview
//!
//! \image html smd-pins.jpg
//! \image rtf smd-pins.jpg
//!
//! The RFM73 is a small module (PCB with some components, ~ 18 x 26 mm) 
//! that implements a transciever for the license-free 2.4 GHz band. 
//! The module contains a small antenna on the PCB, 
//! there is no provision for an external antenna.
//! Two versions of the module exist, one with 
//! pins and one without. The pins are on a 1.28 mm grid, which is twice the
//! density of a standard DIL package. The module requires 1.9 .. 3.6 Volt
//! for its power, 3.3 Volt seems to be the preferred operating voltage. 
//! It is NOT designed to run at 5.0 Volt. 
//! According to the datasheet that maximum 'typical' operating current 
//! is 23 mA.
//!
//! Note that 'licence free' means that certain well-described use of this 
//! frequency band is allowd without a license. 
//! The RFM73 is designed to be compatible with the requirements for such 
//! use, but it is the responsibility of anyone who sells a product that
//! to make sure that it complies with such rules.
//!
//! The main component of an RFM73 module is the RF70 chip (hidden beneath
//! the black blob). The manufacturer provides separate datasheets for both 
//! the RF70 chip and the RFM73 module. 
//! The two datasheets seem to be the same, except for the physical 
//! and pinout information which is of course different for the chip
//! and the module, so you can probably ignore the RF70 chip datasheet.
//! The RFM73 module datasheet can currently be found at 
//! http://www.hoperf.com/upload/rf/RFM73.pdf
//!
//! The RFM73 module is intended for short-range communication,
//! like wireless computer peripherals (mouse, keyboard, tablet, etc.)
//! key fobs (car opener, garage door opener, motorized fence opener - 
//! some cryptography will probably be required for such applications)
//! and toys. In a line of sight situation a maximum range of 50 .. 100 m
//! is possible. Indoors communication within a single room will generally
//! be OK (unless you have a very large room..) but passing even a single
//! wall can be a problem.
//!
//! An RFM73 module must be used with a micro controller that initializes
//! and operates the module. By itself the module can not be used as a
//! 'wireless wire', although such a mode could be implemented in the
//! micro controller. The interface between the RFM73 and the micro controller
//! is a standard 4-PIN SPI interface (MISO, MOSI, CLCK, CSN) plus a CE
//! (Chip Enable) pin. The module also provides an IRQ pin that could be used
//! to speed up the detection of certain events within the module. 
//! The library does not used this pin. 
//! The datasheet seems to claim that the SPI input pins are 5V-tolerant, 
//! but experiments have shown that this is not the case. 
//!
//! An RFM73 module operates in the 2.4 GHz band. 
//! Within that band a frequency can be selected in 1 MHz steps. 
//! RFM73 modules can communicate only when they use the same frequency.
//!
//! The RFM73 module operates on a packet basis. 
//! Each packet has a destination address.
//! By default the address is a 5 byte value, but a 4 or 3 byte
//! address can also be configured. 
//! For successful communication the RFM73 modules that are involved must
//! be configures for the same address length, and the receiving
//! module(s) must have one of their so-called pipes configured
//! for the address used by the transmitter.
//!
//! An RFM73 contains 6 so-called data pipes that can receive packages.
//! Each pipe has an address, and will receive only messages
//! for that address.
//! Pipes 0 and 1 have a full (up to 5 bytes) address. 
//! For pipes 2 .. 6 only the least significant byte can be
//! configured, the other bytes are copied from the address of pipe 1.
//! The length of the address (3, 4 or 5 bytes) is the same for 
//! transmission and for all 6 receive data pipes.
//!
//! A packet contains 1 .. 32 user data bytes. 
//! The packet length can either be fixed or flexible.
//! When the packet length is fixed each receiving pipe is configured
//! with its packet length. The length of a package that is sent is 
//! determined by the number of bytes written into the transmit buffer,
//! and it it must match the length configured for the receiving pipe.
//! When the packet length is flexible the length of a package is again
//! determined by the number of bytes written into the transmit, but in
//! this case this number is transmitted in the message, and on the
//! receiving side it can be retrieved by the R_RX_PL_WID command
//! (rfm73_channel_payload_size function).
//!
//! The simple way to send a package is without acknowledgment.
//! The RFM73 just transmits the package and considers it done.
//! It can be received by zero, one, or multiple RFM73 modules, but
//! the sending modules does not care and has no way of knowing.
//! For this simple mode of communication the involved RFM73's
//! - must be configured for the same channel frequency
//! - must use the same address length
//! - the receiving RFM73 must have a receive pipe configured
//!   for the same address as used by the transmitting RFM73
//!
//! Alternatively a package can be sent with acknowledgment and
//! (if necessary) retransmission. 
//! In this case the RFM73 will send the message, and consider it
//! done only when it receives an acknowledge for it.
//! If it does not receive an acknowledge within a fixed amount of
//! time it will re-transmit the package, up to a set maximum.
//! A receiving RFM73 can automatically send an acknowledgment
//! for a successfully received package. 
//! For this to work the same requirements as for simple unacknowledged
//! communication apply, and additionally on both RFM73's
//! - CRC must be enabled (length 1 or 2) on both modules
//! - receive data pipe 0 must be configured for the same address 
//!   as the module uses for transmitting
//!
//! The automatic retransmission provided by the RFM73 uses a fixed
//! retransmission timeout, which is probably fine for correcting an 
//! occasional packet loss due to interference from other 2.4 GHz sources,
//! but it is potentially ineffective when the interference is caused by
//! another RFM73 that uses the same timeout! In such cases the 
//! micro controller should implement its own retransmission scheme,
//! with for instance a random timeout, and maybe an exponential 
//! back off.
//! 
//
//***************************************************************************//

//***************************************************************************//
//
//! \page hardware RFM73 hardware interface
//!
//! \image html smd-pins.jpg
//! \image rtf smd-pins.jpg
//!
//! The RFM73 uses a standard 4-wire SPI interface.
//! It also provides an active low interrupt pin, which could be used to
//! avoid polling. This library does not use the interrupt pin.
//! The RFM73 also has a CE (chip enable) input, which must be de-asserted
//! to put the chip in standby or power-down mode, and must be cycled
//! to switch between receive and transmit mode. Hence the interface
//! uses 5 data pins, plus ground and power (3.3V):
//! - GND : common ground
//! - VDD : 3.3V power
//! - CE : Chip Enable, active high, micro controller output
//! - CSN : Chip Select Negative, active low, micro controller output
//! - SCK : Serial ClocK, micro controller output
//! - MOSI : Master Out Slave In, micro controller output
//! - MISO : Master In Slave Out, micro controller input
//! - IRQ : Interrupt ReQuest, not used
//!
//! When the micro controller operates at 3.3 Volt (or lower, 
//! the RFM73 datasheet claims operation down to 1.9 Volt) all lines, 
//! including power,  
//! can be connected directly to the micro controller. 
//! If you are experimenting and want to protect yourself against
//! problems when you accidentally configure the pin connected to MISO
//! as output, you could insert a suitable series resistor in this line.
//! 2k2 seems to be a good choice.
//!
//! When the micro controller operates at 5 Volt there are three possible
//! issues:
//! - power : the RFM73 operates at 1.9 .. 3.3 Volt, so the 5 Volt must somehow
//!   be reduced to a voltage within this range
//! - data from the micro controller to the RFM73 : the datasheet seems to
//!   claim that the inputs are 5V tolerant. You can also convert the 5V
//!   outputs of your micro controller down to 3.3V.
//! - data from RFM73 to the micro controller : in most cases this will not 
//!   be a problem, but you might want to check the minimum voltage required
//!   by your micro controller to reliably detect a logic 1. For instance a 
//!   PIC requires ~ 2.0 Volt on a TTL level input, but 0.8 * VDD on a
//!   Schmitt Trigger input! And you must consider this at the maximum VDD 
//!   for the micro controller, which can be 5.5 Volt when delivered by 
//!   an 7805 or an USB port.
//!
//! There are various ways to create a 3.3 Volt supply for the RFM73 from a
//! 5 Volt supply. I prefer to use a low-drop low-quiescent
//! current voltage linear regulator. Read the datasheet of the regulator
//! carefully: some put very stringent requirements on the value and impedance
//! of the decoupling capacitors. My favorite is classic LM117/LM317 model
//! which is produced by many companies
//!
//! \image html ldo-reg.png
//! \image rtf ldo-reg.png
//!
//! A crude way to create the power for the RFM73 is to use a 
//! resistor divider. I would do this only in an experimental
//! setup on my desk, never in a final product.
//! The source impedance of the divider causes a drop in the 
//! voltage when the RFM73 uses more current.
//! This drop can be reduced by lowering the resistor values, but at
//! the cost of a higher current through the resistors. The RFM73
//! can operate down to 1.9 Volt, but at that level the micro controller 
//! might not reliably recognize a logic 1 from the RFM73. Another issue
//! is the dissipation in the resistors. The circuit below is a compromise.
//! It uses three equal-valued resistors because I don't stock many
//! different resistor values.
//! The idle current through the resistors is 83 mA at 5.5 Volt, in
//! this situation the RFM73 gets 3.7 Volt. That is slightly high, but
//! probably not a big problem.
//! When the RFM73 draws its maximum current of 23 mA when the 
//! micro controller's power is at 4.5 Volt the RFM73 still gets 2.6 Volt. 
//! You might want to double-check that the micro controller accepts ~ 2 Volt
//! as a logic 1. 
//!
//! TBW: picture
//! 
//
//***************************************************************************//

/*! \brief Pin number of IRQ contact on RFM73 module.*/
#define RFM73_IRQ_PIN     PB5
/*! \brief PORT register to IRQ contact on RFM73 module.*/
#define RFM73_IRQ_PORT    PORTB
/*! \brief PIN register of IRQ contact on RFM73 module.*/
#define RFM73_IRQ_IN      PINB
/*! \brief DDR register of IRQ contact on RFM73 module.*/
#define RFM73_IRQ_DIR     DDRB
/*! \brief Pin number of CE contact on RFM73 module.*/
#define RFM73_CE_PIN      PB4
/*! \brief PORT register to CE contact on RFM73 module.*/
#define RFM73_CE_PORT     PORTB
/*! \brief PIN register of CE contact on RFM73 module.*/
#define RFM73_CE_IN       PINB
/*! \brief DDR register of CE contact on RFM73 module.*/
#define RFM73_CE_DIR      DDRB
/*! \brief Pin number of CSN contact on RFM73 module.*/
#define RFM73_CSN_PIN     PB0
/*! \brief PORT register to CSN contact on RFM73 module.*/
#define RFM73_CSN_PORT    PORTB
/*! \brief PIN register of CSN contact on RFM73 module.*/
#define RFM73_CSN_IN      PINB
/*! \brief DDR register of CSN contact on RFM73 module.*/
#define RFM73_CSN_DIR     DDRB

/*! \brief Setting high level on CE line.*/
#define RFM73_CE_HIGH     RFM73_CE_PORT |= (1 << RFM73_CE_PIN)
/*! \brief Setting low level on CE line.*/
#define RFM73_CE_LOW      RFM73_CE_PORT &=~(1 << RFM73_CE_PIN)
/* It is important to never stay in TX mode for more than 4ms at one time. */
//#define RFM73_CE_TX_PULSE RFM73_CE_HIGH; _delay_us(20); RFM73_CE_LOW
/*! \brief Setting high level on CSN line.*/
#define RFM73_CSN_HIGH    RFM73_CSN_PORT |= (1 << RFM73_CSN_PIN)
/*! \brief Setting low level on CSN line.*/
#define RFM73_CSN_LOW     RFM73_CSN_PORT &=~(1 << RFM73_CSN_PIN)
//#define RFM73_CSN_PULSE   RFM73_CSN_HIGH; _delay_us(10); RFM73_CSN_LOW

/*! \brief Value sent to first argument of rfm73_set_rf_params function. Set
output power to -10 dBm.*/
#define RFM73_OUT_PWR_MINUS10DBM   0
/*! \brief Value sent to first argument of rfm73_set_rf_params function. Set
output power to -5 dBm.*/
#define RFM73_OUT_PWR_MINUS5DBM    1
/*! \brief Value sent to first argument of rfm73_set_rf_params function. Set
output power to 0 dBm.*/
#define RFM73_OUT_PWR_0DBM         2
/*! \brief Value sent to first argument of rfm73_set_rf_params function. Set
output power to +5 dBm.*/
#define RFM73_OUT_PWR_PLUS5DBM     3

/*! \brief Value sent to second argument of rfm73_set_rf_params function. Set
low noise amplifier gain to 0 dBm.*/
#define RFM73_LNA_GAIN_HIGH        1
/*! \brief Value sent to second argument of rfm73_set_rf_params function. Set
low noise amplifier gain to -20 dBm.*/
#define RFM73_LNA_GAIN_LOW         0

/*! \brief Value sent to third argument of rfm73_set_rf_params function. Set
air data rate to 1 Mbps.*/
#define RFM73_DATA_RATE_1MBPS      0
/*! \brief Value sent to third argument of rfm73_set_rf_params function. Set
air data rate to 2 Mbps.*/
#define RFM73_DATA_RATE_2MBPS      0b10
/*! \brief Value sent to third argument of rfm73_set_rf_params function. Set
air data rate to 250 kbps.*/
#define RFM73_DATA_RATE_250KBPS    0b01

/*! \brief Value sent to first argument of rfm73_send_packet function. Enables
auto-acknowledge of sending packet. Auto-acknowledge must be explicitly enabled
by rfm73_set_autoack function.*/
#define RFM73_TX_WITH_ACK          1
/*! \brief Value sent to first argument of rfm73_send_packet function. Disables
auto-acknowledge of sending packet.*/
#define RFM73_TX_WITH_NOACK        0

/*! \brief Value sent to first argument of rfm73_receive_packet function. In
this case function will automatically transmit received message back.*/
#define RFM73_RX_WITH_ACK          1
/*! \brief Value sent to first argument of rfm73_receive_packet function. In
this case function will only receive new message.*/
#define RFM73_RX_WITH_NOACK        0

/*! \brief Maximum data size that could be sent in one packet.*/
#define RFM73_MAX_PACKET_LEN       32

/*! \brief Value sent to rfm73_set_address_width function and determine address
field width of 3 bytes of all modules in network.*/
#define RFM73_ADR_WID_3BYTES       0b01
/*! \brief Value sent to rfm73_set_address_width function and determine address
field width of 4 bytes of all modules in network.*/
#define RFM73_ADR_WID_4BYTES       0b10
/*! \brief Value sent to rfm73_set_address_width function and determine address
field width of 5 bytes of all modules in network.*/
#define RFM73_ADR_WID_5BYTES       0b11

/* set tx mode */
void rfm73_tx_mode();
/* set rx mode (high energy drain if power up) */
void rfm73_rx_mode();
/* power up module, enabling most of it features */
void rfm73_power_up();
/* power down module */
void rfm73_power_down();

/* initilize module ith some default settings */
void rfm73_init(uint8_t out_pwr, uint8_t lna_gain, uint8_t data_rate,
                uint8_t ch);
/* masking events that affects IRQ pin */
void rfm73_mask_int(uint8_t mask_rx_dr, uint8_t mask_tx_ds,
                    uint8_t mask_max_rt);
/* set rf params */
void rfm73_set_rf_params(uint8_t out_pwr, uint8_t lna_gain, uint8_t data_rate);
/* set address width */
void rfm73_set_address_width(uint8_t aw);
/* set autoretransmit params: time and number of tries */
void rfm73_set_autort(uint16_t rt_time, uint8_t rt_count);
/* set rf channel from 0 to 127 */
void rfm73_set_channel(uint8_t _cfg);
/* set receiver payload width of specified pipeline */
void rfm73_set_rx_payload_width(uint8_t pipeline, uint8_t wid);
/* enables and disables dynamic payload of specifiec pipelines */
void rfm73_set_dyn_payload(uint8_t pipeline_mask);
/* returns selected channel */
uint8_t rfm73_get_channel();
/* returns receiver's payload width of specified pipeline */
uint8_t rfm73_get_rx_payload_width(uint8_t pipeline);
/* returns enabled state of dynamic payload feature of specified pipelines */
uint8_t rfm73_get_dyn_payload();
/* returns rf quality characteristics: number of packets lost since channel
change and number of times last packet was retransmitted */
uint8_t rfm73_observe(uint8_t* packet_lost, uint8_t* retrans_count);
/* returns carrier detect status bit */
uint8_t rfm73_carrier_detect();
/* checks and receives new packet */
uint8_t rfm73_receive_packet(uint8_t type, uint8_t* data_buf, uint8_t* len);
/* sends data */
uint8_t rfm73_send_packet(uint8_t type, uint8_t* pbuf, uint8_t len);
/* find receivers within all datarates and all channels from ch to 127 */
uint8_t rfm73_find_receiver(uint8_t* ch, uint8_t* dr);

#endif
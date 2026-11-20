/*
  Custom variant for the NULI Radio Board (RADIOGAGA) rev V1.

  MCU : ATSAMD51J19A-M (QFN-64)   -- same die as an Adafruit Feather M4 Express
  Radio: Semtech SX1262IMLTRT (LoRa) on SERCOM0 SPI

  This variant is a self-contained copy that lives in the repo
  (variants/radio_board) so the system framework files are never touched.
  It only defines the pins this board actually wires up; it is intentionally
  minimal (no I2C, no Serial1, no analog pins) and exists to get SPI comms
  to the radio working.

  Verified against the KiCad schematic (RadioBoard-RADIOGAGA, untitled.kicad_sch):

    Net        SAMD51   SERCOM0 pad (func D / PIO_SERCOM_ALT)
    --------   ------   ------------------------------------
    SCK        PA05     PAD1   -> SPI SCK
    MISO       PA06     PAD2   -> SPI MISO (DIPO = PAD2)
    MOSI       PA07     PAD3   -> SPI MOSI (DOPO = PAD3, SCK on PAD1)
    NSS        PA04     (GPIO chip select, driven by software)
    BUSY_RF    PA08     (GPIO input)
    INT_RF     PA09     (GPIO input / DIO1 IRQ, EXTINT9)
    RF_RESET   PB11     (GPIO output, active low reset)
    LED (D2)   PB17     (status LED)
    USB D-/D+  PA24/25  (native USB)

  NOTE: function D (PIO_SERCOM_ALT) is mandatory for the SPI pins. With
  function C, SCK would land on PAD3 and MOSI on PAD1, which is not a legal
  SAMD5x DOPO combination and SPI would silently fail.
*/

#ifndef _VARIANT_RADIO_BOARD_
#define _VARIANT_RADIO_BOARD_

// The definitions here needs a SAMD core >=1.6.10
#define ARDUINO_SAMD_VARIANT_COMPLIANCE 10610

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

/** Frequency of the board main oscillator (32.768 kHz crystal, Y1) */
#define VARIANT_MAINOSC		(32768ul)

/** Master clock frequency */
#define VARIANT_MCK        (F_CPU)

#define VARIANT_GCLK0_FREQ (F_CPU)
#define VARIANT_GCLK1_FREQ (48000000UL)
#define VARIANT_GCLK2_FREQ (100000000UL)

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
#include "SERCOM.h"
#endif // __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

// Number of pins defined in the PinDescription array
#define PINS_COUNT           (11u)
#define NUM_DIGITAL_PINS     (11u)
#define NUM_ANALOG_INPUTS    (0u)
#define NUM_ANALOG_OUTPUTS   (0u)
#define analogInputToDigitalPin(p)  (-1)

#define digitalPinToPort(P)        ( &(PORT->Group[g_APinDescription[P].ulPort]) )
#define digitalPinToBitMask(P)     ( 1 << g_APinDescription[P].ulPin )
#define portOutputRegister(port)   ( &(port->OUT.reg) )
#define portInputRegister(port)    ( &(port->IN.reg) )
#define portModeRegister(port)     ( &(port->DIR.reg) )
#define digitalPinHasPWM(P)        ( g_APinDescription[P].ulPWMChannel != NOT_ON_PWM || g_APinDescription[P].ulTCChannel != NOT_ON_TIMER )

#define ADC_RESOLUTION		12

/*
 * Analog/DAC are not used on this minimal variant, but the Arduino core
 * (wiring_analog.c) references these symbols unconditionally. Define them
 * as inert sentinels so the core compiles; they intentionally do NOT point
 * at real pins (e.g. PA05 is SCK and must not be claimed as a DAC).
 */
#define PIN_A0               (0xFFu)
#define PIN_DAC0             (0xFFu)
#define PIN_DAC1             (0xFEu)
static const uint8_t A0 = PIN_A0;

/*
 * Radio (SX1262) control pins -- use these names in sketches.
 */
#define PIN_RADIO_NSS        (0u)   // PA04
#define PIN_RADIO_BUSY       (4u)   // PA08
#define PIN_RADIO_DIO1       (5u)   // PA09
#define PIN_RADIO_RESET      (6u)   // PB11

// LED (D2)
#define PIN_LED_13           (7u)   // PB17
#define PIN_LED              PIN_LED_13
#define LED_BUILTIN          PIN_LED_13

/*
 * SPI Interface -- SERCOM0, function D (PIO_SERCOM_ALT)
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO         (2u)   // PA06  SERCOM0/PAD[2]
#define PIN_SPI_MOSI         (3u)   // PA07  SERCOM0/PAD[3]
#define PIN_SPI_SCK          (1u)   // PA05  SERCOM0/PAD[1]
#define PERIPH_SPI           sercom0
#define PAD_SPI_TX           SPI_PAD_3_SCK_1
#define PAD_SPI_RX           SERCOM_RX_PAD_2

static const uint8_t SS	  = PIN_RADIO_NSS ; // PA04 software chip select
static const uint8_t MOSI = PIN_SPI_MOSI ;
static const uint8_t MISO = PIN_SPI_MISO ;
static const uint8_t SCK  = PIN_SPI_SCK ;

/*
 * No I2C / Wire on this minimal variant
 */
#define WIRE_INTERFACES_COUNT 0

/*
 * USB
 */
#define PIN_USB_DM          (8ul)  // PA24
#define PIN_USB_DP          (9ul)  // PA25
// No USB host on this board; placeholder pin keeps the core's USB host
// stack compiling (the entry is a NOT_A_PIN, so pinMode() is a no-op).
#define PIN_USB_HOST_ENABLE (10ul)

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus

/*	=========================
 *	===== SERCOM DEFINITION
 *	=========================
*/
extern SERCOM sercom0;
extern SERCOM sercom1;
extern SERCOM sercom2;
extern SERCOM sercom3;
extern SERCOM sercom4;
extern SERCOM sercom5;

#endif

// Serial monitor / native USB CDC
#define SERIAL_PORT_USBVIRTUAL      Serial
#define SERIAL_PORT_MONITOR         Serial

#endif /* _VARIANT_RADIO_BOARD_ */

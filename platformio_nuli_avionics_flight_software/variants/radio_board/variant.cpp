/*
  Custom variant for the NULI Radio Board (RADIOGAGA) rev V1.
  See variant.h for the full pin-mapping rationale.

  Pin index -> SAMD51 port mapping (must match the macros in variant.h):

    0  PA04  NSS   (GPIO chip select)
    1  PA05  SCK   (SERCOM0/PAD1, func D)
    2  PA06  MISO  (SERCOM0/PAD2, func D)
    3  PA07  MOSI  (SERCOM0/PAD3, func D)
    4  PA08  BUSY  (GPIO input)
    5  PA09  DIO1  (GPIO input / IRQ, EXTINT9)
    6  PB11  RESET (GPIO output)
    7  PB17  LED   (status)
    8  PA24  USB D-
    9  PA25  USB D+
*/

#include "variant.h"

const PinDescription g_APinDescription[]=
{
  // 0: PA04 - Radio NSS (software chip select)
  { PORTA,  4, PIO_DIGITAL,     PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_4 },

  // 1..3: SPI on SERCOM0 - MUST be PIO_SERCOM_ALT (peripheral function D)
  { PORTA,  5, PIO_SERCOM_ALT,  PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_5 }, // SCK : SERCOM0/PAD[1]
  { PORTA,  6, PIO_SERCOM_ALT,  PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_6 }, // MISO: SERCOM0/PAD[2]
  { PORTA,  7, PIO_SERCOM_ALT,  PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 }, // MOSI: SERCOM0/PAD[3]

  // 4: PA08 - Radio BUSY (input)
  { PORTA,  8, PIO_DIGITAL,     PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_8 },

  // 5: PA09 - Radio DIO1 / IRQ (input, external interrupt capable)
  { PORTA,  9, PIO_DIGITAL,     PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_9 },

  // 6: PB11 - Radio RESET (output, active low)
  { PORTB, 11, PIO_DIGITAL,     PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_11 },

  // 7: PB17 - Status LED (D2)
  { PORTB, 17, PIO_DIGITAL,     PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_1 },

  // 8..9: USB (native)
  { PORTA, 24, PIO_COM,         PIN_ATTR_NONE,    No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // USB/DM
  { PORTA, 25, PIO_COM,         PIN_ATTR_NONE,    No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // USB/DP

  // 10: USB host enable - not present on this board (placeholder, no-op)
  { NOT_A_PORT, 0, PIO_NOT_A_PIN, PIN_ATTR_NONE,  No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },
} ;

const void* g_apTCInstances[TCC_INST_NUM+TC_INST_NUM]={ TCC0, TCC1, TCC2, TCC3, TCC4, TC0, TC1, TC2, TC3, TC4, TC5 } ;
const uint32_t GCLK_CLKCTRL_IDs[TCC_INST_NUM+TC_INST_NUM] = { TCC0_GCLK_ID, TCC1_GCLK_ID, TCC2_GCLK_ID, TCC3_GCLK_ID, TCC4_GCLK_ID, TC0_GCLK_ID, TC1_GCLK_ID, TC2_GCLK_ID, TC3_GCLK_ID, TC4_GCLK_ID, TC5_GCLK_ID } ;

// Multi-serial objects instantiation (SPI uses sercom0; the rest are
// instantiated for completeness / linker compatibility).
SERCOM sercom0( SERCOM0 ) ;
SERCOM sercom1( SERCOM1 ) ;
SERCOM sercom2( SERCOM2 ) ;
SERCOM sercom3( SERCOM3 ) ;
SERCOM sercom4( SERCOM4 ) ;
SERCOM sercom5( SERCOM5 ) ;

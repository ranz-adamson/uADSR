// uADSR project
// original by zerogroupsystems 
// via ModWiggler: https://modwiggler.com/forum/viewtopic.php?p=3216248#p3216248
// Adopted for GitHub by Ranz Adamson
// October 2023

#ifndef MCP48X1_H
#define MCP48X1_H

/*
         ____
  VDD --|1  8|-- VOUT
  CS  --|2  7|-- VSS
  SCK --|3  6|-- SHDN
  SDI --|4  5|-- LDAC
        -----

  VDD  - connect to +5V
  CS   - connect to MCP48X1_LATCH_PIN
  SCK  - connect to MCP48X1_CLOCK_PIN
  SDI  - connect to MCP48X1_DATA_PIN
  LDAC - connect to GND
  SHDN - ?
  VSS  - connect to GND
  VOUT - output

*/

#ifndef RESOLUTION_BITS
#define RESOLUTION_BITS 10
#endif // RESOLUTION_BITS

#if   RESOLUTION_BITS == 8

#define MSB_VALUE_PART(v)  (v & 0xF0) >> 4
#define LSB_VALUE_PART(v)  (v & 0x0F) << 4

#elif RESOLUTION_BITS == 10

#define MSB_VALUE_PART(v)  (v & 0x3C0) >> 6
#define LSB_VALUE_PART(v)  (v & 0x3F)  << 2

#elif RESOLUTION_BITS == 12

#define MSB_VALUE_PART(v)  (v & 0xF00) >> 8
#define LSB_VALUE_PART(v)  (v & 0x7F )
#define LSB_VALUE_PART

#endif // RESOLUTION_BITS

#if defined( __AVR_ATtinyX4__ )

#define PIN_TO_BIT(p)  (p <= 3 ? _BV(p) : _BV(10 - p))
#define PIN_TO_PORT(p) (p <= 3 ? PORTB : PORTA)
#define PIN_TO_DDR(p)  (p <= 3 ? DDRB : DDRA)

#elif defined ( __AVR_ATtinyX5__ )

#define PIN_TO_BIT(p)  _BV(p)
#define PIN_TO_PORT(p) PORTB
#define PIN_TO_DDR(p)  DDRB

#elif defined( __AVR_ATtinyX313__ )

#define PIN_TO_BIT(p)  (p == 0 || p == 3 ? _BV(0) : \
                        (p == 1 || p == 2 ? _BV(1) : \
                         (p <= 8 ? _BV(p - 2) : \
                          (p < 17 ? _BV(p - 9) : _BV(2)))))
#define PIN_TO_PORT(p) (p <= 1 || (p >=4 && p <=8) ? PORTD : \
                        (p == 2 || p == 3 || p == 17) ? PORTA : PORTB)
#define PIN_TO_DDR(p)  (p <= 1 || (p >=4 && p <=8) ? DDRD : \
                        (p == 2 || p == 3 || p == 17) ? DDRA : DDRB)

#else  // assume ATMEGAx8

#define PIN_TO_BIT(p)  (p <= 7 ? _BV(p) : \
                        (p <= 13 ? _BV(p - 8) : _BV(p - 14)))
#define PIN_TO_PORT(p) (p <= 7 ? PORTD : \
                        (p <= 13 ? PORTB : PORTC))
#define PIN_TO_DDR(p)  (p <= 7 ? DDRD : \
                        (p <= 13 ? DDRB : DDRC))

#endif

#ifndef MCP48X1_CLOCK_PIN
#define MCP48X1_CLOCK_PIN  5
#endif // MCP48X1_CLOCK_PIN

#ifndef MCP48X1_DATA_PIN
#define MCP48X1_DATA_PIN  6
#endif // MCP48X1_DATA_PIN

#ifndef MCP48X1_LATCH_PIN
#define MCP48X1_LATCH_PIN  7
#endif // MCP48X1_LATCH_PIN

#define CLOCK_PORT PIN_TO_PORT(MCP48X1_CLOCK_PIN)
#define DATA_PORT  PIN_TO_PORT(MCP48X1_DATA_PIN)
#define LATCH_PORT PIN_TO_PORT(MCP48X1_LATCH_PIN)

#define CLOCK_TICK CLOCK_PORT |= PIN_TO_BIT(MCP48X1_CLOCK_PIN); \
                   CLOCK_PORT &= ~PIN_TO_BIT(MCP48X1_CLOCK_PIN)

#define BIT_SELECT(v, b) (v &1 << b) >> b

#ifdef __cplusplus
#define GET_BYTE_VALUES getByteValues
#define SEND_BYTE       sendByte
#else
#define GET_BYTE_VALUES MCP48x1_getByteValues
#define SEND_BYTE       MCP48x1_sendByte
void MCP48x1_getByteValues(
    int value, unsigned short *firstByte, unsigned short *secondByte,
    int gain_enable, int shutdown);
void MCP48x1_sendByte(unsigned short value);
#endif  // __cplusplus

#ifdef __cplusplus
class MCP48x1 {
 public:
#endif  // __cplusplus

#ifdef __cplusplus
  MCP48x1()
#else
  void MCP48x1_init()
#endif  // __cplusplus
  {
    // Enable pins for output.
    PIN_TO_DDR(MCP48X1_CLOCK_PIN) |= PIN_TO_BIT(MCP48X1_CLOCK_PIN);
    PIN_TO_DDR(MCP48X1_DATA_PIN)  |= PIN_TO_BIT(MCP48X1_DATA_PIN);
    PIN_TO_DDR(MCP48X1_LATCH_PIN) |= PIN_TO_BIT(MCP48X1_LATCH_PIN);
  }

#ifdef __cplusplus
  void write(int value, bool gain_enable=true, bool shutdown=false)
#else
  void MCP48x1_write(int value, int gain_enable, int shutdown)
#endif  // __cplusplus
  {
    unsigned short firstByte;
    unsigned short secondByte;
    GET_BYTE_VALUES(value, &firstByte, &secondByte, gain_enable, shutdown);

    LATCH_PORT &= ~PIN_TO_BIT(MCP48X1_LATCH_PIN);

    SEND_BYTE(firstByte);
    SEND_BYTE(secondByte);

    LATCH_PORT |= PIN_TO_BIT(MCP48X1_LATCH_PIN);
  }

#ifdef __cplusplus
private:
#endif  // __cplusplus

#ifdef __cplusplus
  void getByteValues(
      int value, unsigned short *firstByte, unsigned short *secondByte,
      bool gain_enable=true, bool shutdown=false)
#else
  void MCP48x1_getByteValues(
      int value, unsigned short *firstByte, unsigned short *secondByte,
      int gain_enable, int shutdown)
#endif  // __cplusplus
  {
    *firstByte = (shutdown ? 0 : 1) << 4;         // enable
    *firstByte |= (gain_enable ? 0 : 1) << 5;     // gain
    *firstByte |= MSB_VALUE_PART(value);
    *secondByte = LSB_VALUE_PART(value);
  }
#ifdef __cplusplus
  void sendByte(byte value)
#else
  void MCP48x1_sendByte(unsigned short value)
#endif  // __cplusplus
  {
    int send_values[2] = {
      DATA_PORT & ~PIN_TO_BIT(MCP48X1_DATA_PIN), // LOW
      DATA_PORT | PIN_TO_BIT(MCP48X1_DATA_PIN), // HIGH
    };

    DATA_PORT = send_values[BIT_SELECT(value, 7)];
    CLOCK_TICK;
    DATA_PORT = send_values[BIT_SELECT(value, 6)];
    CLOCK_TICK;
    DATA_PORT = send_values[BIT_SELECT(value, 5)];
    CLOCK_TICK;
    DATA_PORT = send_values[BIT_SELECT(value, 4)];
    CLOCK_TICK;
    DATA_PORT = send_values[BIT_SELECT(value, 3)];
    CLOCK_TICK;
    DATA_PORT = send_values[BIT_SELECT(value, 2)];
    CLOCK_TICK;
    DATA_PORT = send_values[BIT_SELECT(value, 1)];
    CLOCK_TICK;
    DATA_PORT = send_values[(value & 1)];
    CLOCK_TICK;
  }
#ifdef __cplusplus
};
#endif  // __cplusplus

#endif // MCP48X1_H

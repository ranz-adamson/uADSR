// uADSR project
// original by zerogroupsystems 
// via ModWiggler: https://modwiggler.com/forum/viewtopic.php?p=3216248#p3216248
// Adopted for GitHub by Ranz Adamson
// October 2023

#include <avr/io.h>

#include <TimerOne.h>
#include <MCP48x1.h>


#define ATTACK_PIN          A2
#define ATTACK_CHANNEL       2

#define DECAY_PIN           A3
#define DECAY_CHANNEL        3

#define SUSTAIN_PIN         A4
#define SUSTAIN_CHANNEL      4

#define RELEASE_PIN         A5
#define RELEASE_CHANNEL      5

#define GATE_PIN             2
#define TRIGGER_PIN          3

#define LED_PIN              9

#define SAMPLES_PER_SECOND 10000

typedef enum {
  IDLE,
  ATTACK,
  DECAY,
  SUSTAIN,
  RELEASE
} ADSRMode;

#define MAX_STATE     ((float)(1 << RESOLUTION_BITS) - 1)

#define ANALOG_VALUE  (ADCL+(ADCH<<8))

// min rise time is 2ms, max rise time 3.0s
#define MIN_RISE      0.002
#define MAX_RISE      3.0

#define INCR_VALUE(P) MAX_STATE / (MAX_RISE + (MIN_RISE - MAX_RISE) / 1023.0 * P) / SAMPLES_PER_SECOND

volatile float state = 0;
volatile ADSRMode mode = IDLE;

volatile float        attack_incr = 0.0;
volatile float        decay_incr = 0.0;
volatile unsigned int sustain_value = 0;
volatile float        release_incr = 0.0;
volatile bool         triggered = false;

MCP48x1 mcp4811;


void play() {
  switch(mode) {
    case ATTACK:
      state += attack_incr;
      if (state >= MAX_STATE ) {
        triggered = false;
        mode = DECAY;
      }
    break;

    case DECAY:
      state -= decay_incr;
      if (state <= sustain_value) {
        mode = SUSTAIN;
      }
    break;

    case SUSTAIN:
      state = sustain_value;
    break;

    case RELEASE:
      state -= release_incr;
      if (state <= 0) {
        mode = IDLE;
      }
    break;

    case IDLE:
    default:
      return;
    break;
  }
  if (state < 0) {
    state = 0;
  } else if (state > MAX_STATE) {
    state = MAX_STATE;
  }
  mcp4811.write((int) state);
}

void setup() {
  pinMode(ATTACK_PIN, INPUT);
  pinMode(DECAY_PIN, INPUT);
  pinMode(SUSTAIN_PIN, INPUT);
  pinMode(RELEASE_PIN, INPUT);
  pinMode(GATE_PIN, INPUT);
  pinMode(TRIGGER_PIN, INPUT);
  // Unused; maybe add intensity?
  pinMode(LED_PIN, OUTPUT);
  Timer1.initialize(1000000 / SAMPLES_PER_SECOND);  // uSEC
  Timer1.attachInterrupt(play);
}

void loop() {
  // start first read
  ADSRMode read_mode = ATTACK;
  ADMUX = (1<<REFS0) | ATTACK_CHANNEL;
  ADCSRA |= (1 << ADSC);

  bool started = false;

  while (true) {
    if ((ADCSRA & (1<<ADIF))) {
      // analog read completed
      // clear read
      ADCSRA |= (1<<ADIF);
      if (read_mode == ATTACK) {
        attack_incr = INCR_VALUE(ANALOG_VALUE);
        read_mode = DECAY;
        ADMUX = (1<<REFS0) | DECAY_CHANNEL;
      } else if (read_mode == DECAY) {
        decay_incr = INCR_VALUE(ANALOG_VALUE);
        read_mode = SUSTAIN;
        ADMUX = (1<<REFS0) | SUSTAIN_CHANNEL;
      } else if (read_mode == SUSTAIN) {
        sustain_value = MAX_STATE / 1023.0 * ANALOG_VALUE;
        read_mode = RELEASE;
        ADMUX = (1<<REFS0) | RELEASE_CHANNEL;
      } else if (read_mode == RELEASE) {
        release_incr = INCR_VALUE(ANALOG_VALUE);
        read_mode = ATTACK;
        ADMUX = (1<<REFS0) | ATTACK_CHANNEL;
      }
      // start the next read
      ADCSRA |= (1 << ADSC);
    }
    //byte gate_value = PORTD & _BV(GATE_PIN);
    byte gate_value = digitalRead(GATE_PIN);
    byte trigger_value = digitalRead(TRIGGER_PIN);
    if (trigger_value > 0 && !triggered) {
      triggered = true;
      mode = ATTACK;
    }
    if (gate_value > 0 && !started) {
      mode = ATTACK;
      started = true;
    } else if (gate_value == LOW && !triggered) {
      mode = RELEASE;
      started = false;
    }
  }
}


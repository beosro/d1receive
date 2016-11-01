#include "RCTrx.hpp"
#include <stdlib.h>
#include "Arduino.h"

#define DEBUG_LOG_LEVEL 0
#define INFO_LOG_LEVEL 1
#define ERROR_LOG_LEVEL 2
#define NO_LOG_LEVEL 3

// #define logLevel INFO_LOG_LEVEL
// #define DebugLog if (logLevel >= DEBUG_LOG_LEVEL) Serial
// #define InfoLog if (logLevel >= INFO_LOG_LEVEL) Serial
// #define ErrorLog if (logLevel >= ERROR_LOG_LEVEL) Serial

#define DebugLog if (false) Serial

#ifdef ESP8266
    // interrupt handler and related code must be in RAM on ESP8266,
    // according to issue #46.
    #define RECEIVE_ATTR ICACHE_RAM_ATTR
#else
    #define RECEIVE_ATTR
#endif

const long tolerancePercent = 25;
int pinState = 0;

#define near(referenceDuration, duration) abs(referenceDuration - duration) < referenceDuration * tolerancePercent / 100

void printDuration(const char *name, long duration) {
  DebugLog.printf("  %ld < %ld < %ld\n", duration - duration * tolerancePercent / 100, duration, duration + duration * tolerancePercent / 100);
}

void invertPin(unsigned long microDelay, int pin) {
  digitalWrite(pin, pinState);
  delayMicroseconds(microDelay);
  pinState = !pinState;
}

HandleDuration::HandleDuration(long duration)
    : requiredDuration(duration) {
  noRead = false;
  misState = 0;
  DebugLog.printf("HandleDuration::HandleDuration\n");
  printDuration("duration", duration);
}

int HandleDuration::sendPulse(State& state, Code code, int sendPin) {
  invertPin(requiredDuration, sendPin);
  state++;
}

bool HandleDuration::processPulse(State& state, long duration) {
  DebugLog.printf("       HandleDuration::processPulse(%d, %ld)\n", state, duration);
  if (near(requiredDuration, duration)) {
    state++;
    return false;
  } else {
    state = misState;
    return noRead;
  }
}

void printAttrs() {
  // DebugLog.printf("Handle2PulseDataBytes::Handle2PulseDataBytes\n");
  // printDuration("bit0DurationA", bit0DurationA);
  // printDuration("bit0DurationB", bit0DurationB);
  // printDuration("bit1DurationA", bit1DurationA);
  // printDuration("bit1DurationB", bit1DurationB);
}


Handle2PulseDataBytes::Handle2PulseDataBytes(CodeReceivedCallback callback, int bitCount, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB)
    : callback(callback), bitCount(bitCount), bit0DurationA(bit0DurationA), bit0DurationB(bit0DurationB), bit1DurationA(bit1DurationA), bit1DurationB(bit1DurationB),
    bitState(0), receivedBits(0), code(0) {
}

int Handle2PulseDataBytes::sendPulse(State& state, Code code, int sendPin) {
  for (int i = bitCount - 1; i >= 0; i--) {
    if (code & 1 << i) {
      invertPin(bit1DurationA, sendPin);
      invertPin(bit1DurationB, sendPin);
    } else {
      invertPin(bit0DurationA, sendPin);
      invertPin(bit0DurationB, sendPin);
    }
  }
  state++;
}

bool Handle2PulseDataBytes::processPulse(State& state, long duration) {
  DebugLog.printf("Handle2PulseDataBytes::processPulse(%d, %ld) %lu", state, duration, bitState);
  if (bitState == 0) {  // receive first duration for a 0 or 1 bit
    if (near(bit0DurationA, duration)) {
      // valid start of a 0 bit
      DebugLog.printf("\n");
      bitState = 1;
      return false;
    } else if (near(duration, bit1DurationA)) {
      // valid start of a 1 bit
      DebugLog.printf("\n");
      bitState = 2;
      return false;
    }
  } else if (bitState == 1) {  // receive second duration for a 0 bit
    if (near(bit0DurationB, duration)) {
      // received a valid 0 bit
      DebugLog.printf(" 0 bit %d of %d\n", receivedBits, bitCount);

      code <<= 1;
      receivedBits++;
      bitState = 0;
      if (receivedBits >= bitCount) {
        callback(code);
        receivedBits = 0;
        bitState = 0;
        state = 0; // all bits read - go to next state
      }
      return false;
    }
  } else if (bitState == 2) {  // receive second duration for a 0 bit
    if (near(bit1DurationB, duration)) {
      // received a valid 1 bit
      DebugLog.printf(" 1 bit %d of %d\n", receivedBits, bitCount);

      code <<= 1;
      code += 1;
      receivedBits++;
      bitState = 0;
      if (receivedBits >= bitCount) {
        callback(code);
        receivedBits = 0;
        bitState = 0;
        state = 0; // all bits read - go to next state
      }
      return false;
    }
  }
  DebugLog.printf(" error - invalid bit pulse duration\n");
  code = 0;
  receivedBits = 0;
  bitState = 0;
  state = 0;
  return false;
}


Handle4PulseDataBytes::Handle4PulseDataBytes(CodeReceivedCallback callback, int bitCount, long bitDurationShort, long bitDurationLong)
    : callback(callback), bitCount(bitCount), bitDurationShort(bitDurationShort), bitDurationLong(bitDurationLong),
    bitState(0), receivedBits(0), code(0) {
}

int Handle4PulseDataBytes::sendPulse(State& state, Code code, int sendPin) {
  for (int i = bitCount - 1; i >= 0; i--) {
    if (code & 1 << i) {
      invertPin(bitDurationShort, sendPin);
      invertPin(bitDurationLong, sendPin);
      invertPin(bitDurationShort, sendPin);
      invertPin(bitDurationShort, sendPin);
    } else {
      invertPin(bitDurationShort, sendPin);
      invertPin(bitDurationShort, sendPin);
      invertPin(bitDurationShort, sendPin);
      invertPin(bitDurationLong, sendPin);
    }
  }
  state++;
}

bool Handle4PulseDataBytes::processPulse(State& state, long duration) {
  return false;
}


ProtocolHandlerProove1::ProtocolHandlerProove1(long startDurationA, long startDurationB,
    long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB)
  : pulseHandlers{
    new HandleDuration(startDurationA),
    (new HandleDuration(startDurationB))->setNoRead(true),
    new Handle2PulseDataBytes([&](Code code) {
      callback(code);
    }, 24, bit0DurationA, bit0DurationB, bit1DurationA, bit1DurationB)
  } {
    state = 0;
}

void ProtocolHandlerProove1::send(Code code, int sendPin) {
  pinState = 1;
  for (int i = 0; i < 10; i++) {
    int state = 0;
    while (state < (sizeof pulseHandlers / sizeof pulseHandlers[0])) {
      pulseHandlers[state]->sendPulse(state, code, sendPin);
    }
  }
  digitalWrite(sendPin, 0);
}

void ProtocolHandlerProove1::process(long duration) {
  // DebugLog.printf("P");
  int n = 10;
  while (pulseHandlers[state]->processPulse(state, duration)) {
    if (n-- <= 0) {
      break;
    }
  };
}


ProtocolHandlerProove2::ProtocolHandlerProove2(long startDurationShort, long startDurationLong,
    long bitDurationShort, long bitDurationLong, long stopDurationShort, long stopDurationLong)
  : pulseHandlers{
      new HandleDuration(startDurationShort),
      (new HandleDuration(startDurationLong))->setNoRead(true),
      new Handle4PulseDataBytes([&](Code code) {
        callback(code);
      }, 32, bitDurationShort, bitDurationLong),
      new HandleDuration(stopDurationShort),
      new HandleDuration(stopDurationLong)
    } {
    state = 0;
}

void ProtocolHandlerProove2::send(Code code, int sendPin) {
  pinState = 1;
  for (int i = 0; i < 10; i++) {
    int state = 0;
    while (state < (sizeof pulseHandlers / sizeof pulseHandlers[0])) {
      pulseHandlers[state]->sendPulse(state, code, sendPin);
    }
  }
  digitalWrite(sendPin, 0);
}

void ProtocolHandlerProove2::process(long duration) {
  // DebugLog.printf("P");
  int n = 10;
  while (pulseHandlers[state]->processPulse(state, duration)) {
    if (n-- <= 0) {
      break;
    }
  };
}


/*
  RCTrx - Arduino libary for remote control outlet switches
*/
RCTrx::RCTrx()
  : protocolHandlers{
    new ProtocolHandlerProove1(350, 31*350, 350, 3*350, 3*350, 350),
    new ProtocolHandlerProove2(289, 2685, 301, 1236, 200, 10580)
  } {
}

RCTrx* RCTrx::inst = 0;

void RCTrx::process(long duration) {
  protocolHandlers[0]->process(duration);
}

void RCTrx::send(Code code, int protocolId) {
  protocolHandlers[protocolId]->send(code, sendPin);
}

void RCTrx::enableReceive(int pin) {
  inst = this;
  pinMode(pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(pin), handleInterrupt, CHANGE);
}

void RCTrx::setSendPin(int pin) {
  sendPin = pin;
  pinMode(pin, OUTPUT);
}

void RECEIVE_ATTR RCTrx::handleInterrupt() {
  static unsigned long lastTime = 0;
  const long time = micros();
  const unsigned int duration = time - lastTime;
  inst->process(duration);
  lastTime = time;
}

void RCTrx::sendTimeArray(long *times, int count) {
  pinState = 1;
  for (int i = 0; i < count; i++) {
    invertPin(times[i], sendPin);
  }
}

void RCTrx::sendCode(Code code) {
  const unsigned long t = 350;
  for (int i = 0; i < 10; i++) {
    digitalWrite(sendPin, 1);
    delayMicroseconds(289);
    digitalWrite(sendPin, 0);
    delayMicroseconds(2685);
    for (int j = 31; j >= 0; j--) {
      if (code & (1 << j)) {
        // one - klkk
        digitalWrite(sendPin, 1);
        delayMicroseconds(301);
        digitalWrite(sendPin, 0);
        delayMicroseconds(1236);
        digitalWrite(sendPin, 1);
        delayMicroseconds(301);
        digitalWrite(sendPin, 0);
        delayMicroseconds(211);
      } else {
        // zero - kkkl
        digitalWrite(sendPin, 1);
        delayMicroseconds(301);
        digitalWrite(sendPin, 0);
        delayMicroseconds(211);
        digitalWrite(sendPin, 1);
        delayMicroseconds(301);
        digitalWrite(sendPin, 0);
        delayMicroseconds(1236);
      }
    }
    digitalWrite(sendPin, 1);
    delayMicroseconds(200);
    digitalWrite(sendPin, 0);
    delayMicroseconds(10580);
  }
}

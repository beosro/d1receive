#include "RCTrx.hpp"
#include <stdlib.h>
#include "Arduino.h"

const long tolerancePercent = 49;

bool near(long a, long b) {
  long tolerance = b * tolerancePercent / 100;
  bool result = abs(a - b) < tolerance;
  return result;
}

void printDuration(const char *name, long duration) {
  Serial.printf("  %ld < %ld < %ld\n", duration - duration * tolerancePercent / 100, duration, duration + duration * tolerancePercent / 100);
}


HandleDuration::HandleDuration(long duration)
    : requiredDuration(duration) {
  noRead = false;
  misState = 0;
  Serial.printf("HandleDuration::HandleDuration\n");
  printDuration("duration", duration);
}

void HandleDuration::sendPulse() {
}

bool HandleDuration::processPulse(State& state, long duration) {
  Serial.printf("       HandleDuration::processPulse(%d, %ld)\n", state, duration);
  if (near(duration, requiredDuration)) {
    state++;
    return false;
  } else {
    state = misState;
    return noRead;
  }
}

void printAttrs() {
  // Serial.printf("Handle2PulseDataBytes::Handle2PulseDataBytes\n");
  // printDuration("bit0DurationA", bit0DurationA);
  // printDuration("bit0DurationB", bit0DurationB);
  // printDuration("bit1DurationA", bit1DurationA);
  // printDuration("bit1DurationB", bit1DurationB);
}


Handle2PulseDataBytes::Handle2PulseDataBytes(ReceivedCodeCallback callback, int count, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB)
    : callback(callback), count(count), bit0DurationA(bit0DurationA), bit0DurationB(bit0DurationB), bit1DurationA(bit1DurationA), bit1DurationB(bit1DurationB),
    bitState(0), receivedBits(0), code(0) {
}

void Handle2PulseDataBytes::sendPulse() {
}

bool Handle2PulseDataBytes::processPulse(State& state, long duration) {
  Serial.printf("Handle2PulseDataBytes::processPulse(%d, %ld) %lu", state, duration, bitState);
  if (bitState == 0) {  // receive first duration for a 0 or 1 bit
    if (near(duration, bit0DurationA)) {
      // valid start of a 0 bit
      Serial.printf("\n");
      bitState = 1;
      return false;
    } else if (near(duration, bit1DurationA)) {
      // valid start of a 1 bit
      Serial.printf("\n");
      bitState = 2;
      return false;
    }
    long tolerance = bit1DurationA * tolerancePercent / 100;
    bool result = abs(duration - bit1DurationA) < tolerance;
    Serial.printf(" abs(%ld, %ld)=%ld < tolerance %ld = %d\n",
        duration, bit1DurationA, abs(duration - bit1DurationA), tolerance, result);
  } else if (bitState == 1) {  // receive second duration for a 0 bit
    if (near(duration, bit0DurationB)) {
      // received a valid 0 bit
      Serial.printf(" 0 bit %d of %d\n", receivedBits, count);

      code <<= 1;
      receivedBits++;
      bitState = 0;
      if (receivedBits >= count) {
        callback(code);
        receivedBits = 0;
        bitState = 0;
        state = 0; // all bits read - go to next state
      }
      return false;
    }
  } else if (bitState == 2) {  // receive second duration for a 0 bit
    if (near(duration, bit1DurationB)) {
      // received a valid 1 bit
      Serial.printf(" 1 bit %d of %d\n", receivedBits, count);

      code <<= 1;
      code += 1;
      receivedBits++;
      bitState = 0;
      if (receivedBits >= count) {
        callback(code);
        receivedBits = 0;
        bitState = 0;
        state = 0; // all bits read - go to next state
      }
      return false;
    }
  }
  Serial.printf(" error - invalid bit pulse duration\n");
  code = 0;
  receivedBits = 0;
  bitState = 0;
  state = 0;
  return false;
}


ProtocolHandlerProove1::ProtocolHandlerProove1(long startDurationA, long startDurationB, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB)
  : pulseHandlers{
    new HandleDuration(startDurationA),
    (new HandleDuration(startDurationB))->setNoRead(true),
    new Handle2PulseDataBytes([&](Code code) {
      callback(code);
    }, 24, bit0DurationA, bit0DurationB, bit1DurationA, bit1DurationB)
  } {
    state = 0;
}

void ProtocolHandlerProove1::send(Code code, int protocolId) {
}

void ProtocolHandlerProove1::process(long duration) {
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
RCTrx::RCTrx() : protocolHandler(new ProtocolHandlerProove1(350, 31*350, 350, 3*350, 3*350, 350)) {
}

void RCTrx::process(long duration) {
  protocolHandler->process(duration);
}

void RCTrx::send(Code, int protocolId) {
}

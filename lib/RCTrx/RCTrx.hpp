/*
  RCTrx - Arduino libary for remote control outlet switches
*/
#ifndef _RCTrx_h
#define _RCTrx_h
#include <functional>

typedef unsigned long Code;
typedef int State;
typedef std::function<void(int)> CodeReceivedCallback;

class PulseHandler {
protected:
  int misState;
  bool noRead;
public:
  virtual int sendPulse(State& state, Code code, int sendPin) = 0;
  virtual bool processPulse(State& prevState, long duration) = 0;     // process duration and return a new state
};

class HandleDuration : public PulseHandler {
  long requiredDuration;
public:
  HandleDuration(long duration);
  int sendPulse(State& state, Code code, int sendPin);
  bool processPulse(State& prevState, long duration);     // process duration and return a new state
  HandleDuration* setNoRead(bool b) {noRead = b; return this;};
  HandleDuration* setMismatchState(State state) {misState = state; return this;};
};

class Handle2PulseDataBytes : public PulseHandler {
  CodeReceivedCallback callback;
  Code code;
  int bitCount;
  long bit0DurationA;
  long bit0DurationB;
  long bit1DurationA;
  long bit1DurationB;
  int bitState;
  int receivedBits;
public:
  Handle2PulseDataBytes(CodeReceivedCallback callback, int count, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB);
  int sendPulse(State& state, Code code, int sendPin);
  bool processPulse(State& prevState, long duration);     // process duration and return a new state
};

class ProtocolHandler {
protected:
  Code receivedCode;
  bool availableCode;
  State state;
  CodeReceivedCallback callback;
public:
  ProtocolHandler() : availableCode(false) {};
  virtual void send(Code code, int sendPin) = 0;
  virtual void process(long duration) = 0;     // process pulse with duration, and return true if a valid packet is received
  int getState() {return state;};
  void onCodeReceived(CodeReceivedCallback aCallback) {callback = aCallback;};
};

class ProtocolHandlerProove1 : public ProtocolHandler {
  PulseHandler* pulseHandlers[3];
public:
  ProtocolHandlerProove1(long startDurationA, long startDurationB, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB);
  void send(Code code, int sendPin);
  void process(long duration);
};

class RCTrx {
  ProtocolHandler *protocolHandler;
  static void handleInterrupt();
  static RCTrx* inst;
  int sendPin;
public:
  RCTrx();
  void process(long duration);
  void send(Code code, int protocolId);
  State getState() {return protocolHandler->getState();};
  void onCodeReceived(CodeReceivedCallback callback) {protocolHandler->onCodeReceived(callback);};
  void enableReceive(int pin);
  void setSendPin(int pin);
  void sendTimeArray(long *times, int count);
  void sendCode(Code code);
};

#endif

/*
  RCTrx - Arduino libary for remote control outlet switches
*/
#ifndef _RCTrx_h
#define _RCTrx_h
#include <functional>

typedef unsigned long Code;
typedef int State;
typedef std::function<void(int)> ReceivedCodeCallback;

class ProtocolHandler;

class PulseHandler {
protected:
  int misState;
  bool noRead;
public:
  virtual void sendPulse() = 0;
  virtual bool processPulse(State& prevState, long duration) = 0;     // process duration and return a new state
};

class HandleDuration : public PulseHandler {
  long requiredDuration;
public:
  HandleDuration(long duration);
  void sendPulse();
  bool processPulse(State& prevState, long duration);     // process duration and return a new state
  HandleDuration* setNoRead(bool b) {noRead = b; return this;};
  HandleDuration* setMismatchState(State state) {misState = state; return this;};
};

class Handle2PulseDataBytes : public PulseHandler {
  ReceivedCodeCallback callback;
  Code code;
  int count;
  long bit0DurationA;
  long bit0DurationB;
  long bit1DurationA;
  long bit1DurationB;
  int bitState;
  int receivedBits;
public:
  Handle2PulseDataBytes(ReceivedCodeCallback callback, int count, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB);
  void sendPulse();
  bool processPulse(State& prevState, long duration);     // process duration and return a new state
};

typedef void (ProtocolHandler::* PHCallback)(Code) ;

class ProtocolHandler {
protected:
  Code receivedCode;
  bool availableCode;
  State state;
  // void phCallback(Code code) {};
  ReceivedCodeCallback callback;
public:
  ProtocolHandler() : availableCode(false) {};
  virtual void send(Code code, int protocolId) = 0;
  virtual void process(long duration) = 0;     // process pulse with duration, and return true if a valid packet is received
  int getState() {return state;};
  void onCodeReceived(ReceivedCodeCallback aCallback) {callback = aCallback;};
};

class ProtocolHandlerProove1 : public ProtocolHandler {
  PulseHandler* pulseHandlers[3];
public:
  ProtocolHandlerProove1(long startDurationA, long startDurationB, long bit0DurationA, long bit0DurationB, long bit1DurationA, long bit1DurationB);
  void send(Code code, int protocolId);
  void process(long duration);
};

class RCTrx {
  ProtocolHandler *protocolHandler;
public:
  RCTrx();
  void process(long duration);
  void send(Code, int protocolId);
  State getState() {return protocolHandler->getState();};
  void onCodeReceived(ReceivedCodeCallback callback) {protocolHandler->onCodeReceived(callback);};
};

#endif

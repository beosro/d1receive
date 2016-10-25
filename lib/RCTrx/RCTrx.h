/*
  RCTrx - Arduino libary for remote control outlet switches
*/
#ifndef _RCTrx_h
#define _RCTrx_h

class Seq {
public:
  int a;
  int b;
  Seq(int a, int b);
  Seq();
};

class RCTrx {
  int pulseLength;
  Seq start;
  Seq bit0;
  Seq bit1;

  int state;

public:
  RCTrx(const int pulseLength, const Seq start, const Seq bit0, const Seq bit1);
  bool process(long duration);
};

#endif

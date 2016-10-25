#include "RCTrx.h"
#include <stdlib.h>

Seq::Seq(const int aA, const int aB) {
  a = aA;
  b = aB;
}

Seq::Seq() {
}

RCTrx::RCTrx(const int aPulseLength, const Seq aStart, const Seq aBit0, const Seq aBit1) {
  pulseLength = aPulseLength;
  start = aStart;
  bit0 = aBit0;
  bit1 = aBit1;
  state = 0;
}



bool near(long a, long b) {
  return abs(a - b) < 15;
}

bool RCTrx::process(long duration) {
  static int code = 0;

  if (state == 0) { // wait for start.a
    code = 0;
    if (near(duration, pulseLength * start.a)) {
      state = 1;
    }
  } else if (state == 1) { // check start.b
    if (near(duration, pulseLength * start.b)) {
      state = 2;
    } else {
      state = 0;  // invalid back to initial state
    }
  } else if (state == 2) {  // collect data bits - bit0.a
    code <<= 1;
    if (near(duration, pulseLength * bit0.a)) {
      state = 3;
    } else if (near(duration, pulseLength * bit1.a)) {
      
    } else {
      state = 0;  // invalid back to initial state
    }
  } else if (state == 3) {  // collect data bits - bit0.b

  } else if (state == 4) {

  } else if (state == 5) {

  }
  return false;
}

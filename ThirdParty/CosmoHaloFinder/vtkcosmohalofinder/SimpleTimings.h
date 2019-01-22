#ifndef SIMPLE_TIMINGS_H
#define SIMPLE_TIMINGS_H

#include <string>
#include <vector>

#include <Partition.h>

#define NAMEWIDTH 5

using std::string;
using std::vector;

namespace cosmotk {

class SimpleTimings {

 public:

  typedef int TimerRef;

  SimpleTimings();
  ~SimpleTimings();

  static TimerRef getTimer(const char *nm);
  static void startTimer(TimerRef tr);
  static void stopTimer(TimerRef tr);
  static double getCurrent(TimerRef tr);
  static void timerStats(TimerRef, bool uselaccum = false, int nameWidth=NAMEWIDTH);
  static void alltimerStats(TimerRef, bool uselaccum = false, int nameWidth=NAMEWIDTH);
  static void stopTimerStats(TimerRef, bool uselaccum = false, int nameWidth=NAMEWIDTH);
  static void fakeTimer(TimerRef tr, double time);
  static void fakeTimerStats(TimerRef tr,double time, bool uselaccum = false, int nameWidth=NAMEWIDTH);
  static void accumStats(int nameWidth=NAMEWIDTH);
  static void timingStats(double t, string name, int nameWidth=NAMEWIDTH);
  static void alltimingStats(double t, string name, int nameWidth=NAMEWIDTH);

 private:

  static vector <string> names;
  static vector <double> start;
  static vector <double> stop;
  static vector <double> accum;
  static vector <double> laccum;
};

}
#endif

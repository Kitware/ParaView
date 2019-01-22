#include "SimpleTimings.h"

#include <cstdio>

#include <mpi.h>

using namespace std;

namespace cosmotk {


vector<string> SimpleTimings::names;
vector<double> SimpleTimings::start;
vector<double> SimpleTimings::stop;
vector<double> SimpleTimings::accum;
vector<double> SimpleTimings::laccum;

SimpleTimings::SimpleTimings() {}
SimpleTimings::~SimpleTimings() {}

SimpleTimings::TimerRef SimpleTimings::getTimer(const char *nm) {
  string s(nm);
  TimerRef tr;
  for(tr=0; tr< static_cast<TimerRef>(names.size()); tr++) {
    if(names[tr].compare(s) == 0)
      break;
  }
  if( names.size()==0 || tr == static_cast<TimerRef>(names.size()) ) {
    names.push_back(s);
    start.push_back(0.0);
    stop.push_back(0.0);
    accum.push_back(0.0);
    laccum.push_back(0.0);
  }
  return tr;
}

void SimpleTimings::startTimer(TimerRef tr) {
  start[tr] = MPI_Wtime();
}

void SimpleTimings::stopTimer(TimerRef tr) {
  stop[tr] = MPI_Wtime();
  accum[tr] += stop[tr] - start[tr];
  laccum[tr] += stop[tr] - start[tr];
}

double SimpleTimings::getCurrent(TimerRef tr) {
  return MPI_Wtime() - start[tr];
}

void SimpleTimings::fakeTimer(TimerRef tr, double time) {
  start[tr] = 0.0;
  stop[tr] = time;
  accum[tr] += stop[tr] - start[tr];
  laccum[tr] += stop[tr] - start[tr];
}

void SimpleTimings::timingStats(double t, string name, int nameWidth) {
  double tmax, tmin, tavg;
  int rank;
  double numranks;

  rank = Partition::getMyProc();
  numranks = 1.0*Partition::getNumProc();

  MPI_Allreduce(&t, &tmax, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&t, &tmin, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  MPI_Allreduce(&t, &tavg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  tavg /= numranks;

  if(rank==0) {
    string tmpname(name.c_str());
    tmpname.resize(nameWidth, ' ');
    printf("%s  max  %.3e s  avg  %.3e s  min  %.3e s\n",
           tmpname.c_str(), tmax, tavg, tmin);
    fflush(stdout);

    fprintf(stderr, "%s done\n", tmpname.c_str());
    fflush(stderr);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  return;
}

void SimpleTimings::alltimingStats(double t, string name, int nameWidth) {
  double tmax, tmin, tavg;
  int rank;
  double numranks;

  rank = Partition::getMyProc();
  numranks = 1.0*Partition::getNumProc();
  
  MPI_Allreduce(&t, &tmax, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&t, &tmin, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  MPI_Allreduce(&t, &tavg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  tavg /= numranks;
  //MPI_Gather(&t, 1, MPI_DOUBLE, times, nranks, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if(rank==0) {
    string tmpname(name.c_str());
    tmpname.resize(nameWidth, ' ');
    printf("%s  max  %.3e s  avg  %.3e s  min  %.3e s\n",
           tmpname.c_str(), tmax, tavg, tmin);
    fflush(stdout);
    //for(int i=0; i < nranks; i++){
     // printf("%d, %10.5f\n",i,times[i]);
    //}

    fprintf(stderr, "%s done\n", tmpname.c_str());
    fflush(stderr);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  printf("%d, %10.5f\n",rank,t);
  fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);

  return;
}

void SimpleTimings::timerStats(TimerRef tr, bool uselaccum, int nameWidth) {
  double t;
  t = uselaccum ? laccum[tr] : stop[tr] - start[tr];
  timingStats(t, names[tr], nameWidth);
  laccum[tr] = 0.0;
  return;
}

void SimpleTimings::alltimerStats(TimerRef tr, bool uselaccum, int nameWidth) {
  double t;
  t = uselaccum ? laccum[tr] : stop[tr] - start[tr];
  alltimingStats(t, names[tr], nameWidth);
  laccum[tr] = 0.0;
  return;
}

void SimpleTimings::stopTimerStats(TimerRef tr, bool uselaccum, int nameWidth) {
  stopTimer(tr);
  timerStats(tr, uselaccum, nameWidth);
  return;
}

void SimpleTimings::fakeTimerStats(TimerRef tr, double time, bool uselaccum, int nameWidth) {
  fakeTimer(tr, time);
  timerStats(tr, uselaccum, nameWidth);
  return;
}

void SimpleTimings::accumStats(int nameWidth) {
  int rank;
//  double numranks;
  rank = Partition::getMyProc();
//  numranks = 1.0*Partition::getNumProc();

  if(rank==0)
    printf("ACCUMULATED STATS\n");

  for(TimerRef tr=0; tr < static_cast<TimerRef>(names.size()); tr++) {
    timingStats(accum[tr], names[tr], nameWidth);
  }

  return;
}

}

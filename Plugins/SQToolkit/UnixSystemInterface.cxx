/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "UnixSystemInterface.h"

#include "FsUtils.h"
#include "SQMacros.h"

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::istringstream;
#include <iostream>
using std::cerr;
using std::endl;

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fenv.h>
#include <signal.h>
#include <execinfo.h>
#include <stdio.h>

void backtrace_handler(int sigNo, siginfo_t *sigInfo, void *context);

//-----------------------------------------------------------------------------
UnixSystemInterface::UnixSystemInterface()
      :
    Pid(-1)
{
  // get the process id
  this->Pid=getpid();
}

//------------------------------------------------------------------------------
string UnixSystemInterface::GetHostName()
{
  char hostName[1024];
  gethostname(hostName,1024);
  return string(hostName);
}

//-----------------------------------------------------------------------------
int UnixSystemInterface::Exec(string &cmd)
{
  vector<string> argStrs;
  istringstream is(cmd);
  while (is.good())
      {
      string argStr;
      is >> argStr;
      argStrs.push_back(argStr);
      }

  pid_t childPid=fork();
  if (childPid==0)
    {

    int nArgStrs=argStrs.size();
    char **args=(char **)malloc((nArgStrs+1)*sizeof(char *));
    for (int i=0; i<nArgStrs; ++i)
      {
      int argLen=argStrs[i].size();
      args[i]=(char *)malloc((argLen+1)*sizeof(char));
      strncpy(args[i],argStrs[i].c_str(),argLen+1);
      #if defined pqSQProcessMonitorDEBUG
      cerr << "[" << args[i] << "]" << endl;
      #endif
      }
    args[nArgStrs]=0;
    execvp(args[0],args);

    sqErrorMacro(cerr,"Failed to exec \"" << cmd << "\".");

    return -1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::StackTraceOnError(int enable)
{
  static int saOrigValid=0;
  static struct sigaction saSEGVOrig;
  static struct sigaction saILLOrig;
  static struct sigaction saBUSOrig;
  static struct sigaction saFPEOrig;

  if (enable && !saOrigValid)
    {
    // save the current actions
    sigaction(SIGSEGV,0,&saSEGVOrig);
    sigaction(SIGILL,0,&saILLOrig);
    sigaction(SIGBUS,0,&saBUSOrig);
    sigaction(SIGFPE,0,&saFPEOrig);
  
    // enable read, disable write
    saOrigValid=1;

    // install ours
    struct sigaction sa;
    sa.sa_sigaction=&backtrace_handler;
    sa.sa_flags=SA_SIGINFO|SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV,&sa,0);
    sigaction(SIGILL,&sa,0);
    sigaction(SIGBUS,&sa,0);
    sigaction(SIGFPE,&sa,0);
    }
  else
  if (!enable && saOrigValid)
    {
    // restore previous actions
    sigaction(SIGSEGV,&saSEGVOrig,0);
    sigaction(SIGILL,&saILLOrig,0);
    sigaction(SIGBUS,&saBUSOrig,0);
    sigaction(SIGFPE,&saFPEOrig,0);

    // enable write, disable read
    saOrigValid=0;
    }
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::CatchAllFloatingPointExceptions(int enable)
{
  if (enable)
    {
    feenableexcept(FE_ALL_EXCEPT);
    }
  else
    {
    fedisableexcept(FE_ALL_EXCEPT);
    }
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::CatchDIVBYZERO(int enable)
{
  if (enable)
    {
    feenableexcept(FE_DIVBYZERO);
    }
  else
    {
    fedisableexcept(FE_DIVBYZERO);
    }
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::CatchINEXACT(int enable)
{
  if (enable)
    {
    feenableexcept(FE_INEXACT);
    }
  else
    {
    fedisableexcept(FE_INEXACT);
    }
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::CatchINVALID(int enable)
{
  if (enable)
    {
    feenableexcept(FE_INVALID);
    }
  else
    {
    fedisableexcept(FE_INVALID);
    }
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::CatchOVERFLOW(int enable)
{
  if (enable)
    {
    feenableexcept(FE_OVERFLOW);
    }
  else
    {
    fedisableexcept(FE_OVERFLOW);
    }
}

//-----------------------------------------------------------------------------
void UnixSystemInterface::CatchUNDERFLOW(int enable)
{
  if (enable)
    {
    feenableexcept(FE_UNDERFLOW);
    }
  else
    {
    fedisableexcept(FE_UNDERFLOW);
    }
}


//*****************************************************************************
void backtrace_handler(
      int sigNo,
      siginfo_t *sigInfo,
      void */*sigContext*/)
{
  cerr << "[" << getpid() << "] ";

  switch (sigNo)
    {
    case SIGFPE:
      cerr << "Caught SIGFPE type ";
      switch (sigInfo->si_code)
        {
        case FPE_INTDIV:
          cerr << "integer division by zero";
          break;

        case FPE_INTOVF:
          cerr << "integer overflow";
          break;

        case FPE_FLTDIV:
          cerr << "floating point divide by zero";
          break;

        case FPE_FLTOVF:
          cerr << "floating point overflow";
          break;

        case FPE_FLTUND:
          cerr << "floating point underflow";
          break;

        case FPE_FLTRES:
          cerr << "floating point inexact result";
          break;

        case FPE_FLTINV:
          cerr << "floating point invalid operation";
          break;

        case FPE_FLTSUB:
          cerr << "floating point subscript out of range";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    case SIGSEGV:
      cerr << "Caught SIGSEGV type ";
      switch (sigInfo->si_code)
        {
        case SEGV_MAPERR:
          cerr << "address not mapped to object";
          break;

        case SEGV_ACCERR:
          cerr << "invalid permission for mapped object";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    case SIGBUS:
      cerr << "Caught SIGBUS type ";
      switch (sigInfo->si_code)
        {
        case BUS_ADRALN:
          cerr << "invalid address alignment";
          break;

        case BUS_ADRERR:
          cerr << "non-exestent physical address";
          break;

        case BUS_OBJERR:
          cerr << "object specific hardware error";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    case SIGILL:
      cerr << "Caught SIGILL type ";
      switch (sigInfo->si_code)
        {
        case ILL_ILLOPC:
          cerr << "illegal opcode";
          break;

        case ILL_ILLOPN:
          cerr << "illegal operand";
          break;

        case ILL_ILLADR:
          cerr << "illegal addressing mode.";
          break;

        case ILL_ILLTRP:
          cerr << "illegal trap";

        case ILL_PRVOPC:
          cerr << "privileged opcode";
          break;

        case ILL_PRVREG:
          cerr << "privileged register";
          break;

        case ILL_COPROC:
          cerr << "co-processor error";
          break;

        case ILL_BADSTK:
          cerr << "internal stack error";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    default:
      cerr << "Caught " << sigNo;
      break;
    }

  // dump a backtrace to stderr
  cerr << "." << endl;

  void *stack[128];
  int n=backtrace(stack,128);
  backtrace_symbols_fd(stack,n,2);

  abort();
}

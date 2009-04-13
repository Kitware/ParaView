#ifndef BootstrapConfigure_h
#define BootstrapConfigure_h

// Configures things for Windows run, we are going to assume that Linux users
// are capable of configuring their environment correctly.

#include <cstdlib>
#include <cstring>

#ifdef WIN32
#include <Windows.h>
#endif

//*****************************************************************************
void UnixToWindowsPath(char *p)
{
  size_t n=strlen(p);
  for (size_t j=0; j<n; ++j)
    {
    if (p[j]=='/')
      {
      p[j]='\\';
      }
    }
}

//*****************************************************************************
void InitializeEnvironment(int argc, char **argv)
{
  #ifdef WIN32
  // Set path to ParaView build.
  char PV3_BIN[]="/home/burlen/ext/kitware_cvs/PV3-VisIt/bin/";
  UnixToWindowsPath(PV3_BIN);
  SetDllDirectory(PV3_BIN);
  // Clean up command tail.
  for (int i=1; i<argc; ++i)
    {
    UnixToWindowsPath(argv[i]);
    }
  #endif
}

#endif

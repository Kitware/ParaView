/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef   	__vtkProcessModuleAutoMPI_h
# define   	__vtkProcessModuleAutoMPI_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/Process.h>
#include <vtksys/stl/string>
#include <vtksys/stl/vector>

#include "vtkObject.h"
#include "vtkClientServerID.h" // needed for UniqueID.
#include "vtkSocket.h"

class vtkGetFreePort: public vtkSocket
{
public:
  int getFreePort();
};

class VTK_EXPORT vtkProcessModuleAutoMPI: public vtkObject
{
public:
  static vtkProcessModuleAutoMPI* New();
  vtkTypeMacro(vtkProcessModuleAutoMPI, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static int UseMulticoreProcessors;
  static void setUseMulticoreProcessors(int val);
  vtkProcessModuleAutoMPI();
  ~vtkProcessModuleAutoMPI();

  int isPossible();
  int ConnectToRemoteBuiltInSelf ();

private:
  void SeparateArguments(const char* str,
                         vtkstd::vector<vtkstd::string>& flags);
  int StartRemoteBuiltInSelf (const char* servername,int port);
  void ReportCommand (const char* const* command, const char* name);
  int StartServer (vtksysProcess* server, const char* name,
                   vtkstd::vector<char>& out,
                   vtkstd::vector<char>& err);
  int WaitForAndPrintLine (const char* pname, vtksysProcess* process,
                           vtkstd::string& line, double timeout,
                           vtkstd::vector<char>& out,
                           vtkstd::vector<char>& err,
                           int* foundWaiting);
  int WaitForLine (vtksysProcess* process, vtkstd::string& line,
                              double timeout,
                              vtkstd::vector<char>& out,
                              vtkstd::vector<char>& err);
  void PrintLine (const char* pname, const char* line);
  void CreateCommandLine (vtksys_stl::vector<const char*>& commandLine,
                          const char* paraView,
                          const char* numProc,
                          int port);
  bool CollectConfiguredOptions ();
  bool SetMPIRun(vtkstd::string mpiexec);

private:
  // This specify the preflags and post flags that can be set using:
  // VTK_MPI_PRENUMPROC_FLAGS VTK_MPI_PREFLAGS / VTK_MPI_POSTFLAGS at config time
  vtkstd::vector<vtkstd::string> MPIPreNumProcFlags;
  vtkstd::vector<vtkstd::string> MPIPreFlags;
  vtkstd::vector<vtkstd::string> MPIPostFlags;

  // MPIServerFlags allows you to specify flags specific for
  // the client or the server
  vtkstd::vector<vtkstd::string> MPIServerPreFlags;
  vtkstd::vector<vtkstd::string> MPIServerPostFlags;

  int TotalMulticoreProcessors;
  double TimeOut;
  vtkstd::string ParaView;  // fullpath to paraview executable
  vtkstd::string ParaViewServer;  // fullpath to paraview server executable
  vtkstd::string MPINumProcessFlag;
  vtkstd::string MPIServerNumProcessFlag;
  vtkstd::string MPIRun;  // fullpath to mpirun executable
  vtkstd::string CurrentPrintLineName;

  vtkGetFreePort *FreePort;
};
#endif 	    /* !__vktProcessModuleAutoMPI_h */

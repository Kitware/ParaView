/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleAutoMPI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessModuleAutoMPI.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSocket.h"
#include <vtksys/SystemTools.hxx>

#include "vtkSocket.h"
#include "vtksys/Process.h"

#include <sstream>
#include <string>
#include <vector>

bool vtkProcessModuleAutoMPI::EnableAutoMPI = 0;
int vtkProcessModuleAutoMPI::NumberOfCores = 0;

namespace
{
class vtkGetFreePort : public vtkSocket
{
public:
  static vtkGetFreePort* New();
  vtkTypeMacro(vtkGetFreePort, vtkSocket);
  int GetFreePort()
  {
    this->SocketDescriptor = this->CreateSocket();
    if (!this->SocketDescriptor)
    {
      vtkErrorMacro("Failed to create socket.");
      return -1;
    }

    if (this->BindSocket(this->SocketDescriptor, 0))
    {
      vtkErrorMacro("Failed to bind socket.");
      return -1;
    }

    int port = this->GetPort(this->SocketDescriptor);
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;
    return port;
  }

private:
  vtkGetFreePort() = default;
  ~vtkGetFreePort() override = default;
};
vtkStandardNewMacro(vtkGetFreePort);

void vtkCopy(std::vector<const char*>& dest, const std::vector<std::string>& src)
{
  dest.resize(src.size());
  for (size_t cc = 0; cc < src.size(); cc++)
  {
    dest[cc] = src[cc].c_str();
  }
  dest.push_back(nullptr);
}
}

class vtkProcessModuleAutoMPIInternals
{
public:
  // This specify the preflags and post flags that can be set using:
  // PARAVIEW_MPI_PREFLAGS / PARAVIEW_MPI_POSTFLAGS at config time
  std::vector<std::string> MPIPreFlags;
  std::vector<std::string> MPIPostFlags;

  int TotalMulticoreProcessors;
  std::string ServerExecutablePath; // fullpath to paraview server executable
  std::string MPINumProcessFlag;
  std::string MPIServerNumProcessFlag;
  std::string MPIRun; // fullpath to mpirun executable
  std::string CurrentPrintLineName;

  void SeparateArguments(const char* str, std::vector<std::string>& flags);
  int StartRemoteBuiltInSelf(const char* servername, int port);
  void ReportCommand(const char* const* command, const char* name);
  int StartServer(
    vtksysProcess* server, const char* name, std::vector<char>& out, std::vector<char>& err);
  int WaitForAndPrintLine(const char* pname, vtksysProcess* process, std::string& line,
    double timeout, std::vector<char>& out, std::vector<char>& err, int* foundWaiting);
  int WaitForLine(vtksysProcess* process, std::string& line, double timeout, std::vector<char>& out,
    std::vector<char>& err);
  void PrintLine(const char* pname, const char* line);
  void CreateCommandLine(
    std::vector<std::string>& commandLine, const char* paraView, const char* numProc, int port);
  bool CollectConfiguredOptions();
  bool SetMPIRun(std::string mpiexec);
};

#ifdef _WIN32
#define PARAVIEW_SERVER "pvserver.exe"
#else
#define PARAVIEW_SERVER "pvserver"
#endif

//------------------------------------------------------------------------macro
/*
 * The standard new macro
 */
vtkStandardNewMacro(vtkProcessModuleAutoMPI);

//------------------------------------------------------------------------cnstr
vtkProcessModuleAutoMPI::vtkProcessModuleAutoMPI()
{
  this->Internals = new vtkProcessModuleAutoMPIInternals;
}

//------------------------------------------------------------------------destr
vtkProcessModuleAutoMPI::~vtkProcessModuleAutoMPI()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------static
void vtkProcessModuleAutoMPI::SetEnableAutoMPI(bool val)
{
  vtkProcessModuleAutoMPI::EnableAutoMPI = val;
}

//-----------------------------------------------------------------------static
void vtkProcessModuleAutoMPI::SetNumberOfCores(int val)
{
  vtkProcessModuleAutoMPI::NumberOfCores = val;
}

//-----------------------------------------------------------------------public
void vtkProcessModuleAutoMPI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------public
/*
 * To determine if it is possible to use multi-cores on the system.
 *
 * @return 1 if possible and 0 if not
 */
int vtkProcessModuleAutoMPI::IsPossible()
{
  this->Internals->TotalMulticoreProcessors = vtkProcessModuleAutoMPI::NumberOfCores;

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (this->Internals->TotalMulticoreProcessors > 1 && vtkProcessModuleAutoMPI::EnableAutoMPI &&
    this->Internals->CollectConfiguredOptions())
  {
    return 1;
  }
  else
  {
    return 0;
  }
#else
  return 0;
#endif
}

//-----------------------------------------------------------------------public
/*
 * This function is called the system running paraview is a multicore
 * and if the user chooses so. This enables a parallel server
 * automatically as the default.
 *
 * @return Id for the connection.
 */
int vtkProcessModuleAutoMPI::ConnectToRemoteBuiltInSelf()
{
  vtkGetFreePort* freePort = vtkGetFreePort::New();
  int port = freePort->GetFreePort();
  if (port <= 0)
  {
    freePort->Delete();
    vtkErrorMacro("Failed to determine free port number.");
    return 0;
  }

  freePort->Delete();
  return this->Internals->StartRemoteBuiltInSelf("localhost", port) ? port : 0;
}

//---------------------------------------------------------------------internal
/*
 * This function is a helper function for ConnectToRemoteBuiltInSelf
 * function. This starts a server at the next free port and return the
 * port number over which the server is listening.
 *
 * @param servername IN Sending in the server name (usually localhost or 127.0.0.1)
 * @return 1 for success 0 otherwise
 */
int vtkProcessModuleAutoMPIInternals::StartRemoteBuiltInSelf(
  const char* vtkNotUsed(servername), int port)
{
  // Create a new server process structure
  vtksysProcess* server = nullptr;
  server = vtksysProcess_New();
  if (!server)
  {
    vtksysProcess_Delete(server);
    cerr << "pvTestDriver: Cannot allocate vtksysProcess to run the server.\n";
    return 0;
  }

  // Construct the Command line that will be executed
  std::string serverExe = this->ServerExecutablePath;
  std::string app_dir = vtksys::SystemTools::GetProgramPath(serverExe.c_str());

  vtksysProcess_SetWorkingDirectory(server, app_dir.c_str());

#if defined(_WIN32)
  // On Windows, spaces in the path when launching the MPI job can cause severe
  // issue. So we use the relative executable path for the server. Since we are
  // setting the working directory to be the one containing the server
  // executable, we should not have any issues (mostly).
  serverExe = PARAVIEW_SERVER;
#endif

  std::vector<std::string> serverCommandStr;
  std::vector<const char*> serverCommand;
  this->CreateCommandLine(
    serverCommandStr, serverExe.c_str(), this->MPIServerNumProcessFlag.c_str(), port);
  vtkCopy(serverCommand, serverCommandStr);

  if (vtksysProcess_SetCommand(server, &serverCommand[0]))
  {
    this->ReportCommand(&serverCommand[0], "SUCCESS:");
  }
  else
  {
    this->ReportCommand(&serverCommand[0], "ERROR:");
  }

  std::vector<char> ServerStdOut;
  std::vector<char> ServerStdErr;

  // Start the data server if there is one
  if (!this->StartServer(server, "server", ServerStdOut, ServerStdErr))
  {
    cerr << "vtkProcessModuleAutoMPIInternals: Server never started.\n";
    vtksysProcess_Delete(server);
    ;
    return 0;
  }

  // FIXME: 'server' leaks!!!
  return 1;
}

//---------------------------------------------------------------------internal
/*
 * Used to set the this->Internals->MPIRun variable to the appropriate MPI path
 *
 */
bool vtkProcessModuleAutoMPIInternals::SetMPIRun(std::string mpiexec)
{
  mpiexec = vtksys::SystemTools::GetFilenameName(mpiexec);
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  std::string app_dir = options->GetApplicationPath();
  app_dir = vtksys::SystemTools::GetProgramPath(app_dir.c_str()) + "/" + mpiexec;
  if (vtksys::SystemTools::FileExists(app_dir.c_str(), true))
  {
    this->MPIRun = app_dir;
    return 1;
  }
  else
  {
    return 0;
  }
}

//---------------------------------------------------------------------internal
/*
 * This method collects numerous configuration options which will be
 * used to compute the MPI execution command to start the AutoMPI
 *
 * @return 1 if the configuration options were obtained successfully. 0 if failed
 */
bool vtkProcessModuleAutoMPIInternals::CollectConfiguredOptions()
{
  if (this->ServerExecutablePath.empty())
  {
    vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();

    // Determine the path to 'pvserver'.
    std::vector<std::string> search_paths;

    // same location as the client executable.
    std::string binary_dir = vtksys::SystemTools::GetProgramPath(options->GetApplicationPath());

    search_paths.push_back(binary_dir);

#if defined(__APPLE__)
    // for mac, add path to the bin dir within the application.
    std::string applepath = binary_dir + "/../bin";
    search_paths.push_back(vtksys::SystemTools::CollapseFullPath(applepath.c_str()));
#endif
    this->ServerExecutablePath =
      vtksys::SystemTools::FindProgram(PARAVIEW_SERVER, search_paths, /*no_system_path=*/true);
  }

  if (this->ServerExecutablePath.empty())
  {
    return false;
  }

// now find all the mpi information if mpi run is set
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#ifdef PARAVIEW_MPIEXEC_EXECUTABLE
  if (!this->SetMPIRun(PARAVIEW_MPIEXEC_EXECUTABLE))
  {
    this->MPIRun = PARAVIEW_MPIEXEC_EXECUTABLE;
  }
#else
#error "PARAVIEW_MPIEXEC_EXECUTABLE must be set when PARAVIEW_USE_MPI is on."
#endif
  if (this->TotalMulticoreProcessors > 1)
  {
    int serverNumProc = this->TotalMulticoreProcessors;

#ifdef PARAVIEW_MPI_NUMPROC_FLAG
    this->MPINumProcessFlag = PARAVIEW_MPI_NUMPROC_FLAG;
#else
#error "Error PARAVIEW_MPI_NUMPROC_FLAG must be defined to run test if MPI is on."
#endif
#ifdef PARAVIEW_MPI_PREFLAGS
    this->SeparateArguments(PARAVIEW_MPI_PREFLAGS, this->MPIPreFlags);
#endif
#ifdef PARAVIEW_MPI_POSTFLAGS
    this->SeparateArguments(PARAVIEW_MPI_POSTFLAGS, this->MPIPostFlags);
#endif
    char buf[1024];
    snprintf(buf, sizeof(buf), "%d", serverNumProc);
    this->MPIServerNumProcessFlag = buf;
  }
#endif

  return 1;
}

//---------------------------------------------------------------------internal
/**
 * The command line to be processed is created by this method
 *
 * @param commandLine holds commands which will be processed to start server
 * @param paraView the location of paraview server
 * @param numProc the total number of processes for MPI
 * @param port the port where the server will be listening
 */
void vtkProcessModuleAutoMPIInternals::CreateCommandLine(
  std::vector<std::string>& commandLine, const char* paraView, const char* numProc, int port)
{
  if (this->MPIRun.size())
  {
    commandLine.push_back(this->MPIRun.c_str());

    commandLine.push_back(this->MPINumProcessFlag.c_str());
    commandLine.push_back(numProc);
    for (unsigned int i = 0; i < this->MPIPreFlags.size(); ++i)
    {
      commandLine.push_back(this->MPIPreFlags[i].c_str());
    }
  }

  commandLine.push_back(paraView);

  for (unsigned int i = 0; i < this->MPIPostFlags.size(); ++i)
  {
    commandLine.push_back(this->MPIPostFlags[i].c_str());
  }

  if (vtkProcessModule::GetProcessModule()->GetOptions()->GetConnectID() != 0)
  {
    std::ostringstream stream;
    stream << "--connect-id=" << vtkProcessModule::GetProcessModule()->GetOptions()->GetConnectID();
    commandLine.push_back(stream.str());
  }

  std::ostringstream stream;
  stream << "--server-port=" << port;
  commandLine.push_back(stream.str());
}

//--------------------------------------------------------------------internal
void vtkProcessModuleAutoMPIInternals::SeparateArguments(
  const char* str, std::vector<std::string>& flags)
{
  std::string arg = str;
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = arg.find_first_of(" ;");
  if (pos2 == arg.npos)
  {
    flags.push_back(str);
    return;
  }
  while (pos2 != arg.npos)
  {
    flags.push_back(arg.substr(pos1, pos2 - pos1));
    pos1 = pos2 + 1;
    pos2 = arg.find_first_of(" ;", pos1 + 1);
  }
  flags.push_back(arg.substr(pos1, pos2 - pos1));
}

//--------------------------------------------------------------------internal
void vtkProcessModuleAutoMPIInternals::PrintLine(const char* pname, const char* line)
{
  // if the name changed then the line is output from a different process
  if (this->CurrentPrintLineName != pname)
  {
    cerr << "-------------- " << pname << " output --------------\n";
    // save the current pname
    this->CurrentPrintLineName = pname;
  }
  cerr << line << "\n";
  cerr.flush();
}

//--------------------------------------------------------------------internal
void vtkProcessModuleAutoMPIInternals::ReportCommand(const char* const* command, const char* name)
{
  cerr << "AutoMPI: " << name << " command is:\n";
  for (const char* const* c = command; *c; ++c)
  {
    cerr << " \"" << *c << "\"";
  }
  cerr << "\n";
}

//---------------------------------------------------------------------internal
/*
 * Helper module to used to start a remote server. Code borrowed form
 * AutoMPI.cxx
 *
 * @param server IN The vtksysProcess instance which has info of the server.
 * @param name IN Name of the server.
 * @param out OUT Piped Stdout and Stderror used to find the status of server.
 * @param err OUT Piped stderr to find the status of the server
 * @return 0 = failure : 1 = success
 */
int vtkProcessModuleAutoMPIInternals::StartServer(
  vtksysProcess* server, const char* name, std::vector<char>& out, std::vector<char>& err)
{
  if (!server)
  {
    return 0;
  }

  cerr << "AutoMPI: starting process " << name << "\n";
  vtksysProcess_Execute(server);

  int foundWaiting = 0;
  std::string output;
  while (!foundWaiting)
  {
    // We wait for 10s at most to get the "Waiting" text printed out by
    // pvserver.
    double timeout = 10.0;
#ifdef _WIN32
    timeout = 20.0; // windows can be slow with MPI process launch.
#endif

    int pipe = this->WaitForAndPrintLine(name, server, output, timeout, out, err, &foundWaiting);
    if (pipe == vtksysProcess_Pipe_None || pipe == vtksysProcess_Pipe_Timeout)
    {
      break;
    }
  }
  if (foundWaiting)
  {
    cerr << "AutoMPI: " << name << " successfully started.\n";
    return 1;
  }
  else
  {
    cerr << "AutoMPI: " << name << " never started.\n";
    vtksysProcess_Kill(server);
    return 0;
  }
}

//--------------------------------------------------------------------internal
int vtkProcessModuleAutoMPIInternals::WaitForAndPrintLine(const char* pname, vtksysProcess* process,
  std::string& line, double timeout, std::vector<char>& out, std::vector<char>& err,
  int* foundWaiting)
{
  int pipe = this->WaitForLine(process, line, timeout, out, err);
  if (pipe == vtksysProcess_Pipe_STDOUT || pipe == vtksysProcess_Pipe_STDERR)
  {
    this->PrintLine(pname, line.c_str());
    if (foundWaiting && (line.find("Waiting") != line.npos))
    {
      *foundWaiting = 1;
    }
  }
  return pipe;
}

//---------------------------------------------------------------------internal
int vtkProcessModuleAutoMPIInternals::WaitForLine(vtksysProcess* process, std::string& line,
  double timeout, std::vector<char>& out, std::vector<char>& err)
{
  line = "";
  std::vector<char>::iterator outiter = out.begin();
  std::vector<char>::iterator erriter = err.begin();
  while (1)
  {
    // Check for a newline in stdout.
    for (; outiter != out.end(); ++outiter)
    {
      if ((*outiter == '\r') && ((outiter + 1) == out.end()))
      {
        break;
      }
      else if (*outiter == '\n' || *outiter == '\0')
      {
        int length = outiter - out.begin();
        if (length > 1 && *(outiter - 1) == '\r')
        {
          --length;
        }
        if (length > 0)
        {
          line.append(&out[0], length);
        }
        out.erase(out.begin(), outiter + 1);
        return vtksysProcess_Pipe_STDOUT;
      }
    }

    // Check for a newline in stderr.
    for (; erriter != err.end(); ++erriter)
    {
      if ((*erriter == '\r') && ((erriter + 1) == err.end()))
      {
        break;
      }
      else if (*erriter == '\n' || *erriter == '\0')
      {
        int length = erriter - err.begin();
        if (length > 1 && *(erriter - 1) == '\r')
        {
          --length;
        }
        if (length > 0)
        {
          line.append(&err[0], length);
        }
        err.erase(err.begin(), erriter + 1);
        return vtksysProcess_Pipe_STDERR;
      }
    }

    // No newlines found.  Wait for more data from the process.
    int length;
    char* data;
    int pipe = vtksysProcess_WaitForData(process, &data, &length, &timeout);
    if (pipe == vtksysProcess_Pipe_Timeout)
    {
      // Timeout has been exceeded.
      return pipe;
    }
    else if (pipe == vtksysProcess_Pipe_STDOUT)
    {
      // Append to the stdout buffer.
      std::vector<char>::size_type size = out.size();
      out.insert(out.end(), data, data + length);
      outiter = out.begin() + size;
    }
    else if (pipe == vtksysProcess_Pipe_STDERR)
    {
      // Append to the stderr buffer.
      std::vector<char>::size_type size = err.size();
      err.insert(err.end(), data, data + length);
      erriter = err.begin() + size;
    }
    else if (pipe == vtksysProcess_Pipe_None)
    {
      // Both stdout and stderr pipes have broken.  Return leftover data.
      if (!out.empty())
      {
        line.append(&out[0], outiter - out.begin());
        out.erase(out.begin(), out.end());
        return vtksysProcess_Pipe_STDOUT;
      }
      else if (!err.empty())
      {
        line.append(&err[0], erriter - err.begin());
        err.erase(err.begin(), err.end());
        return vtksysProcess_Pipe_STDERR;
      }
      else
      {
        return vtksysProcess_Pipe_None;
      }
    }
  }
}

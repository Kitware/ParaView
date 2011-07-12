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
#include "vtkPVOptions.h"
#include "vtkMultiThreader.h"
#include "vtkSocket.h"
#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>
#include "vtkstd/string"
#include "vtkstd/vector"
#include "vtksys/Process.h"
#include "vtksys/stl/string"
#include "vtksys/stl/vector"
#include "vtkSocket.h"

int vtkProcessModuleAutoMPI::UseMulticoreProcessors = 0;

namespace
{
  class vtkGetFreePort: public vtkSocket
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

    if (this->BindSocket(this->SocketDescriptor,0))
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
  vtkGetFreePort(){}
  ~vtkGetFreePort() {}
  };
  vtkStandardNewMacro(vtkGetFreePort);

  void vtkCopy(vtksys_stl::vector<const char*>& dest,
    const vtksys_stl::vector<vtkstd::string>& src)
    {
    dest.resize(src.size());
    for (size_t cc=0; cc < src.size(); cc++)
      {
      dest[cc] = src[cc].c_str();
      }
    dest.push_back(NULL);
    }
}

class vtkProcessModuleAutoMPIInternals
{
public:
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
  void CreateCommandLine (vtksys_stl::vector<vtkstd::string>& commandLine,
                          const char* paraView,
                          const char* numProc,
                          int port);
  bool CollectConfiguredOptions ();
  bool SetMPIRun(vtkstd::string mpiexec);
};

#ifdef WIN32
#  define PARAVIEW_SERVER "pvserver.exe"
#else
#  define PARAVIEW_SERVER "pvserver"
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
  this->Internals->TimeOut = 300;
}

//------------------------------------------------------------------------destr
vtkProcessModuleAutoMPI::~vtkProcessModuleAutoMPI()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------static
void vtkProcessModuleAutoMPI::SetUseMulticoreProcessors(int val)
{
  vtkProcessModuleAutoMPI::UseMulticoreProcessors = val;
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
   this->Internals->TotalMulticoreProcessors =
     vtkMultiThreader::GetGlobalDefaultNumberOfThreads();

#ifdef VTK_USE_MPI
   if( this->Internals->TotalMulticoreProcessors >1
       && vtkProcessModuleAutoMPI::UseMulticoreProcessors
     && this->Internals->CollectConfiguredOptions())
    {
    return 1;
    }
  else
    {
    return 0;
    }
#else
  return 0;
#endif //VTK_USE_MPI
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
  freePort->Delete();

  this->Internals->StartRemoteBuiltInSelf("localhost",port);
  return port;
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
int vtkProcessModuleAutoMPIInternals::
  StartRemoteBuiltInSelf(const char* vtkNotUsed(servername),int port)
{
  // Create a new server process structure
  vtksysProcess* server =0;
  server =vtksysProcess_New();
  if(!server)
    {
    vtksysProcess_Delete(server);
    cerr << "pvTestDriver: Cannot allocate vtksysProcess to run the server.\n";
    return 0;
    }

  if(server)
    {
    // Construct the Command line that will be executed
    vtksys_stl::vector<vtkstd::string> serverCommandStr;
    vtksys_stl::vector<const char*> serverCommand;
    //vtkstd::string serverExe = this->ParaViewServer;

    vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
    vtkstd::string app_dir =
      vtksys::SystemTools::GetProgramPath(options->GetApplicationPath());

#if defined(__APPLE__)
    vtkstd::string pvserver_rel(app_dir + "/../bin/" + vtkstd::string(PARAVIEW_SERVER));
    vtkstd::string serverExe = vtksys::SystemTools::CollapseFullPath(pvserver_rel.c_str());
    vtksysProcess_SetWorkingDirectory(server, app_dir.c_str());
    cerr << "Mac AppDir: " << serverExe << endl;
    cerr << "Mac ServerExe: " << serverExe << endl;
#elif defined(WIN32)
    vtkstd::string serverExe = PARAVIEW_SERVER;

    // Set the working directory as the location of pvserver. Some
    // mpi packages have issue with full paths containing spaces so
    // lets just invoke mpiexec  with no path to pvserver.
    cerr << "Setting working directory to be " << app_dir.c_str() << endl;
    vtksysProcess_SetWorkingDirectory(server, app_dir.c_str());
#else
    vtkstd::string serverExe = 
      app_dir + vtkstd::string("/") + vtkstd::string(PARAVIEW_SERVER);
#endif

    this->CreateCommandLine(serverCommandStr,
                      serverExe.c_str(),
                      this->MPIServerNumProcessFlag.c_str(),
                      port);
    vtkCopy(serverCommand, serverCommandStr);

    
    if(vtksysProcess_SetCommand(server, &serverCommand[0]))
      {
      this->ReportCommand(&serverCommand[0], "SUCCESS:");
      }
    else
      {
      this->ReportCommand(&serverCommand[0], "ERROR:");
      }
    }

  vtkstd::vector<char> ServerStdOut;
  vtkstd::vector<char> ServerStdErr;

 // Start the data server if there is one
  if(!this->StartServer(server, "server",
                        ServerStdOut, ServerStdErr))
    {
    cerr << "vtkProcessModuleAutoMPIInternals: Server never started.\n";
    vtksysProcess_Delete(server);;
    return 0;
    }
  return 1;
}

//---------------------------------------------------------------------internal
/*
 * Used to set the this->Internals->MPIRun variable to the appropriate MPI path
 *
 */
bool vtkProcessModuleAutoMPIInternals::SetMPIRun(vtkstd::string mpiexec)
{
  mpiexec = vtksys::SystemTools::GetFilenameName(mpiexec);
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  vtkstd::string app_dir = options->GetApplicationPath();
  app_dir = vtksys::SystemTools::GetProgramPath(app_dir.c_str())+"/"+mpiexec;
  if(vtksys::SystemTools::FileExists(app_dir.c_str(),true))
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
// set the path to the binary directory
  this->ParaViewServer = PARAVIEW_BINARY_DIR;
#ifdef  CMAKE_INTDIR
  this->ParaViewServer  += "/" CMAKE_INTDIR;
#endif
  // now set the final execuable names
  this->ParaViewServer += "/" PARAVIEW_SERVER;

  // now find all the mpi information if mpi run is set
#ifdef VTK_USE_MPI
#ifdef VTK_MPIRUN_EXE
  if(!this->SetMPIRun(VTK_MPIRUN_EXE))
    {
    this->MPIRun = VTK_MPIRUN_EXE;
    }
#else
    cerr << "AutoMPI Error: "
         << "VTK_MPIRUN_EXE must be set when VTK_USE_MPI is on."
         << endl;
    return 0;
#endif
  if(this->TotalMulticoreProcessors >1)
    {
    int serverNumProc = this->TotalMulticoreProcessors;

# ifdef VTK_MPI_NUMPROC_FLAG
    this->MPINumProcessFlag = VTK_MPI_NUMPROC_FLAG;
# else
    cerr << "Error VTK_MPI_NUMPROC_FLAG must be defined to run test if MPI is on.\n";
    return;
# endif
#ifdef VTK_MPI_PRENUMPROC_FLAGS
    this->SeparateArguments(VTK_MPI_PRENUMPROC_FLAGS, this->MPIPreNumProcFlags);
#endif
# ifdef VTK_MPI_PREFLAGS
    this->SeparateArguments(VTK_MPI_PREFLAGS, this->MPIPreFlags);
# endif
# ifdef VTK_MPI_POSTFLAGS
    this->SeparateArguments(VTK_MPI_POSTFLAGS, this->MPIPostFlags);
# endif
    char buf[1024];
    sprintf(buf, "%d", serverNumProc);
    this->MPIServerNumProcessFlag = buf;
    }
#endif // VTK_USE_MPI

# ifdef VTK_MPI_SERVER_PREFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_PREFLAGS, this->MPIServerPreFlags);
# endif
# ifdef VTK_MPI_SERVER_POSTFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_POSTFLAGS, this->MPIServerPostFlags);
# endif
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
void
vtkProcessModuleAutoMPIInternals::CreateCommandLine(
  vtksys_stl::vector<vtkstd::string>& commandLine,
  const char* paraView,
  const char* numProc,
  int port)
{
  if(this->MPIRun.size())
    {
    commandLine.push_back(this->MPIRun.c_str());
    for (unsigned int i = 0; i < this->MPIPreNumProcFlags.size(); ++i)
      {
      commandLine.push_back(this->MPIPreNumProcFlags[i].c_str());
      }

    commandLine.push_back(this->MPINumProcessFlag.c_str());
    commandLine.push_back(numProc);
    for(unsigned int i = 0; i < this->MPIPreFlags.size(); ++i)
      {
      commandLine.push_back(this->MPIPreFlags[i].c_str());
      }
    for(unsigned int i = 0; i < this->MPIServerPreFlags.size(); ++i)
      {
      commandLine.push_back(this->MPIServerPreFlags[i].c_str());
      }

    }
  char temp[100];
  vtkstd::string portString;
  sprintf(temp,"--server-port=%d",port);
  portString+=temp;
  portString+='\0';
  commandLine.push_back(paraView);
  commandLine.push_back(portString.c_str());

  for(unsigned int i = 0; i < this->MPIPostFlags.size(); ++i)
    {
    commandLine.push_back(this->MPIPostFlags[i].c_str());
    }
  // If there is specific flags for the server to pass to mpirun, add them
  for(unsigned int i = 0; i < this->MPIServerPostFlags.size(); ++i)
    {
    commandLine.push_back(this->MPIServerPostFlags[i].c_str());
    }
}

//--------------------------------------------------------------------internal
void vtkProcessModuleAutoMPIInternals::SeparateArguments(const char* str,
                                     vtkstd::vector<vtkstd::string>& flags)
{
  vtkstd::string arg = str;
  vtkstd::string::size_type pos1 = 0;
  vtkstd::string::size_type pos2 = arg.find_first_of(" ;");
  if(pos2 == arg.npos)
    {
    flags.push_back(str);
    return;
    }
  while(pos2 != arg.npos)
    {
    flags.push_back(arg.substr(pos1, pos2-pos1));
    pos1 = pos2+1;
    pos2 = arg.find_first_of(" ;", pos1+1);
    }
  flags.push_back(arg.substr(pos1, pos2-pos1));
}

//--------------------------------------------------------------------internal
void vtkProcessModuleAutoMPIInternals::PrintLine(const char* pname, const char* line)
{
  // if the name changed then the line is output from a different process
  if(this->CurrentPrintLineName != pname)
    {
    cerr << "-------------- " << pname
         << " output --------------\n";
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
  for(const char* const * c = command; *c; ++c)
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
int vtkProcessModuleAutoMPIInternals::StartServer(vtksysProcess* server, const char* name,
                              vtkstd::vector<char>& out,
                              vtkstd::vector<char>& err)
{
  if(!server)
    {
    return 1;
    }
  cerr << "AutoMPI: starting process " << name << "\n";
  vtksysProcess_SetTimeout(server, this->TimeOut);
  vtksysProcess_Execute(server);
  int foundWaiting = 0;
  vtkstd::string output;
  while(!foundWaiting)
    {
    int pipe = this->WaitForAndPrintLine(name, server, output, 100.0, out, err,
                                         &foundWaiting);
    if(pipe == vtksysProcess_Pipe_None ||
       pipe == vtksysProcess_Pipe_Timeout)
      {
      break;
      }
    }
  if(foundWaiting)
    {
    cerr << "AutoMPI: " << name << " sucessfully started.\n";
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
                                      vtkstd::string& line, double timeout,
                                      vtkstd::vector<char>& out,
                                      vtkstd::vector<char>& err,
                                      int* foundWaiting)
{
  int pipe = this->WaitForLine(process, line, timeout, out, err);
  if(pipe == vtksysProcess_Pipe_STDOUT || pipe == vtksysProcess_Pipe_STDERR)
    {
    this->PrintLine(pname, line.c_str());
    if(foundWaiting && (line.find("Waiting") != line.npos))
      {
      *foundWaiting = 1;
      }
    }
  return pipe;
}

//---------------------------------------------------------------------internal
int vtkProcessModuleAutoMPIInternals::WaitForLine(vtksysProcess* process, vtkstd::string& line,
                              double timeout,
                              vtkstd::vector<char>& out,
                              vtkstd::vector<char>& err)
{
  line = "";
  vtkstd::vector<char>::iterator outiter = out.begin();
  vtkstd::vector<char>::iterator erriter = err.begin();
  while(1)
    {
    // Check for a newline in stdout.
    for(;outiter != out.end(); ++outiter)
      {
      if((*outiter == '\r') && ((outiter+1) == out.end()))
        {
        break;
        }
      else if(*outiter == '\n' || *outiter == '\0')
        {
        int length = outiter-out.begin();
        if(length > 1 && *(outiter-1) == '\r')
          {
          --length;
          }
        if(length > 0)
          {
          line.append(&out[0], length);
          }
        out.erase(out.begin(), outiter+1);
        return vtksysProcess_Pipe_STDOUT;
        }
      }

    // Check for a newline in stderr.
    for(;erriter != err.end(); ++erriter)
      {
      if((*erriter == '\r') && ((erriter+1) == err.end()))
        {
        break;
        }
      else if(*erriter == '\n' || *erriter == '\0')
        {
        int length = erriter-err.begin();
        if(length > 1 && *(erriter-1) == '\r')
          {
          --length;
          }
        if(length > 0)
          {
          line.append(&err[0], length);
          }
        err.erase(err.begin(), erriter+1);
        return vtksysProcess_Pipe_STDERR;
        }
      }

    // No newlines found.  Wait for more data from the process.
    int length;
    char* data;
    int pipe = vtksysProcess_WaitForData(process, &data, &length, &timeout);
    if(pipe == vtksysProcess_Pipe_Timeout)
      {
      // Timeout has been exceeded.
      return pipe;
      }
    else if(pipe == vtksysProcess_Pipe_STDOUT)
      {
      // Append to the stdout buffer.
      vtkstd::vector<char>::size_type size = out.size();
      out.insert(out.end(), data, data+length);
      outiter = out.begin()+size;
      }
    else if(pipe == vtksysProcess_Pipe_STDERR)
      {
      // Append to the stderr buffer.
      vtkstd::vector<char>::size_type size = err.size();
      err.insert(err.end(), data, data+length);
      erriter = err.begin()+size;
      }
    else if(pipe == vtksysProcess_Pipe_None)
      {
      // Both stdout and stderr pipes have broken.  Return leftover data.
      if(!out.empty())
        {
        line.append(&out[0], outiter-out.begin());
        out.erase(out.begin(), out.end());
        return vtksysProcess_Pipe_STDOUT;
        }
      else if(!err.empty())
        {
        line.append(&err[0], erriter-err.begin());
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

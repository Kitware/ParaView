/*=========================================================================

  Module:    vtkKWRemoteExecute.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRemoteExecute.h"

#include "vtkObjectFactory.h"
#include "vtkMultiThreader.h"

#ifdef _MSC_VER
#pragma warning (push, 1)
#pragma warning (disable: 4702)
#endif

#include <vector>
#include <map>
#include <string>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _WIN32
#else
# include <sys/types.h>
# include <unistd.h>
#endif

//----------------------------------------------------------------------------
//============================================================================
class vtkKWRemoteExecuteInternal
{
public:
  vtkKWRemoteExecuteInternal()
    {
    }
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  VectorOfStrings Args;
  vtkstd::string Command;
};
//============================================================================
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWRemoteExecute );

//----------------------------------------------------------------------------
vtkKWRemoteExecute::vtkKWRemoteExecute()
{
  this->Internals = new vtkKWRemoteExecuteInternal;
  this->RemoteHost = 0;
  this->Result = NOT_RUN;

  this->SSHUser = 0;
  this->SSHCommand = 0;
  this->SSHArguments = 0;

  this->SetSSHCommand("ssh");

  this->MultiThreader = vtkMultiThreader::New();
  this->ProcessThreadId = -1;
}

//----------------------------------------------------------------------------
vtkKWRemoteExecute::~vtkKWRemoteExecute()
{
  delete this->Internals;
  this->SetRemoteHost(0);

  this->SetSSHUser(0);
  this->SetSSHCommand(0);
  this->SetSSHArguments(0);

  this->MultiThreader->Delete();
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::Detach()
{
  int res = VTK_ERROR;
  cout << "Detaching ParaView" << endl;
#ifdef _WIN32
  // No code for detaching yet.
  vtkGenericWarningMacro("Cannot detach on windows yet");
#elif __linux__
  if ( daemon(0,0) == -1 )
    {
    vtkGenericWarningMacro("Problem detaching ParaView");
    return VTK_ERROR;
    }
  res = VTK_OK;
#else
  vtkGenericWarningMacro("Cannot detach on this system yet");
#endif
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::RunRemoteCommand(const char* args)
{
  if ( !this->RemoteHost )
    {
    vtkErrorMacro("Remote host not set");
    return 0;
    }

  if ( !this->SSHCommand )
    {
    vtkErrorMacro("SSH Command not set");
    return 0;
    }
  if ( args )
    {
    this->Internals->Command = args;
    }
  cout << "This is: " << this << endl;
  vtkMultiThreader* th = this->MultiThreader;
  this->ProcessThreadId = th->SpawnThread(
    (vtkThreadFunctionType)(vtkKWRemoteExecute::RunCommandThread), this);
  this->Result = vtkKWRemoteExecute::RUNNING;
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::WaitToFinish()
{
  if ( this->ProcessThreadId < 0 )
    {
    cout << "No process running" << endl;
    return 0;
    }
  int res = 0;
  vtkMultiThreader* th = this->MultiThreader;
  th->TerminateThread(this->ProcessThreadId);
  if ( this->Result == vtkKWRemoteExecute::SUCCESS )
    {
    res = 1;
    }
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::RunCommand(const char* args)
{
  cout << "Execute [" << args << "]" << endl;
  system(args);
  return VTK_OK;
}

//----------------------------------------------------------------------------
void* vtkKWRemoteExecute::RunCommandThread(void* vargs)
{
  vtkMultiThreader::ThreadInfo *ti = static_cast<vtkMultiThreader::ThreadInfo*>(vargs);
  vtkKWRemoteExecute* self = static_cast<vtkKWRemoteExecute*>(ti->UserData);
  if ( !self )
    {
    cout << "Have no pointer to self" << endl;
    return 0;
    }

  cout << "self is " << self << endl;

  vtkstd::string command = "";
  command +=  self->SSHCommand;
  command += " ";
  if ( self->SSHArguments )
    {
    command += self->SSHArguments;
    command += " ";
    }
  if ( self->SSHUser )
    {
    command += "-l ";
    command += self->SSHUser;
    command += " ";
    }

  command += self->RemoteHost;
  command += " ";

  command += "'" + self->Internals->Command + "'";

  int res = self->RunCommand(command.c_str());
  if ( res == VTK_OK )
    {
    self->Result = vtkKWRemoteExecute::SUCCESS;
    }
  else
    {
    self->Result = vtkKWRemoteExecute::FAIL;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRemoteExecute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteHost: " << ( this->RemoteHost ?
                                      this->RemoteHost : "(none)" ) << endl;
  os << indent << "SSHCommand: " << ( this->SSHCommand ? 
                                      this->SSHCommand : "(none)" ) << endl;
  os << indent << "Result: " << this->Result << endl;
  os << indent << "SSHArguments: " << ( this->SSHArguments ? 
                                        this->SSHArguments : "(none)" ) << endl;
  os << indent << "SSHUser: " << ( this->SSHUser ? 
                                   this->SSHUser : "(none)" ) << endl;
}




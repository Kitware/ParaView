/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRemoteExecute.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWRemoteExecute.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
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
vtkCxxRevisionMacro(vtkKWRemoteExecute, "1.13");

//----------------------------------------------------------------------------
vtkKWRemoteExecute::vtkKWRemoteExecute()
{
  this->Internals = new vtkKWRemoteExecuteInternal;
  this->RemoteHost = 0;
  this->ProcessRunning = 0;
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
  this->ProcessRunning = 1;
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
  if ( self ) 
    {
    self->ProcessRunning = 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRemoteExecute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteHost: " << this->RemoteHost << endl;
  os << indent << "SSHCommand: " << (this->SSHCommand?this->SSHCommand:"(none)") << endl;
  os << indent << "Result: " << this->Result << endl;
  os << indent << "SSHArguments: " << (this->SSHArguments?this->SSHArguments:"(none)") << endl;
  os << indent << "SSHUser: " << (this->SSHUser?this->SSHUser:"(none)") << endl;
}

/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModule.cxx
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
#include "vtkPVProcessModule.h"

#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFloatArray.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVWindow.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkStringList.h"
#include "vtkTclUtil.h"
#include "vtkToolkits.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"

#include <vtkstd/string>

int vtkStringListCommand(ClientData cd, Tcl_Interp *interp,
                         int argc, char *argv[]);

struct vtkPVArgs
{
  int argc;
  char **argv;
  int* RetVal;
};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProcessModule);
vtkCxxRevisionMacro(vtkPVProcessModule, "1.24.2.7");

int vtkPVProcessModuleCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVProcessModule::vtkPVProcessModule()
{
  this->UniqueID.ID = 3;
  this->Controller = NULL;
  this->TemporaryInformation = NULL;
  this->RootResult = NULL;
  this->ClientServerStream = 0;
  this->ClientInterpreter = 0;
}

//----------------------------------------------------------------------------
vtkPVProcessModule::~vtkPVProcessModule()
{
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
  this->SetRootResult(NULL);
  if(this->ClientInterpreter)
    {
    this->ClientInterpreter->Delete();
    }
  delete this->ClientServerStream;
}

// Declare the initialization function as external
// this is defined in the PackageInit file
extern void Vtkparaviewcswrapped_Initialize(vtkClientServerInterpreter *arlu);


//----------------------------------------------------------------------------
int vtkPVProcessModule::Start(int argc, char **argv)
{
  if (this->Controller == NULL)
    {
    this->Controller = vtkDummyController::New();
    vtkMultiProcessController::SetGlobalController(this->Controller);
    }

  vtkPVApplication *app = this->GetPVApplication();
  // For SGI pipes option.
  app->SetNumberOfPipes(1);

#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
  app->SetupTrapsForSignals(myId);
#endif // PV_HAVE_TRAPS_FOR_SIGNALS
  app->SetProcessModule(this);
  app->Script("wm withdraw .");

  this->InitializeTclMethodImplementations();


  this->ClientServerStream = new vtkClientServerStream;
  this->ClientInterpreter = vtkClientServerInterpreter::New();
  this->ClientInterpreter->SetLogFile("c:/pvClient.out");
  Vtkparaviewcswrapped_Initialize(this->ClientInterpreter);
  this->GetStream()
    << vtkClientServerStream::Assign
    << this->GetApplicationID() << app
    << vtkClientServerStream::End;
  this->ClientInterpreter->ProcessStream(this->GetStream());

  app->Start(argc,argv);


  return app->GetExitStatus();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::Exit()
{
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVProcessModule::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->Application);
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetPartitionId()
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::BroadcastScript(const char* format, ...)
{
  char event[1600];
  char* buffer = event;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  va_list ap;
  va_start(ap, format);
  int length = this->Application->EstimateFormatLength(format, ap);
  va_end(ap);

  if(length > 1599)
    {
    buffer = new char[length+1];
    }

  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);

  this->BroadcastSimpleScript(buffer);

  if(buffer != event)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::ServerScript(const char* format, ...)
{
  char event[1600];
  char* buffer = event;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  va_list ap;
  va_start(ap, format);
  int length = this->Application->EstimateFormatLength(format, ap);
  va_end(ap);

  if(length > 1599)
    {
    buffer = new char[length+1];
    }

  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);

  this->ServerSimpleScript(buffer);

  if(buffer != event)
    {
    delete [] buffer;
    }
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::BroadcastSimpleScript(const char *str)
{
  this->Application->SimpleScript(str);
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::ServerSimpleScript(const char *str)
{
  // Do this so that only the client server process module
  // needs to implement this method.
  this->BroadcastSimpleScript(str);
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::RemoteScript(int id, const char* format, ...)
{
  char event[1600];
  char* buffer = event;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  va_list ap;
  va_start(ap, format);
  int length = this->EstimateFormatLength(format, ap);
  va_end(ap);

  if(length > 1599)
    {
    buffer = new char[length+1];
    }

  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);

  this->RemoteSimpleScript(id, buffer);

  if(buffer != event)
    {
    delete [] buffer;
    }
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::RemoteSimpleScript(int remoteId, const char *str)
{
  // send string to evaluate.
  if (vtkString::Length(str) <= 0)
    {
    return;
    }

  if (remoteId == 0)
    {
    this->Application->SimpleScript(str);
    }
  }


//----------------------------------------------------------------------------
int vtkPVProcessModule::GetNumberOfPartitions()
{
  return 1;
}




//----------------------------------------------------------------------------
void vtkPVProcessModule::GatherInformation(vtkPVInformation* info,
                                           char* objectTclName)
{
  // Just a simple way of passing the information object to the next
  // method.
  this->TemporaryInformation = info;
  // Some objects are not created on the client (data.
  if (!info->GetRootOnly())
    {
    this->ServerScript(
      "[$Application GetProcessModule] GatherInformationInternal %s %s",
      info->GetClassName(), objectTclName);
    }
  else
    {
    this->RootScript(
      "[$Application GetProcessModule] GatherInformationInternal %s %s",
      info->GetClassName(), objectTclName);
    }
  this->TemporaryInformation = NULL;
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::GatherInformationInternal(char*, vtkObject* object)
{
  // This class is used only for one processes.
  if (this->TemporaryInformation == NULL)
    {
    vtkErrorMacro("Information argument not set.");
    return;
    }
  if (object == NULL)
    {
    vtkErrorMacro("Object tcl name must be wrong.");
    return;
    }

  this->TemporaryInformation->CopyFromObject(object);
 }


//----------------------------------------------------------------------------
void vtkPVProcessModule::RootScript(const char* format, ...)
{
  char event[1600];
  char* buffer = event;

  va_list ap;
  va_start(ap, format);
  int length = this->EstimateFormatLength(format, ap);
  va_end(ap);

  if(length > 1599)
    {
    buffer = new char[length+1];
    }

  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);

  this->RootSimpleScript(buffer);

  if(buffer != event)
    {
    delete [] buffer;
    }
}



//----------------------------------------------------------------------------
void vtkPVProcessModule::RootSimpleScript(const char *script)
{
  // Default implementation just executes locally.
  this->Script(script);
  this->SetRootResult(this->Application->GetMainInterp()->result);
}

//----------------------------------------------------------------------------
const char* vtkPVProcessModule::GetRootResult()
{
  return this->RootResult;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SetApplication(vtkKWApplication* arg)
{
  this->Superclass::SetApplication(arg);
  this->InitializeTclMethodImplementations();
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetDirectoryListing(const char* dir,
                                            vtkStringList* dirs,
                                            vtkStringList* files)
{
  return this->GetDirectoryListing(dir, dirs, files, "readable");
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetDirectoryListing(const char* dir,
                                            vtkStringList* dirs,
                                            vtkStringList* files,
                                            const char* perm)
{
  char* result = vtkString::Duplicate(this->Application->Script(
    "::paraview::vtkPVProcessModule::GetDirectoryListing {%s} {%s}",
    dir, perm));
  if(strcmp(result, "<NO_SUCH_DIRECTORY>") == 0)
    {
    dirs->RemoveAllItems();
    files->RemoveAllItems();
    delete [] result;
    return 0;
    }
  vtkTclGetObjectFromPointer(this->Application->GetMainInterp(), dirs,
                             vtkStringListCommand);
  char* dirsTcl = vtkString::Duplicate(
    Tcl_GetStringResult(this->Application->GetMainInterp()));
  vtkTclGetObjectFromPointer(this->Application->GetMainInterp(), files,
                             vtkStringListCommand);
  char* filesTcl = vtkString::Duplicate(
    Tcl_GetStringResult(this->Application->GetMainInterp()));
  this->Application->Script(
    "::paraview::vtkPVProcessModule::ParseDirectoryListing {%s} {%s} {%s}",
    result, dirsTcl, filesTcl
    );
  delete [] dirsTcl;
  delete [] filesTcl;
  delete [] result;
  return 1;
}

//----------------------------------------------------------------------------
vtkKWLoadSaveDialog* vtkPVProcessModule::NewLoadSaveDialog()
{
  vtkKWLoadSaveDialog* dialog = vtkKWLoadSaveDialog::New();
  return dialog;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InitializeTclMethodImplementations()
{
  if ( !this->Application )
    {
    return;
    }
  this->Application->Script(
    "namespace eval ::paraview::vtkPVProcessModule {\n"
    "  proc GetDirectoryListing { dir perm {exp {[A-Za-z0-9]*}} } {\n"
    "    set files {}\n"
    "    set dirs {}\n"
    "    if {$dir == {<GET_DRIVE_LETTERS>}} {\n"
    "      foreach drive [file volumes] {\n"
    "        if {![catch {file stat $drive .}]} {\n"
    "          lappend dirs $drive\n"
    "        }\n"
    "      }\n"
    "      return [list $dirs $files]\n"
    "    }\n"
    "    if {$dir != {}} {\n"
    "      set cwd [pwd]\n"
    "      if {[catch {cd $dir}]} {\n"
    "        return {<NO_SUCH_DIRECTORY>}\n"
    "      }\n"
    "    }\n"
    "    set entries [glob -nocomplain $exp]\n"
    "    foreach f [lsort -dictionary $entries] {\n"
    "      if {[file isfile $f] && [file $perm $f]} {\n"
    "        lappend files $f\n"
    "      } elseif {[file isdirectory $f] && [file readable $f]} {\n"
    "        lappend dirs $f\n"
    "      }\n"
    "    }\n"
    "    if {$dir != {}} { cd $cwd }\n"
    "    return [list $dirs $files]\n"
    "  }\n"
    "  proc ParseDirectoryListing { lst dirs files } {\n"
    "    $dirs RemoveAllItems\n"
    "    $files RemoveAllItems\n"
    "    foreach f [lindex $lst 0] { $dirs AddString $f }\n"
    "    foreach f [lindex $lst 1] { $files AddString $f }\n"
    "  }\n"
    "}\n"
    );
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::ReceiveRootPolyData(const char* tclName,
                                            vtkPolyData* out)
{
  // Make sure we have a named Tcl VTK object.
  if(!tclName || !tclName[0])
    {
    return 0;
    }

  // We are the server.  Just return the object from the local
  // interpreter.
  vtkstd::string name = this->GetPVApplication()->EvaluateString(tclName);
  vtkObject* obj = this->GetPVApplication()->TclToVTKObject(name.c_str());
  vtkPolyData* dobj = vtkPolyData::SafeDownCast(obj);
  if(dobj)
    {
    out->DeepCopy(dobj);
    }
  else
    {
    return 0;
    }
  return 1;

}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToClient()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToServer()
{
  this->ClientInterpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToClientAndServer()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVProcessModule::NewStreamObject(const char* type)
{
  vtkClientServerStream& stream = this->GetStream();
  vtkClientServerID id = this->GetUniqueID();
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;
  return id;
}

vtkObjectBase* vtkPVProcessModule::GetObjectFromID(vtkClientServerID id)
{
  return this->ClientInterpreter->GetObjectFromID(id);
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::DeleteStreamObject(vtkClientServerID id)
{
  vtkClientServerStream& stream = this->GetStream();
  stream << vtkClientServerStream::Delete << id
         <<  vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVProcessModule::GetLastServerResult()
{
  return this->ClientInterpreter->GetLastResult();
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVProcessModule::GetLastClientResult()
{
  return this->GetLastServerResult();
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVProcessModule::GetUniqueID()
{
  this->UniqueID.ID++;
  return this->UniqueID;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVProcessModule::GetApplicationID()
{
  vtkClientServerID id = {1};
  return id;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
  if (this->RootResult)
    {
    os << indent << "RootResult: " << this->RootResult << endl;
    }
}

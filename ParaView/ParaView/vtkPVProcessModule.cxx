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
#include "vtkPVDataInformation.h"
#include "vtkPVPart.h"
#include "vtkPVWindow.h"
#include "vtkShortArray.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkStringList.h"
#include "vtkTclUtil.h"
#include "vtkToolkits.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

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
vtkCxxRevisionMacro(vtkPVProcessModule, "1.17");

int vtkPVProcessModuleCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVProcessModule::vtkPVProcessModule()
{
  this->Controller = NULL;
  this->TemporaryInformation = NULL; 
}

//----------------------------------------------------------------------------
vtkPVProcessModule::~vtkPVProcessModule()
{
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
}



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
void vtkPVProcessModule::BroadcastScript(char *format, ...)
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
void vtkPVProcessModule::ServerScript(char *format, ...)
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
void vtkPVProcessModule::RemoteScript(int id, char *format, ...)
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
void vtkPVProcessModule::GatherDataInformation(vtkPVDataInformation* info, 
                                               char* deciTclName)
{
  // This ivar is so temprary, we do not need to increase the reference count.
  this->TemporaryInformation = info;
  this->BroadcastScript("[$Application GetProcessModule] GatherDataInformation %s",
                        deciTclName); 
  this->TemporaryInformation = NULL; 
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::GatherDataInformation(vtkSource *deci)
{
  vtkDataObject** dataObjects;
  vtkSource* geo;
  vtkDataObject* geoData;
  vtkDataObject* deciData;
  vtkDataSet* data;

  if (deci == NULL)
    {
    vtkErrorMacro("Deci tcl name must be wrong.");
    return;
    }

  // Get the data object form the decimate filter.
  // This is a bit of a hack. Maybe we should have a PVPart object
  // on all processes.
  // Sanity checks to avoid slim chance of segfault.
  dataObjects = deci->GetOutputs(); 
  if (dataObjects == NULL || dataObjects[0] == NULL)
    {
    vtkErrorMacro("Could not get deci output.");
    return;
    }
  deciData = dataObjects[0];
  dataObjects = deci->GetInputs(); 
  if (dataObjects == NULL || dataObjects[0] == NULL)
    {
    vtkErrorMacro("Could not get deci input.");
    return;
    }
  geoData = dataObjects[0];
  geo = geoData->GetSource();
  if (geo == NULL)
    {
    vtkErrorMacro("Could not get geo.");
    return;
    }
  dataObjects = geo->GetInputs(); 
  if (dataObjects == NULL || dataObjects[0] == NULL)
    {
    vtkErrorMacro("Could not get geo input.");
    return;
    }
  data = vtkDataSet::SafeDownCast(dataObjects[0]);
  if (data == NULL)
    {
    vtkErrorMacro("It couldn't be a vtkDataObject???");
    return;
    }

  this->TemporaryInformation->CopyFromData(data);
  this->TemporaryInformation->SetGeometryMemorySize(geoData->GetActualMemorySize());
  this->TemporaryInformation->SetLODMemorySize(deciData->GetActualMemorySize());
 }




//----------------------------------------------------------------------------
void vtkPVProcessModule::InitializePVPartPartition(vtkPVPart *part)
{
  int numProcs = 1;

  if (getenv("PV_DEBUG_HALF") != NULL)
    {
    numProcs = 2;
    }
  this->Script("%s SetNumberOfPieces %d",
               part->GetMapperTclName(), numProcs);
  this->Script("%s SetPiece %d", part->GetMapperTclName(), 0);
  this->Script("%s SetUpdateNumberOfPieces %d",
               part->GetUpdateSuppressorTclName(), numProcs);
  this->Script("%s SetUpdatePiece %d", 
               part->GetUpdateSuppressorTclName(), 0);
  this->Script("%s SetNumberOfPieces %d",
               part->GetLODMapperTclName(), numProcs);
  this->Script("%s SetPiece %d", part->GetLODMapperTclName(), 0);
  this->Script("%s SetUpdateNumberOfPieces %d",
               part->GetLODUpdateSuppressorTclName(), numProcs);
  this->Script("%s SetUpdatePiece %d", 
               part->GetLODUpdateSuppressorTclName(), 0);
}



//----------------------------------------------------------------------------
void vtkPVProcessModule::RootScript(char *format, ...)
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
}

//----------------------------------------------------------------------------
const char* vtkPVProcessModule::GetRootResult()
{
  return this->Application->GetMainInterp()->result;
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
  dialog->SetParent(this->GetPVApplication()->GetMainWindow());
  return dialog;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InitializeTclMethodImplementations()
{
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
    "      } elseif {[file isdirectory $f] && [file $perm $f]} {\n"
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
vtkDataObject* vtkPVProcessModule::ReceiveRootDataObject(const char* tclName)
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
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(obj);
  if(dobj)
    {
    // This method must return a reference to the object.  The caller
    // will invoke Delete().
    dobj->Register(0);
    }
  return dobj;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;;
}

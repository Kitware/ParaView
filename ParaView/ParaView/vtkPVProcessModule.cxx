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
#include "vtkObjectFactory.h"

#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkMultiProcessController.h"
#include "vtkDummyController.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkMapper.h"
#include "vtkString.h"
#include "vtkPVPart.h"



struct vtkPVArgs
{
  int argc;
  char **argv;
  int* RetVal;
};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProcessModule);
vtkCxxRevisionMacro(vtkPVProcessModule, "1.5");

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
void vtkPVProcessModule::BroadcastSimpleScript(const char *str)
{
  this->Application->SimpleScript(str);
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
                                               char *dataTclName)
{
  // This ivar is so temprary, we do not need to increase the reference count.
  this->TemporaryInformation = info;
  this->BroadcastScript("[$Application GetProcessModule] GatherDataInformation %s",
                        dataTclName); 
  this->TemporaryInformation = NULL; 
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::GatherDataInformation(vtkDataSet *data)
{
  if (data == NULL)
    {
    vtkErrorMacro("Data Tcl name has not been set correctly.");
    return;
    }
  this->TemporaryInformation->CopyFromData(data);
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
char* vtkPVProcessModule::NewRootResult()
{
  char* str;
  char* result;

  result = this->Application->GetMainInterp()->result;
  str = new char[strlen(result)+1];
  strcpy(str, result);

  return str;
}





//----------------------------------------------------------------------------
void vtkPVProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;;
}

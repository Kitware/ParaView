/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProcessModule.h"

#include "vtkCallbackCommand.h"
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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProcessModule);
vtkCxxRevisionMacro(vtkPVProcessModule, "1.47");

vtkCxxSetObjectMacro(vtkPVProcessModule, Application, vtkKWApplication);

//----------------------------------------------------------------------------
vtkPVProcessModule::vtkPVProcessModule()
{
  this->Application = 0; 
  this->MPIMToNSocketConnectionID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVProcessModule::~vtkPVProcessModule()
{
  // Free Interpreter and ClientServerStream.
  this->FinalizeInterpreter();
  if (this->Application)
    {
    this->Application->Delete();
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

  if (app->GetStartGUI())
    {
    app->Script("wm withdraw .");
    app->Start(argc,argv);
    }
  else
    {
    app->Exit();
    }
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
int vtkPVProcessModule::GetNumberOfPartitions()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetDirectoryListing(const char* dir,
                                            vtkStringList* dirs,
                                            vtkStringList* files,
                                            int save)
{
  // Get the listing from the server.
  vtkClientServerID lid = this->NewStreamObject("vtkPVServerFileListing");
  this->GetStream() << vtkClientServerStream::Invoke
                    << lid << "GetFileListing" << dir << save
                    << vtkClientServerStream::End;
  this->SendStreamToServerRoot();
  vtkClientServerStream result;
  if(!this->GetLastServerResult().GetArgument(0, 0, &result))
    {
    vtkErrorMacro("Error getting file list result from server.");
    this->DeleteStreamObject(lid);
    this->SendStreamToServerRoot();
    return 0;
    }
  this->DeleteStreamObject(lid);
  this->SendStreamToServerRoot();

  // Parse the listing.
  if ( dirs )
    {
    dirs->RemoveAllItems();
    }
  if ( files )
    {
    files->RemoveAllItems();
    }
  if(result.GetNumberOfMessages() == 2)
    {
    int i;
    // The first message lists directories.
    if ( dirs )
      {
      for(i=0; i < result.GetNumberOfArguments(0); ++i)
        {
        const char* d;
        if(result.GetArgument(0, i, &d))
          {
          dirs->AddString(d);
          }
        else
          {
          vtkErrorMacro("Error getting directory name from listing.");
          }
        }
      }

    // The second message lists files.
    if ( files )
      {
      for(i=0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* f;
        if(result.GetArgument(1, i, &f))
          {
          files->AddString(f);
          }
        else
          {
          vtkErrorMacro("Error getting file name from listing.");
          }
        }
      }
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkKWLoadSaveDialog* vtkPVProcessModule::NewLoadSaveDialog()
{
  vtkKWLoadSaveDialog* dialog = vtkKWLoadSaveDialog::New();
  return dialog;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkPVProcessModule::GetObjectFromID(vtkClientServerID id)
{
  return this->Interpreter->GetObjectFromID(id);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkPVProcessModule::GetObjectFromIntID(unsigned int idin)
{
  vtkClientServerID id;
  id.ID = idin;
  return this->GetObjectFromID(id);
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
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InitializeInterpreter()
{
  if(this->Interpreter)
    {
    return;
    }

  this->Superclass::InitializeInterpreter();

  // Create the interpreter and supporting stream.
  // Assign standard IDs.
  vtkPVApplication *app = this->GetPVApplication();
  vtkClientServerStream css;
  css << vtkClientServerStream::Assign
      << this->GetApplicationID() << app
      << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::FinalizeInterpreter()
{
  if(!this->Interpreter)
    {
    return;
    }

  // Delete the standard IDs.
  vtkClientServerStream css;
  css << vtkClientServerStream::Delete
      << this->GetApplicationID()
      << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);

  this->Superclass::FinalizeInterpreter();
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::LoadModule(const char* name, const char* directory)
{
  this->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "LoadModuleInternal" << name << directory
    << vtkClientServerStream::End;
  this->SendStreamToServer();
  int result = 0;
  if(!this->GetLastServerResult().GetArgument(0, 0, &result))
    {
    vtkErrorMacro("LoadModule could not get result from server.");
    return 0;
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::LoadModuleInternal(const char* name,
                                           const char* directory)
{
  const char* paths[] = {directory, 0};
  return this->Interpreter->Load(name, paths);
}

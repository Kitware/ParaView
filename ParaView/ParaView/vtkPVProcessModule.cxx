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

#include <vtkstd/string>

vtkPVProcessModule* vtkPVProcessModule::ProcessModule = 0;

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
vtkCxxRevisionMacro(vtkPVProcessModule, "1.42");

int vtkPVProcessModuleCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVProcessModule::vtkPVProcessModule()
{
  this->UniqueID.ID = 3;
  this->Controller = NULL;
  this->TemporaryInformation = NULL;
  this->ClientServerStream = 0;
  this->Interpreter = 0;
  this->InterpreterObserver = 0;
  this->ReportInterpreterErrors = 1;
}

//----------------------------------------------------------------------------
vtkPVProcessModule::~vtkPVProcessModule()
{
  // Free Interpreter and ClientServerStream.
  this->FinalizeInterpreter();

  // Other cleanup.
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVProcessModule* vtkPVProcessModule::GetProcessModule()
{
  return vtkPVProcessModule::ProcessModule;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SetProcessModule(vtkPVProcessModule* pm)
{
  vtkPVProcessModule::ProcessModule = pm;
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

void vtkPVProcessModule::GatherInformationRenderServer(vtkPVInformation* ,
                                                       vtkClientServerID )
{
  vtkErrorMacro("This should only be called from the client of a client render server mode paraview");
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::GatherInformation(vtkPVInformation* info,
                                           vtkClientServerID id)
{
  // Just a simple way of passing the information object to the next
  // method.
  this->TemporaryInformation = info;
  this->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "GatherInformationInternal" << info->GetClassName() << id
    << vtkClientServerStream::End;
  this->SendStreamToClientAndServer();
  this->TemporaryInformation = NULL;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::GatherInformationInternal(const char*,
                                                   vtkObject* object)
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
void vtkPVProcessModule::SetApplication(vtkKWApplication* arg)
{
  this->Superclass::SetApplication(arg);
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
void vtkPVProcessModule::SendStreamToClient()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToClientAndServer()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToClientAndServerRoot()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToRenderServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToRenderServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToRenderServerAndServerRoot()
{ 
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToRenderServerAndServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToClientAndRenderServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToRenderServerClientAndServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendStreamToClientAndRenderServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
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
  return this->Interpreter->GetObjectFromID(id);
}

vtkObjectBase* vtkPVProcessModule::GetObjectFromIntID(unsigned int idin)
{
  vtkClientServerID id;
  id.ID = idin;
  return this->GetObjectFromID(id);
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
  if(this->Interpreter)
    {
    return this->Interpreter->GetLastResult();
    }
  else
    {
    static vtkClientServerStream emptyResult;
    return emptyResult;
    }
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVProcessModule::GetLastClientResult()
{
  return this->GetLastServerResult();
}

//----------------------------------------------------------------------------
vtkClientServerInterpreter* vtkPVProcessModule::GetInterpreter()
{
  return this->Interpreter;
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
vtkClientServerID vtkPVProcessModule::GetProcessModuleID()
{
  vtkClientServerID id = {2};
  return id;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "ReportInterpreterErrors: "
     << this->ReportInterpreterErrors << endl;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InitializeInterpreter()
{
  if(this->Interpreter)
    {
    return;
    }

  // Create the interpreter and supporting stream.
  this->Interpreter = vtkClientServerInterpreter::New();
  this->ClientServerStream = new vtkClientServerStream;
  this->Interpreter->SetLogFile("./cs.log");
  
  // Setup a callback for the interpreter to report errors.
  this->InterpreterObserver = vtkCallbackCommand::New();
  this->InterpreterObserver->SetCallback(&vtkPVProcessModule::InterpreterCallbackFunction);
  this->InterpreterObserver->SetClientData(this);
  this->Interpreter->AddObserver(vtkCommand::UserEvent,
                                 this->InterpreterObserver);

  // Assign standard IDs.
  vtkPVApplication *app = this->GetPVApplication();
  vtkClientServerStream css;
  css << vtkClientServerStream::Assign
      << this->GetApplicationID() << app
      << vtkClientServerStream::End;
  css << vtkClientServerStream::Assign
      << this->GetProcessModuleID() << this
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
  css << vtkClientServerStream::Delete
      << this->GetProcessModuleID()
      << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);

  // Free the interpreter and supporting stream.
  this->Interpreter->RemoveObserver(this->InterpreterObserver);
  this->InterpreterObserver->Delete();
  delete this->ClientServerStream;
  this->Interpreter->Delete();
  this->Interpreter = 0;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InterpreterCallbackFunction(vtkObject*,
                                                     unsigned long eid,
                                                     void* cd, void* d)
{
  reinterpret_cast<vtkPVProcessModule*>(cd)->InterpreterCallback(eid, d);
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InterpreterCallback(unsigned long, void* pinfo)
{
  if(!this->ReportInterpreterErrors)
    {
    return;
    }

  const char* errorMessage;
  vtkClientServerInterpreterErrorCallbackInfo* info
    = static_cast<vtkClientServerInterpreterErrorCallbackInfo*>(pinfo);
  const vtkClientServerStream& last = this->Interpreter->GetLastResult();
  if(last.GetNumberOfMessages() > 0 &&
     (last.GetCommand(0) == vtkClientServerStream::Error) &&
     last.GetArgument(0, 0, &errorMessage))
    {
    ostrstream error;
    error << "\nwhile processing\n";
    info->css->PrintMessage(error, info->message);
    error << ends;
    vtkErrorMacro(<< errorMessage << error.str());
    error.rdbuf()->freeze(0);
    vtkErrorMacro("Aborting execution for debugging purposes.");
    vtkPVApplication::Abort();
    }
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::LoadModule(const char* name)
{
  this->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID() << "LoadModuleInternal" << name
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
int vtkPVProcessModule::LoadModuleInternal(const char* name)
{
  return this->Interpreter->Load(name);
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::SendStringToClient(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToClient();
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::SendStringToClientAndServer(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToClientAndServer();
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::SendStringToClientAndServerRoot(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToClientAndServerRoot();
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::SendStringToServer(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToServer();
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::SendStringToServerRoot(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToServerRoot();
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVProcessModule::GetStringFromServer()
{
  return this->GetLastServerResult().StreamToString();
}

//----------------------------------------------------------------------------
const char* vtkPVProcessModule::GetStringFromClient()
{
  return this->GetLastClientResult().StreamToString();
}

/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h"
#include "vtkProcessModule.h"

#include "vtkAlgorithm.h"
#include "vtkCallbackCommand.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformation.h"
#include "vtkInstantiator.h"
#include "vtkToolkits.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVRenderModule.h"
#include "vtkPVOptions.h"
#include "vtkProcessModuleGUIHelper.h"

#include <vtkstd/map>

#include "vtkSmartPointer.h"
#include "vtkStdString.h"

vtkProcessModule* vtkProcessModule::ProcessModule = 0;

struct vtkProcessModuleInternals
{
  typedef 
  vtkstd::map<vtkStdString, vtkSmartPointer<vtkDataObject> > DataTypesType;
  
  DataTypesType DataTypes;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkProcessModule, "1.18");
vtkCxxSetObjectMacro(vtkProcessModule, RenderModule, vtkPVRenderModule);

//----------------------------------------------------------------------------
//****************************************************************************
class vtkProcessModuleObserver: public vtkCommand
{
public:
  static vtkProcessModuleObserver *New() 
    {return new vtkProcessModuleObserver;};

  vtkProcessModuleObserver()
    {
    this->PM = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
    void* calldata)
    {
    if ( this->PM )
      {
      this->PM->ExecuteEvent(wdg, event, calldata);
      }
    this->AbortFlagOn();
    }

  void SetPM(vtkProcessModule* pm)
    {
    this->PM = pm;
    }

private:
  vtkProcessModule* PM;
};
//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
{
  this->UniqueID.ID = 3;
  this->Controller = NULL;
  this->TemporaryInformation = NULL;
  this->ClientServerStream = 0;
  this->Interpreter = 0;
  this->InterpreterObserver = 0;
  this->ReportInterpreterErrors = 1;
  this->Internals = new vtkProcessModuleInternals;
  this->Observer = vtkProcessModuleObserver::New();
  this->Observer->SetPM(this);

  this->ProgressEnabled = 0;
  this->ProgressRequests = 0;
  this->ProgressHandler = vtkPVProgressHandler::New();
  this->RenderModule = 0;
  this->GUIHelper = 0;
  this->LogFile = 0;
}

//----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule()
{
  if(this->GUIHelper)
    {
    this->GUIHelper->Delete();
    }
  this->ProgressHandler->Cleanup();
  this->ProgressHandler->Delete();
  this->ProgressHandler = 0;
  // Free Interpreter and ClientServerStream.
  this->FinalizeInterpreter();

  // Other cleanup.
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
  this->Observer->Delete();
  this->Observer = 0;
  this->SetRenderModule(0);

  delete this->Internals;

  if (this->LogFile)
    {
    this->LogFile->close();
    delete this->LogFile;
    this->LogFile = 0;
    }
}

//----------------------------------------------------------------------------
vtkProcessModule* vtkProcessModule::GetProcessModule()
{
  return vtkProcessModule::ProcessModule;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetProcessModule(vtkProcessModule* pm)
{
  vtkProcessModule::ProcessModule = pm;
}

//----------------------------------------------------------------------------
void vtkProcessModule::GatherInformationRenderServer(vtkPVInformation* ,
                                                     vtkClientServerID )
{
  vtkErrorMacro("This should only be called from the client of a client render server mode paraview");
}

//----------------------------------------------------------------------------
void vtkProcessModule::GatherInformation(vtkPVInformation* info,
                                         vtkClientServerID id)
{
  // Just a simple way of passing the information object to the next
  // method.
  vtkClientServerStream stream;
  this->TemporaryInformation = info;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "GatherInformationInternal" 
         << info->GetClassName() 
         << id
         << vtkClientServerStream::End;
  this->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
  this->TemporaryInformation = NULL;
}

//----------------------------------------------------------------------------
void vtkProcessModule::GatherInformationInternal(const char*,
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
    vtkErrorMacro("Object id name must be wrong.");
    return;
    }

  this->TemporaryInformation->CopyFromObject(object);
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkProcessModule::CreateSendFlag(vtkTypeUInt32 servers)
{
  if(servers)
    {
    return CLIENT;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStream(vtkTypeUInt32 servers, 
                                 vtkClientServerStream& stream,
                                 int resetStream)
{
  vtkTypeUInt32 sendflag = this->CreateSendFlag(servers);
  if(sendflag & DATA_SERVER)
    {
    this->SendStreamToDataServer(stream);
    }
  if(sendflag & RENDER_SERVER)
    {
    this->SendStreamToRenderServer(stream);
    }
  if(sendflag & DATA_SERVER_ROOT)
    {
    this->SendStreamToDataServerRoot(stream);
    }
  if(sendflag & RENDER_SERVER_ROOT)
    {
    this->SendStreamToRenderServerRoot(stream);
    }
  if(sendflag & CLIENT)
    {
    this->SendStreamToClient(stream);
    }
  if(resetStream)
    {
    stream.Reset();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStreamToClient(vtkClientServerStream& stream)
{
  this->Interpreter->ProcessStream(stream);
  return 0;
}

//----------------------------------------------------------------------------
// send a stream to the data server
int vtkProcessModule::SendStreamToDataServer(vtkClientServerStream&)
{
  vtkErrorMacro("SendStreamToDataServer called on process module that does not implement it");
  return -1;
}

//----------------------------------------------------------------------------
// send a stream to the data server root mpi process
int vtkProcessModule::SendStreamToDataServerRoot(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToDataServerRoot called on process module that does not implement it");
  return -1;
}

//----------------------------------------------------------------------------
// send a stream to the render server
int vtkProcessModule::SendStreamToRenderServer(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToRenderServer called on process module that does not implement it");
  return -1;
}

//----------------------------------------------------------------------------
// send a stream to the render server root mpi process
int vtkProcessModule::SendStreamToRenderServerRoot(vtkClientServerStream&)
{
  vtkErrorMacro(
    "SendStreamToRenderServerRoot called on process module that does not implement it");
  return -1;
}


//----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::NewStreamObject(
  const char* type, vtkClientServerStream& stream)
{
  vtkClientServerID id = this->GetUniqueID();
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;
  return id;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkProcessModule::GetObjectFromID(vtkClientServerID id)
{
  return this->Interpreter->GetObjectFromID(id);
}

//----------------------------------------------------------------------------
void vtkProcessModule::DeleteStreamObject(
  vtkClientServerID id, vtkClientServerStream& stream)
{
  stream << vtkClientServerStream::Delete << id
         <<  vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastResult(vtkTypeUInt32 server)
{
  switch(server)
    {
    case DATA_SERVER:
    case DATA_SERVER_ROOT:
      return this->GetLastDataServerResult();
      break;
    case RENDER_SERVER:
    case RENDER_SERVER_ROOT:
      return this->GetLastRenderServerResult();
      break;
    case CLIENT:
      return this->GetLastClientResult();
    }
  vtkWarningMacro("GetLastResult called with a bad server flag returning CLIENT result");
  return this->GetLastClientResult();
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastDataServerResult()
{
  return this->GetLastClientResult();
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastRenderServerResult()
{
  return this->GetLastClientResult();
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastClientResult()
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
vtkClientServerInterpreter* vtkProcessModule::GetInterpreter()
{
  return this->Interpreter;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetUniqueID()
{
  this->UniqueID.ID++;
  return this->UniqueID;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetProcessModuleID()
{
  vtkClientServerID id = {2};
  return id;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkProcessModule::GetDataObjectOfType(const char* classname)
{
  if (!classname)
    {
    return 0;
    }

  // Since we can not instantiate these classes, we'll replace
  // them with a subclass
  if (strcmp(classname, "vtkDataSet") == 0)
    {
    classname = "vtkImageData";
    }
  else if (strcmp(classname, "vtkPointSet") == 0)
    {
    classname = "vtkPolyData";
    }

  vtkProcessModuleInternals::DataTypesType::iterator it =
    this->Internals->DataTypes.find(classname);
  if (it != this->Internals->DataTypes.end())
    {
    return it->second.GetPointer();
    }

  vtkObject* object = vtkInstantiator::CreateInstance(classname);
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);
  if (!dobj)
    {
    if (object)
      {
      object->Delete();
      }
    return 0;
    }

  this->Internals->DataTypes[classname] = dobj;
  dobj->Delete();

  return dobj;
}

//----------------------------------------------------------------------------
void vtkProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "ProgressRequests: " << this->ProgressRequests << endl;
  os << indent << "ProgressHandler: " << this->ProgressHandler << endl;
  os << indent << "ProgressEnabled: " << this->ProgressEnabled << endl;
  os << indent << "ReportInterpreterErrors: "
     << this->ReportInterpreterErrors << endl;
  os << indent << "Options:" << (this->Options?"":"(none)") << endl;
  if ( this->Options )
    {
    this->Options->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkProcessModule::InitializeInterpreter()
{
  if(this->Interpreter)
    {
    return;
    }

  // Create the interpreter and supporting stream.
  this->Interpreter = vtkClientServerInterpreter::New();
  this->ClientServerStream = new vtkClientServerStream;

  // Setup a callback for the interpreter to report errors.
  this->InterpreterObserver = vtkCallbackCommand::New();
  this->InterpreterObserver->SetCallback(
    &vtkProcessModule::InterpreterCallbackFunction);
  this->InterpreterObserver->SetClientData(this);
  this->Interpreter->AddObserver(vtkCommand::UserEvent,
    this->InterpreterObserver);

  // Assign standard IDs.
  // TODO move this to subclass
  //   vtkPVApplication *app = this->GetPVApplication();
  vtkClientServerStream css;
  //   css << vtkClientServerStream::Assign
  //       << this->GetApplicationID() << app
  //       << vtkClientServerStream::End;
  css << vtkClientServerStream::Assign
    << this->GetProcessModuleID() << this
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);

  bool needLog = false;
  if(getenv("VTK_CLIENT_SERVER_LOG"))
    {
    needLog = true;
    if(this-Options->GetClientMode())
      {
      needLog = false;
      this->GetInterpreter()->SetLogFile("paraviewClient.log");
      }
    if(this->Options->GetServerMode())
      {
      needLog = false;
      this->GetInterpreter()->SetLogFile("paraviewServer.log");
      }
    if(this->Options->GetRenderServerMode())
      {
      needLog = false;
      this->GetInterpreter()->SetLogFile("paraviewRenderServer.log");
      }
    } 
  if(needLog)
    {
    this->GetInterpreter()->SetLogFile("paraview.log");
    }
}

//----------------------------------------------------------------------------
void vtkProcessModule::FinalizeInterpreter()
{
  if(!this->Interpreter)
    {
    return;
    }

  // Delete the standard IDs.
  vtkClientServerStream css;
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
void vtkProcessModule::InterpreterCallbackFunction(vtkObject*,
                                                   unsigned long eid,
                                                   void* cd, void* d)
{
  reinterpret_cast<vtkProcessModule*>(cd)->InterpreterCallback(eid, d);
}

//----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallback(unsigned long, void* pinfo)
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
    cerr << errorMessage << endl;
    error.rdbuf()->freeze(0);
    vtkErrorMacro("Aborting execution for debugging purposes.");
    abort();
    }
}

//----------------------------------------------------------------------------
void vtkProcessModule::RegisterProgressEvent(vtkObject* po, int id)
{
  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(po);
  if ( !alg )
    {
    return;
    }
  alg->AddObserver(vtkCommand::ProgressEvent, this->Observer);
  this->ProgressHandler->RegisterProgressEvent(alg, id);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendPrepareProgress()
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetProcessModuleID() << "PrepareProgress" 
         << vtkClientServerStream::End;
  this->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
  this->ProgressRequests ++;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendCleanupPendingProgress()
{
  if ( this->ProgressRequests < 0 )
    {
    vtkErrorMacro("Internal ParaView Error: Progress requests went below zero");
    abort();
    }
  this->ProgressRequests --;
  if ( this->ProgressRequests > 0 )
    {
    return;
    }
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetProcessModuleID() << "CleanupPendingProgress" 
         << vtkClientServerStream::End;
  this->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkProcessModule::PrepareProgress()
{
  this->ProgressHandler->PrepareProgress(this);
}

//----------------------------------------------------------------------------
void vtkProcessModule::CleanupPendingProgress()
{
  this->ProgressHandler->CleanupPendingProgress(this);
}

//----------------------------------------------------------------------------
void vtkProcessModule::ProgressEvent(vtkObject *o, int val, const char* str)
{
  this->ProgressHandler->InvokeProgressEvent(this, o, val, str);
}

//----------------------------------------------------------------------------
void vtkProcessModule::ExecuteEvent(
  vtkObject *o, unsigned long event, void* calldata)
{
  switch ( event ) 
    {
  case vtkCommand::ProgressEvent:
      {
      int progress = 
        static_cast<int>(*reinterpret_cast<double*>(calldata)* 100.0);
      this->ProgressEvent(o, progress, 0);
      }
    break;
  case vtkCommand::WrongTagEvent:
      {
      int tag = -1;
      int len = -1;
      char val = -1;
      const char* data = reinterpret_cast<const char*>(calldata);
      const char* ptr = data;
      memcpy(&tag, ptr, sizeof(tag));
      if ( tag != vtkProcessModule::PROGRESS_EVENT_TAG )
        {
        vtkErrorMacro("Internal ParaView Error: "
                      "Socket Communicator received wrong tag: " 
                      << tag);
        abort();
        return;
        }
      ptr += sizeof(tag);
      memcpy(&len, ptr, sizeof(len));
      ptr += sizeof(len);
      val = *ptr;
      ptr ++;
      if ( val < 0 || val > 100 )
        {
        vtkErrorMacro("Received progres not in the range 0 - 100: " << (int)val);
        return;
        }
      this->ProgressEvent(o, val, ptr);
      }
    break;
    }
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetGUIHelper(vtkProcessModuleGUIHelper* h)
{
  if ( this->GUIHelper )
    {
    this->GUIHelper->UnRegister(this);
    this->GUIHelper = 0;
    }
  if ( h )
    {
    this->GUIHelper = h;
    h->Register(this);
    }
}

//----------------------------------------------------------------------------
int vtkProcessModule::SetupRenderModule()
{
  const char* renderModuleName = this->Options->GetRenderModuleName();
  // The client chooses a render module.
  if (renderModuleName == NULL)
    { // The render module has not been set by the user.  Choose a default.
    if (this->Options->GetTileDimensions()[0])
      {
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
      renderModuleName = "IceTRenderModule";
#else
      renderModuleName = "MultiDisplayRenderModule";
#endif
      }
    else if (this->Options->GetClientMode())
      { // Client server, no tiled display.
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
      renderModuleName = "DeskTopRenderModule";
#else
      renderModuleName = "MPIRenderModule";
#endif        
      }
    else
      { // Single process, or one MPI program
#ifdef VTK_USE_MPI
      renderModuleName = "MPIRenderModule";
#else
      renderModuleName = "LODRenderModule";
#endif
      }
    }
  
  // Create the rendering module here.
  char* rmClassName;
  rmClassName = new char[strlen(renderModuleName) + 20];
  sprintf(rmClassName, "vtkPV%s", renderModuleName);
  vtkObject* o = vtkInstantiator::CreateInstance(rmClassName);
  vtkPVRenderModule* rm = vtkPVRenderModule::SafeDownCast(o);
  if (rm == 0)
    {
    vtkErrorMacro("Could not create render module " << rmClassName);
    renderModuleName = "RenderModule";
    o = vtkInstantiator::CreateInstance("vtkPVRenderModule");
    rm = vtkPVRenderModule::SafeDownCast(o);
    if ( rm == 0 )
      {
      vtkErrorMacro("Could not create the render module.");
      return 0;
      }
    }
  if (this->ProcessModule == NULL)
    {
    vtkErrorMacro("missing ProcessModule");
    return 0;
    }
  else
    { // Looks like a circular reference to me!
    this->SetRenderModule(rm);
    rm->SetProcessModule(this);
    }
  o->Delete();
  o = NULL;
  rm = NULL;

  delete [] rmClassName;
  rmClassName = NULL;

  this->Options->SetRenderModuleName(renderModuleName);

  return 1;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetOptions(vtkPVOptions* op)
{
  this->Options = op;
  if ( op ) 
    {
    if ( op->GetServerMode() )
      {
      this->GetProgressHandler()->SetServerMode(1);
      }
    if ( op->GetClientMode() )
      {
      this->GetProgressHandler()->SetClientMode(1);
      }
    }
}

//----------------------------------------------------------------------------
vtkCommand* vtkProcessModule::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void vtkProcessModule::Initialize()
{
  this->InitializeInterpreter();
}

//----------------------------------------------------------------------------
void vtkProcessModule::Finalize()
{
  this->SetGUIHelper(0);
  this->SetRenderModule(0);
  this->FinalizeInterpreter();
}

//----------------------------------------------------------------------------
ofstream* vtkProcessModule::GetLogFile()
{
  return this->LogFile;
}

//----------------------------------------------------------------------------
void vtkProcessModule::CreateLogFile(const char *prefix)
{
  if (!prefix)
    {
    return;
    }
  ostrstream fileName;
  fileName << prefix << this->Controller->GetLocalProcessId() << ".txt"
           << ends;
  if (this->LogFile)
    {
    this->LogFile->close();
    delete this->LogFile;
    }
  this->LogFile = new ofstream(fileName.str(), ios::out);
  fileName.rdbuf()->freeze(0);
  if (this->LogFile->fail())
    {
    delete this->LogFile;
    this->LogFile = 0;
    }
}

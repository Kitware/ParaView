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
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkSource.h"
#include "vtkStringList.h"
#include "vtkToolkits.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"
#include "vtkTimerLog.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVServerInformation.h"
#include "vtkInstantiator.h"
#include "vtkPVOptions.h"

#include "vtkPVDemoPaths.h"

#include <sys/stat.h>

#include <kwsys/SystemTools.hxx>

// initialze the class variables
int vtkPVProcessModule::GlobalLODFlag = 0;


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProcessModule);
vtkCxxRevisionMacro(vtkPVProcessModule, "1.32");

//----------------------------------------------------------------------------
vtkPVProcessModule::vtkPVProcessModule()
{
  this->MPIMToNSocketConnectionID.ID = 0;
  this->LogThreshold = 0;
  this->DemoPath = 0;
  this->ServerInformation = vtkPVServerInformation::New();
  this->UseTriangleStrips = 0;
  this->UseImmediateMode = 1;
  this->Options = 0;
  this->ApplicationInstallationDirectory = 0;
  this->Timer = vtkTimerLog::New();
}

//----------------------------------------------------------------------------
vtkPVProcessModule::~vtkPVProcessModule()
{ 
  this->SetGUIHelper(0);
  this->SetDemoPath(0);
  this->SetApplicationInstallationDirectory(0);
  this->FinalizeInterpreter();
  this->ServerInformation->Delete();
  this->Timer->Delete();
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::Start(int argc, char **argv)
{
  if(!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set, for vtkPVProcessModule to be able to run a gui.");
    return -1;
    }
  
  if (this->Controller == NULL)
    {
    this->Controller = vtkDummyController::New();
    vtkMultiProcessController::SetGlobalController(this->Controller);
    }

  this->CreateLogFile("NodeLog");

  return this->GUIHelper->RunGUIStart(argc, argv, 1, 0);
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::Exit()
{
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetDirectoryListing(const char* dir,
                                            vtkStringList* dirs,
                                            vtkStringList* files,
                                            int save)
{
  // Get the listing from the server.
  vtkClientServerStream stream;
  vtkClientServerID lid = 
    this->NewStreamObject("vtkPVServerFileListing", stream);
  stream << vtkClientServerStream::Invoke
         << lid << "GetFileListing" << dir << save
         << vtkClientServerStream::End;
  this->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
  vtkClientServerStream result;
  if(!this->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result))
    {
    vtkErrorMacro("Error getting file list result from server.");
    this->DeleteStreamObject(lid, stream);
    this->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);
    return 0;
    }
  this->DeleteStreamObject(lid, stream);
  this->SendStream(vtkProcessModule::DATA_SERVER_ROOT, stream);

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
void vtkPVProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LogThreshold: " << this->LogThreshold << endl;
  os << indent << "DemoPath: " << (this->DemoPath?this->DemoPath:"(none)") << endl;
  os << indent << "UseTriangleStrips: " << this->UseTriangleStrips << endl;
  os << indent << "UseImmediateMode: " << this->UseImmediateMode << endl;

  os << indent << "Options: ";
  if(this->Options)
    {
    this->Options->PrintSelf(os << endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ServerInformation: ";
  if (this->ServerInformation)
    {
    this->ServerInformation->PrintSelf(os << endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;;
    }
  os << indent << "ApplicationInstallationDirectory: " << (this->ApplicationInstallationDirectory?this->ApplicationInstallationDirectory:"(none)") << endl;
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::InitializeInterpreter()
{
  if(this->Interpreter)
    {
    return;
    }

  this->Superclass::InitializeInterpreter();

}

//----------------------------------------------------------------------------
void vtkPVProcessModule::FinalizeInterpreter()
{
  if(!this->Interpreter)
    {
    return;
    }

  this->Superclass::FinalizeInterpreter();
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::LoadModule(const char* name, const char* directory)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "LoadModuleInternal" << name << directory
         << vtkClientServerStream::End;
  this->SendStream(vtkProcessModule::DATA_SERVER, stream);
  int result = 0;
  if(!this->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result))
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

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendPrepareProgress()
{ 
  if(!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set, for SendPrepareProgress.");
    return;
    }
  this->GUIHelper->SendPrepareProgress();
  this->Superclass::SendPrepareProgress();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SendCleanupPendingProgress()
{
  this->Superclass::SendCleanupPendingProgress();
  if ( this->ProgressRequests > 0 )
    {
    return;
    }
 if(!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set, for SendCleanupPendingProgress.");
    return;
    }
  this->GUIHelper->SendCleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SetLocalProgress(const char* filter, int progress)
{
 if(!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set, for SetLocalProgress.  " << filter << " " << progress);
    return;
    }
 this->GUIHelper->SetLocalProgress(filter, progress);
}


//----------------------------------------------------------------------------
void vtkPVProcessModule::LogStartEvent(char* str)
{
  vtkTimerLog::MarkStartEvent(str);
  this->Timer->StartTimer();
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::LogEndEvent(char* str)
{
  this->Timer->StopTimer();
  vtkTimerLog::MarkEndEvent(str);
  if (strstr(str, "id:") && this->LogFile)
    {
    *this->LogFile << str << ", " << this->Timer->GetElapsedTime()
                   << " seconds" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::SetLogBufferLength(int length)
{
  vtkTimerLog::SetMaxEntries(length);
}

//----------------------------------------------------------------------------
void vtkPVProcessModule::ResetLog()
{
  vtkTimerLog::ResetLog();
}
//----------------------------------------------------------------------------
void vtkPVProcessModule::SetEnableLog(int flag)
{
  vtkTimerLog::SetLogging(flag);
}
//----------------------------------------------------------------------------
void vtkPVProcessModule::SetGlobalLODFlag(int val)
{
  if (vtkPVProcessModule::GlobalLODFlag == val)
    {
    return;
    }
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "SetGlobalLODFlagInternal"
         << val
         << vtkClientServerStream::End;
  this->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
}

 
//----------------------------------------------------------------------------
void vtkPVProcessModule::SetGlobalLODFlagInternal(int val)
{
  vtkPVProcessModule::GlobalLODFlag = val;
}



//----------------------------------------------------------------------------
int vtkPVProcessModule::GetGlobalLODFlag()
{
  return vtkPVProcessModule::GlobalLODFlag;
}

//============================================================================
// Stuff that is a part of render-process module.

//-----------------------------------------------------------------------------
const char* vtkPVProcessModule::GetDemoPath()
{
  int found=0;
  char temp1[1024];
  struct stat fs;

  this->SetDemoPath(NULL);

  if(this->Options)
    {
    kwsys_stl::string selfPath, errorMsg;
    if (kwsys::SystemTools::FindProgramPath(
          this->Options->GetArgv0(), selfPath, errorMsg))
      {
      const char* relPath = "../share/paraview-" PARAVIEW_VERSION "/Demos";
      char* newPath = new char[selfPath.size()+strlen(relPath)+2];
      sprintf(newPath, "%s/%s", selfPath.c_str(), relPath);

      char* demoFile = new char[strlen(newPath)+strlen("/Demo1.pvs")+1];
      sprintf(demoFile, "%s/Demo1.pvs", newPath);

      if (stat(demoFile, &fs) == 0)
        {
        this->SetDemoPath(newPath);
        found = 1;
        }
      delete[] demoFile;
      delete[] newPath;
      }
    }

  if (!found)
    {
    // Look in binary and installation directories
    const char** dir;
    for(dir=VTK_PV_DEMO_PATHS; !found && *dir; ++dir)
      {
      sprintf(temp1, "%s/Demo1.pvs", *dir);
      if (stat(temp1, &fs) == 0) 
        {
        this->SetDemoPath(*dir);
        found = 1;
        }
      }
    }

  return this->DemoPath;
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetRenderNodePort()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetRenderNodePort();
}

//----------------------------------------------------------------------------
char* vtkPVProcessModule::GetMachinesFileName()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetMachinesFileName();
}

//----------------------------------------------------------------------------
int vtkPVProcessModule::GetClientMode()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetClientMode();
}

//----------------------------------------------------------------------------
// This method leaks memory.  It is a quick and dirty way to set different 
// DISPLAY environment variables on the render server.  I think the string 
// cannot be deleted until paraview exits.  The var should have the form:
// "DISPLAY=amber1"
void vtkPVProcessModule::SetProcessEnvironmentVariable(int processId,
                                                       const char* var)
{
  (void)processId;
  char* envstr = kwsys::SystemTools::DuplicateString(var);
  putenv(envstr);
}

//-----------------------------------------------------------------------------
int vtkPVProcessModule::SetupRenderModule()
{
  // If the user has not set rendering options on the client, get them from
  // the server.
  if (!this->Options->GetTileDimensions()[0])
    {
    this->Options->SetTileDimensions
      (this->ServerInformation->GetTileDimensions());
    }
  if (!this->Options->GetUseOffscreenRendering())
    {
    this->Options->SetUseOffscreenRendering
      (this->ServerInformation->GetUseOffscreenRendering());
    }

  const char *renderModuleName = this->Options->GetRenderModuleName();
  if (renderModuleName == NULL)
    {
    // If we are in client/server mode, the server options determine the
    // render module.
    if (this->Options->GetTileDimensions()[0])
      {
      if (this->ServerInformation->GetUseIceT())
        {
        renderModuleName = "IceTRenderModule";
        }
      else
        {
        renderModuleName = "MultiDisplayRenderModule";
        }
      }
    else if (this->Options->GetClientMode())
      {
      if (this->ServerInformation->GetUseIceT())
        {
        renderModuleName = "DeskTopRenderModule";
        }
      else
        {
        renderModuleName = "MPIRenderModule";
        }
      }
    else
      {
      // We are not in Client/Server mode, so we can just use local info.
#ifdef VTK_USE_MPI
      renderModuleName = "MPIRenderModule";
#else
      renderModuleName = "LODRenderModule";
#endif
      }
    this->Options->SetRenderModuleName(renderModuleName);
    }

  return this->Superclass::SetupRenderModule();
}

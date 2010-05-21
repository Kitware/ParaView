/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSynchronizedRenderWindowsClient.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"

class vtkPVSynchronizedRenderWindowsClient::vtkInternals
{
public:
  struct RenderWindowInfo
    {
    vtkSmartPointer<vtkRenderWindow> RenderWindow;
    int Size[2];
    int Position[2];
    RenderWindowInfo()
      {
      this->Size[0] = this->Size[1] = 0;
      this->Position[0] = this->Position[1] = 0;
      }
    };

  typedef vtkstd::map<unsigned int, RenderWindowInfo> RenderWindowsMap;
  RenderWindowsMap RenderWindows;

  unsigned int GetKey(vtkRenderWindow* win)
    {
    RenderWindowsMap::iterator iter;
    for (iter = this->RenderWindows.begin(); iter != this->RenderWindows.end();
      ++iter)
      {
      if (iter->second.RenderWindow == win)
        {
        return iter->first;
        }
      }
    return 0;
    }
};

vtkStandardNewMacro(vtkPVSynchronizedRenderWindowsClient);
vtkCxxRevisionMacro(vtkPVSynchronizedRenderWindowsClient, "$Revision$");
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindowsClient::vtkPVSynchronizedRenderWindowsClient()
{
  this->Internals = new vtkInternals();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm || pm->GetActiveRenderServerSocketController())
    {
    vtkErrorMacro(
      "vtkPVSynchronizedRenderWindowsClient cannot be used in the current\n"
      "setup. Aborting for debugging purposes.");
    abort();
    }

  this->SetParallelController(pm->GetActiveRenderServerSocketController());

  // since the processes on which this class is instantiated is always the
  // client. As far as the superclass is concerned, this process is the root
  // node.
  this->SetRootProcessId(0);
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindowsClient::~vtkPVSynchronizedRenderWindowsClient()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVSynchronizedRenderWindowsClient:NewRenderWindow()
{
  // client always creates new window for each view in the multi layout
  // configuration.
  return vtkRenderWindow::New();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::AddRenderWindow(
  unsigned int id, vtkRenderWindow* renWin)
{
  assert(renWin != NULL && id != 0);

  if (this->Internals->RenderWindows.find(id) !=
    this->Internals->RenderWindows.end())
    {
    vtkErrorMacro("ID for render window already in use: " << id);
    return;
    }

  this->Internals->RenderWindows[id].RenderWindow = renWin;
  renWin->AddObserver(vtkCommand::StartEvent, this->Observer);
  renWin->AddObserver(vtkCommand::EndEvent, this->Observer);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::RemoveRenderWindow(
  vtkRenderWindow* renWin)
{
  this->RemoveRenderWindow(this->Internals->GetKey(renWin));
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::RemoveRenderWindow(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    iter->second.RemoveObserver(this->Observer);
    this->Internals->RenderWindows.erase(iter);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::HandleStartRender(vtkRenderWindow* renWin)
{
  this->RenderWindow = renWin;
  this->Superclass::HandleStartRender(renWin);
  this->RenderWindow = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::MasterStartRender()
{
  // Need to the tell the server which is the active render window.
  vtkMultiProcessStream stream;
  stream << this->Internals->GetKey(this->RenderWindow);

  // Pass in the information about the layout for all the windows.
  // TODO: This gets called when rendering each render window. However, this
  // information does not necessarily get invalidated that frequently. Can we be
  // smart about it?
  this->SaveWindowLayout(stream);

  vtkstd::vector<unsigned char> data;
  stream.GetRawData(data);

  this->ParallelController->TriggerRMIOnAllChildren(
    &data[0], static_cast<int>(data.size()), SYNC_MULTI_RENDER_WINDOW_TAG);

  this->Superclass::MasterStartRender();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SlaveStartRender()
{
  vtkErrorMacro("This class must be created on the slave process.");
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SetWindowSize(unsigned int id,
  int width, int height)
{
  this->Internals->RenderWindow[id].Size[0] = width;
  this->Internals->RenderWindow[id].Size[1] = height;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SetWindowPosition(unsigned int id,
  int px, int py)
{
  this->Internals->RenderWindow[id].Position[0] = px;
  this->Internals->RenderWindow[id].Position[1] = py;
}

//----------------------------------------------------------------------------
const int *vtkPVSynchronizedRenderWindowsClient::GetWindowSize(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    return iter->second.Size;
    }
  return NULL;
}

//----------------------------------------------------------------------------
const int *vtkPVSynchronizedRenderWindowsClient::GetWindowPosition(unsigned int id)
{
  vtkInternals::RenderWindowsMap::iterator iter =
    this->Internals->RenderWindows.find(id);
  if (iter != this->Internals->RenderWindows.end())
    {
    return iter->second.Position;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::SaveWindowLayout(
  vtkMultiProcessStream& stream)
{
  int full_size[2] = {0, 0};
  stream << static_cast<unsigned int>(this->Internals->RenderWindowsMap.size());

  vtkInternals::RenderWindowsMap::iterator iter;
  for (iter = this->Internals->RenderWindows.begin();
    iter != this->Internals->RenderWindows.end(); ++iter)
    {
    const int *actual_size = iter->second.Size;
    const int *position = iter->second.Position;

    full_size[0] = full_size[0] > (position[0] + actual_size[0])?
      full_size[0] : position[0] + actual_size[0];
    full_size[1] = full_size[1] > (position[1] + actual_size[1])?
      full_size[1] : position[1] + actual_size[1];
    stream << iter->first << position[0] << position[1]
      << actual_size[0] << actual_size[1];
    }
  // Now push the full size.
  stream << full_size[0] << full_size[1];
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindowsClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

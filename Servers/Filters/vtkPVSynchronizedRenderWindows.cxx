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
#include "vtkPVSynchronizedRenderWindows.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVSynchronizedRenderWindows);
vtkCxxRevisionMacro(vtkPVSynchronizedRenderWindows, "$Revision$");
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindows::vtkPVSynchronizedRenderWindows()
{
  this->Identifier = 0;
  this->ImageReductionFactor = 1;
  this->RenderWindow = NULL;
  this->RemoteRendering = true;
  this->RenderEventPropagation = true;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSocketController cs_controller =
    pm->GetActiveRenderServerSocketController();

  vtkMultiProcessController mpi_controller = vtkMultiProcessController::GetGlobalController();
  if (cs_controller)
    {
    bool is_server = pm->GetOptions()->GetServerMode() != 0;

    this->SyncWindowsCS = vtkSmartPointer<vtkSynchronizedRenderWindows>::New();
    this->SyncWindowsCS->SetParallelController(cs_controller);
    this->SyncWindowsCS->SetRootProcessId(is_server? 1 : 0);

    this->SyncRenderersCS = vtkSmartPointer<vtkSynchronizedRenderers>::New();
    this->SyncRenderersCS->SetParallelController(cs_controller);
    this->SyncRenderersCS->SetRootProcessId(is_server? 1 : 0);
    }

  if (mpi_controller && mpi_controller->GetNumberOfProcesses() > 1)
    {
    // this is never true on client.

    this->SyncWindowsP = vtkSmartPointer<vtkSynchronizedRenderWindows>::New();
    this->SyncWindowsP->SetParallelController(mpi_controller);

    this->SyncRenderersP = vtkSmartPointer<vtkSynchronizedRenderers>::New();
    this->SyncRenderersP->SetParallelController(mpi_controller);

    if (this->SyncWindowsCS)
      {
      // FIXME: If we are in client-server mode, then we don't need to write back
      // images unless in tile-display or cave-rendering mode.
      this->SyncRenderersP->WriteBackImagesOff();
      }
    }
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindows::~vtkPVSynchronizedRenderWindows()
{
  this->SetRenderWindow(0);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetRenderWindow(vtkRenderWindow* window)
{
  vtkSetObjectBodyMacro(RenderWindow, vtkRenderWindow, renWin);
  if (this->SyncWindowsCS)
    {
    this->SyncWindowsCS->SetRenderWindow(renWin);
    }
  if (this->SyncWindowsP)
    {
    this->SyncWindowsP->SetRenderWindow(renWin);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetIdentifier(unsigned int id)
{
  this->Identifier = id;
  if (this->SyncWindowsCS)
    {
    this->SyncWindowsCS->SetIdentifier(renWin);
    }

  if (this->SyncWindowsP)
    {
    this->SyncWindowsP->SetIdentifier(renWin);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::AddSynchronizedRenderer(vtkRenderer* ren)
{
  if (this->SyncRenderersCS)
    {
    this->SyncRenderersCS->SetRenderer(ren);
    }

  if (this->SyncRenderersP)
    {
    this->SyncRenderersP->SetRenderer(ren);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

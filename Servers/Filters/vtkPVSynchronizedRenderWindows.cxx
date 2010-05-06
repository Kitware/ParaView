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
  this->SyncRenderersP = vtkSynchronizedRenderers::New();
  this->SyncRenderersCS = vtkSynchronizedRenderers::New();

  this->SyncWindowsP = vtkSynchronizedRenderWindows::New();
  this->SyncWindowsCS = vtkSynchronizedRenderWindows::New();

  // this class handles the render event propagation part.
  this->SyncWindowsP->SetRenderEventPropagation(false);
  this->SyncWindowsCS->SetRenderEventPropagation(false);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSocketController cs_controller =
    pm->GetActiveRenderServerSocketController();

  vtkMultiProcessController mpi_controller = vtkMultiProcessController::GetGlobalController();

  if (!cs_controller)
    {
    this->SyncWindowsCS->ParallelRenderingOff();
    }

  if (!mpi_controller || mpi_controller->GetNumberOfProcesses() == 1)
    {
    this->SyncWindowsP->ParallelRenderingOff();
    }
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderWindows::~vtkPVSynchronizedRenderWindows()
{
  this->SetRenderWindow(0);

  this->SyncRenderersP->Delete();
  this->SyncWindowsP->Delete();

  this->SyncRenderersCS->Delete();
  this->SyncWindowsCS->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetRenderWindow(vtkRenderWindow* window)
{
  vtkSetObjectBodyMacro(RenderWindow, vtkRenderWindow, renWin);
  this->SyncWindowsCS->SetRenderWindow(renWin);
  this->SyncWindowsP->SetRenderWindow(renWin);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::SetIdentifier(unsigned int id)
{
  this->Identifier = id;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderWindows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

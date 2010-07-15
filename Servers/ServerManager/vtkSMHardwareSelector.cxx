/*=========================================================================

  Program:   ParaView
  Module:    vtkSMHardwareSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMHardwareSelector.h"

#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVHardwareSelector.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkSMHardwareSelector);
//----------------------------------------------------------------------------
vtkSMHardwareSelector::vtkSMHardwareSelector()
{
  this->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  // Camera observer is used to clear the buffers if the camera changes.
  vtkMemberFunctionCommand<vtkSMHardwareSelector>* observer =
    vtkMemberFunctionCommand<vtkSMHardwareSelector>::New();
  observer->SetCallback(*this, &vtkSMHardwareSelector::ClearBuffers);
  this->CameraObserver = observer;
}

//----------------------------------------------------------------------------
vtkSMHardwareSelector::~vtkSMHardwareSelector()
{
  this->CameraObserver->Delete();
}

//----------------------------------------------------------------------------
vtkSelection* vtkSMHardwareSelector::Select(unsigned int region[4])
{
  vtkPVHardwareSelector* selector = vtkPVHardwareSelector::SafeDownCast(
    this->GetClientSideObject());

  // Capture new buffers if needed, or uses
  this->CaptureBuffers();

  vtkSelection* sel = selector->GenerateSelection(region);

  return sel;
}

//----------------------------------------------------------------------------
void vtkSMHardwareSelector::StartSelectionPass()
{
  vtkPVHardwareSelector* selector = vtkPVHardwareSelector::SafeDownCast(
    this->GetClientSideObject());
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "SetCurrentPass"
          << selector->GetCurrentPass()
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkSMHardwareSelector::CaptureBuffers()
{
  vtkPVHardwareSelector* selector = vtkPVHardwareSelector::SafeDownCast(
    this->GetClientSideObject());

  if (this->CaptureTime < this->GetMTime())
    {
    //cout << "Clear And ReCapture" << endl;

    vtkMemberFunctionCommand<vtkSMHardwareSelector>* observer =
      vtkMemberFunctionCommand<vtkSMHardwareSelector>::New();
    observer->SetCallback(*this, &vtkSMHardwareSelector::StartSelectionPass);
    selector->AddObserver(vtkCommand::StartEvent, observer);

    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
      << this->GetID()
      << "BeginSelection"
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    selector->CaptureBuffers();

    stream  << vtkClientServerStream::Invoke
      << this->GetID()
      << "EndSelection"
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    selector->RemoveObserver(observer);
    observer->Delete();

    if (!selector->GetRenderer()->GetActiveCamera()->HasObserver(
        vtkCommand::ModifiedEvent, this->CameraObserver))
      {
      selector->GetRenderer()->GetActiveCamera()->AddObserver(vtkCommand::ModifiedEvent,
        this->CameraObserver);
      }

    this->CaptureTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSMHardwareSelector::ClearBuffers()
{
  if (this->CaptureTime > this->GetMTime())
    {
    // the check avoid calling ClearBuffers() when capture is happening.
    vtkPVHardwareSelector* selector = vtkPVHardwareSelector::SafeDownCast(
      this->GetClientSideObject());
    if (selector)
      {
      selector->ClearBuffers();
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSMHardwareSelector::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);

  // if any property changes, then this method is called, and we ensure that we
  // update our mtime.
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



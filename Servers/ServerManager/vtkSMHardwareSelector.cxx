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

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkPVHardwareSelector.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkSMHardwareSelector);
//----------------------------------------------------------------------------
vtkSMHardwareSelector::vtkSMHardwareSelector()
{
  this->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
vtkSMHardwareSelector::~vtkSMHardwareSelector()
{
}

//----------------------------------------------------------------------------
vtkSelection* vtkSMHardwareSelector::Select()
{
  vtkPVHardwareSelector* selector = vtkPVHardwareSelector::SafeDownCast(
    this->GetClientSideObject());

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
  
  vtkSelection* sel = selector->Select();

  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "EndSelection"
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream);
  
  selector->RemoveObserver(observer);
  observer->Delete();
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
void vtkSMHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



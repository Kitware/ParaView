/*=========================================================================

  Program:   ParaView
  Module:    vtkRM3DWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRM3DWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkRM3DWidget, "1.1");

//----------------------------------------------------------------------------
vtkRM3DWidget::vtkRM3DWidget()
{
  this->PVProcessModule = 0;
  this->Widget3DID.ID = 0;
}

//----------------------------------------------------------------------------
vtkRM3DWidget::~vtkRM3DWidget()
{
  if (this->Widget3DID.ID && this->PVProcessModule )
    {    
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID
                    << "EnabledOff" << vtkClientServerStream::End;
    this->PVProcessModule->DeleteStreamObject(this->Widget3DID);
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  this->Widget3DID.ID = 0;
  this->PVProcessModule = 0;
}
//----------------------------------------------------------------------------
void vtkRM3DWidget::Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, vtkClientServerID interactorID)
{
  this->PVProcessModule = pm;

  if(this->Widget3DID.ID != 0)
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID 
                    << "SetCurrentRenderer" 
                    << rendererID
                    << vtkClientServerStream::End;

    // Default/dummy interactor for satelite procs.
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID 
                    << "SetInteractor" 
                    << interactorID
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}
void vtkRM3DWidget::PlaceWidget(double bds[6])
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "PlaceWidget" 
                  << bds[0] << bds[1] << bds[2] << bds[3] 
                  << bds[4] << bds[5] << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}
//----------------------------------------------------------------------------
void vtkRM3DWidget::SetVisibility(int visibility)
{
  this->PVProcessModule->GetStream() 
                  << vtkClientServerStream::Invoke << this->Widget3DID
                  << "SetEnabled" << visibility << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
                  vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}
//----------------------------------------------------------------------------
void vtkRM3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Widget3DID: " << this->Widget3DID << endl;
}

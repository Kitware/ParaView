/*=========================================================================

  Program:   ParaView
  Module:    vtkRMSphereWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRMSphereWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"

vtkStandardNewMacro(vtkRMSphereWidget);
vtkCxxRevisionMacro(vtkRMSphereWidget, "1.1");

//----------------------------------------------------------------------------
vtkRMSphereWidget::vtkRMSphereWidget()
{
  this->SphereID.ID = 0;
  
  this->LastAcceptedCenter[0] = this->LastAcceptedCenter[1] =
    this->LastAcceptedCenter[2] = 0.0;
  this->LastAcceptedRadius = 1;
}

//----------------------------------------------------------------------------
vtkRMSphereWidget::~vtkRMSphereWidget()
{
  if (this->SphereID.ID && this->PVProcessModule )
    {
    this->PVProcessModule->DeleteStreamObject(this->SphereID);
    this->SphereID.ID = 0;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
}
//----------------------------------------------------------------------------
void vtkRMSphereWidget::SetCenter(double x, double y, double z)
{
  this->Center[0] = x;
  this->Center[1] = y;
  this->Center[2] = z;
  if ( this->Widget3DID.ID )
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                    << "SetCenter" << x << y << z
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------

void vtkRMSphereWidget::GetCenter(double pts[3])
{
  if(pts == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  pts[0] = this->Center[0];
  pts[1] = this->Center[1];
  pts[2] = this->Center[2];
}
//----------------------------------------------------------------------------
void vtkRMSphereWidget::SetRadius(double radius)
{
  this->Radius = radius;
  if ( this->Widget3DID.ID )
    {
    this->PVProcessModule->GetStream() 
                  << vtkClientServerStream::Invoke << this->Widget3DID
                  << "SetRadius" << this->Radius
                  << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(
                  vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMSphereWidget::ResetInternal()
{
  this->SetCenter(this->LastAcceptedCenter[0],
    this->LastAcceptedCenter[1],this->LastAcceptedCenter[2]);
  this->SetRadius(this->LastAcceptedRadius);
}
//----------------------------------------------------------------------------
void vtkRMSphereWidget::UpdateVTKObject()
{
  if ( this->SphereID.ID &&  this->PVProcessModule)
    {
    this->SetLastAcceptedCenter(this->Center);
    this->SetLastAcceptedRadius(this->Radius);

    this->PVProcessModule->GetStream() 
                    << vtkClientServerStream::Invoke << this->SphereID
                    << "SetCenter" << this->Center[0] 
                    << this->Center[1] << this->Center[2] 
                    << vtkClientServerStream::End;
    this->PVProcessModule->GetStream() 
                    << vtkClientServerStream::Invoke << this->SphereID
                    << "SetRadius" << this->Radius
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
}
//----------------------------------------------------------------------------
void vtkRMSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SphereID: " << this->SphereID << endl;
  os << indent << "Center: " << this->Center[0]
        << ", " << this->Center[1] << ", " <<this->Center[2] << endl;
  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "LastAcceptedCenter: " << this->LastAcceptedCenter[0]
        << ", " << this->LastAcceptedCenter[1] << ", " << 
        this->LastAcceptedCenter[2] << endl;
  os << indent << "LastAcceptedRadius: " << this->LastAcceptedRadius << endl;
}

void vtkRMSphereWidget::Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, 
    vtkClientServerID interactorID)
{
  this->Widget3DID = pm->NewStreamObject("vtkSphereWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "PlaceWidget" << 0 << 1 << 0 << 1 << 0 << 1 
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->SphereID = pm->NewStreamObject("vtkSphere");
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  this->Superclass::Create(pm,rendererID,interactorID);

}

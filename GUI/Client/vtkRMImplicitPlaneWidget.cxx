/*=========================================================================

  Program:   ParaView
  Module:    vtkRMImplicitPlaneWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRMImplicitPlaneWidget.h"

#include "vtkImplicitPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkRMImplicitPlaneWidget);
vtkCxxRevisionMacro(vtkRMImplicitPlaneWidget, "1.1");

//----------------------------------------------------------------------------
vtkRMImplicitPlaneWidget::vtkRMImplicitPlaneWidget()
{

  this->LastAcceptedCenter[0] = this->LastAcceptedCenter[1] =
    this->LastAcceptedCenter[2] = 0;
  this->LastAcceptedNormal[0] = this->LastAcceptedNormal[1] = 0;
  this->LastAcceptedNormal[2] = 1;
  this->PlaneID.ID = 0;
}

//----------------------------------------------------------------------------
vtkRMImplicitPlaneWidget::~vtkRMImplicitPlaneWidget()
{
  if (this->PVProcessModule && this->PlaneID.ID != 0)
    {
    this->PVProcessModule->DeleteStreamObject(this->PlaneID);
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    this->PlaneID.ID = 0;
    }
}

//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, 
    vtkClientServerID interactorID)
{
  this->PlaneID = pm->NewStreamObject("vtkPlane");
  // create the plane on all servers and client
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  // Create the 3D widget on each process.
  // This is for tiled display and client server.
  // This may decrease compresion durring compositing.
  // We should have a special call instead of broadcast script.
  // Better yet, control visibility based on mode (client-server ...). 
  this->Widget3DID = pm->NewStreamObject("vtkImplicitPlaneWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "SetPlaceFactor" << 1.0 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "OutlineTranslationOff"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "PlaceWidget"
                  << 0 << 1 << 0 << 1 << 0 << 1
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  float opacity = 1.0;
  if (pm->GetNumberOfPartitions() == 1)
    { 
    opacity = .25;
    }
  
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "GetPlaneProperty"
                  << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetOpacity" 
                  << opacity 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID
                  << "GetSelectedPlaneProperty" 
                  << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetOpacity" 
                  << opacity 
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->Superclass::Create(pm,rendererID,interactorID);
}
//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::SetCenter(double x, double y, double z)
{
  this->Center[0] = x;
  this->Center[1] = y;
  this->Center[2] = z;

  if ( this->Widget3DID.ID )
    { 
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID << "SetOrigin" << x << y << z
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  if ( this->PlaneID.ID )
    { 
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->PlaneID << "SetOrigin" << x << y << z
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::DATA_SERVER);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::GetCenter(double pts[3])
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
void vtkRMImplicitPlaneWidget::UpdateVTKObject(vtkClientServerID objectID, char *variableName)
{
  this->SetLastAcceptedCenter(this->Center[0],this->Center[1],this->Center[2]);
  this->SetLastAcceptedNormal(this->Normal[0],this->Normal[1],this->Normal[2]);
  this->SetDrawPlane(0);
  // This should be done in the initialization.
  // There must be a more general way of hooking up the plane object.
  // ExtractCTH uses this varible, General Clipping uses the select widget.
  if (variableName && objectID.ID != 0)
    {
    ostrstream str;
    str << "Set" << variableName << ends;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << objectID
                    << str.str() << this->PlaneID << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::DATA_SERVER);
    delete [] str.str();
    }
  if ( this->PlaneID.ID != 0 )
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
          << this->PlaneID << "SetOrigin"
          << this->Center[0] << this->Center[1] <<  this->Center[2] 
          << vtkClientServerStream::End;

    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
          << this->PlaneID << "SetNormal"
          << this->Normal[0] << this->Normal[1] <<  this->Normal[2] 
          << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::SetNormal(double x, double y, double z)
{
  this->Normal[0] = x;
  this->Normal[1] = y;
  this->Normal[2] = z;

  if ( this->Widget3DID.ID)
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID << "SetNormal" << x << y << z
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    } 
  if ( this->PlaneID.ID )
    { 
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->PlaneID << "SetNormal" << x << y << z
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::DATA_SERVER);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::GetNormal(double pts[3])
{
  if(pts == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  pts[0] = this->Normal[0];
  pts[1] = this->Normal[1];
  pts[2] = this->Normal[2];
}
//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::ResetInternal()
{
  this->SetDrawPlane(0);
  this->SetCenter(this->LastAcceptedCenter[0],
      this->LastAcceptedCenter[1],
      this->LastAcceptedCenter[2]);
  this->SetNormal(this->LastAcceptedNormal[0],
    this->LastAcceptedNormal[1],
    this->LastAcceptedNormal[2]);
}
//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::SetDrawPlane(int val)
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "SetDrawPlane" << val
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  
}
//----------------------------------------------------------------------------
void vtkRMImplicitPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PlaneID: " << this->PlaneID;
  os << indent << "LastAcceptedCenter: " << this->LastAcceptedCenter[0]
        << ", " << this->LastAcceptedCenter[1] << ", " << 
        this->LastAcceptedCenter[2] << endl;
  os << indent << "LastAcceptedNormal: " << this->LastAcceptedNormal[0]
        << ", " << this->LastAcceptedNormal[1] << ", " << 
        this->LastAcceptedNormal[2] << endl;

}

/*=========================================================================

  Program:   ParaView
  Module:    vtkRMBoxWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRMBoxWidget.h"

#include "vtkImplicitPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"

#include "vtkTransform.h"
#include "vtkPlanes.h"
#include "vtkBoxWidget.h"

vtkStandardNewMacro(vtkRMBoxWidget);
vtkCxxRevisionMacro(vtkRMBoxWidget, "1.1");

//----------------------------------------------------------------------------
vtkRMBoxWidget::vtkRMBoxWidget()
{
  this->BoxID.ID = 0;
  this->BoxMatrixID.ID = 0;
  this->BoxTransformID.ID = 0;

  this->BoxTransform = 0;
  this->Box = 0;

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->Position[cc] = 0.0;
    this->Scale[cc] = 1.0;
    this->Rotation[cc] = 0.0;
    this->LastAcceptedPosition[cc] = 0.0;
    this->LastAcceptedScale[cc] = 1.0;
    this->LastAcceptedRotation[cc] = 0.0;
    }
}

//----------------------------------------------------------------------------
vtkRMBoxWidget::~vtkRMBoxWidget()
{
  if (this->PVProcessModule && this->BoxID.ID)
    {
    this->PVProcessModule->DeleteStreamObject(this->BoxID);
    this->BoxID.ID = 0;
    }
  if (this->PVProcessModule && this->BoxTransformID.ID )
    {
    this->PVProcessModule->DeleteStreamObject(this->BoxTransformID);
    this->BoxTransformID.ID = 0; 
    }
  if (this->PVProcessModule && this->BoxMatrixID.ID)
    {
    this->PVProcessModule->DeleteStreamObject(this->BoxMatrixID);
    this->BoxMatrixID.ID = 0;
    }
  if(this->PVProcessModule)
    {
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::Create(vtkPVProcessModule *pm, 
    vtkClientServerID rendererID, vtkClientServerID interactorID)
{
  this->Widget3DID = pm->NewStreamObject("vtkBoxWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "SetPlaceFactor" << 1.0 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "PlaceWidget"
                  << 0 << 1 << 0 << 1 << 0 << 1
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  this->BoxID = pm->NewStreamObject("vtkPlanes");
  this->BoxMatrixID = pm->NewStreamObject("vtkMatrix4x4");
  this->BoxTransformID = pm->NewStreamObject("vtkTransform");
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->Box = vtkPlanes::SafeDownCast(pm->GetObjectFromID(this->BoxID));
  this->BoxTransform = vtkTransform::SafeDownCast(pm->GetObjectFromID(this->BoxTransformID));
   
  this->Superclass::Create(pm,rendererID,interactorID);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::Update()
{
  vtkTransform* trans = this->BoxTransform;
  trans->Identity();
  trans->Translate(this->Position);
  trans->RotateZ(this->Rotation[2]);
  trans->RotateX(this->Rotation[0]);
  trans->RotateY(this->Rotation[1]);
  trans->Scale(this->Scale);
  vtkMatrix4x4* mat = trans->GetMatrix();
 
  this->PVProcessModule->GetStream() 
                  << vtkClientServerStream::Invoke << this->BoxTransformID
                  << "Identity" << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke << this->BoxMatrixID
                  << "DeepCopy" 
                  << vtkClientServerStream::InsertArray(&mat->Element[0][0], 16)
                  << vtkClientServerStream::End;
  this->PVProcessModule->GetStream() 
                  << vtkClientServerStream::Invoke << this->BoxTransformID
                  << "SetMatrix" << this->BoxMatrixID << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  this->PVProcessModule->GetStream() 
                  << vtkClientServerStream::Invoke << this->Widget3DID
                  << "SetTransform" << this->BoxTransformID << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::SetPosition(double x, double y, double z)
{
  this->SetPositionNoEvent(x,y,z);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::SetPositionNoEvent(double x, double y, double z)
{
  if(this->Position[0] != x || this->Position[1] != y || this->Position[2] != z)
    {
    this->Position[0] = x;
    this->Position[1] = y;
    this->Position[2] = z;
    this->Update();
    }
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::GetPosition(double pts[3])
{
  if(pts == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>
    (this->PVProcessModule->GetObjectFromID(this->Widget3DID));
  box->GetTransform(this->BoxTransform);
  this->BoxTransform->GetPosition(this->Position);
  pts[0] = this->Position[0];
  pts[1] = this->Position[1];
  pts[2] = this->Position[2];
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::SetRotation(double x, double y, double z)
{
  this->SetRotationNoEvent(x,y,z);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::SetRotationNoEvent(double x, double y, double z)
{
  if(this->Rotation[0] != x || this->Rotation[1] != y || this->Rotation[2] != z)
    {
    this->Rotation[0] = x;
    this->Rotation[1] = y;
    this->Rotation[2] = z;
    this->Update();
    }
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::GetRotation(double pts[3])
{
  if(pts == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>
    (this->PVProcessModule->GetObjectFromID(this->Widget3DID));
  box->GetTransform(this->BoxTransform);
  this->BoxTransform->GetOrientation(this->Rotation);
  pts[0] = this->Rotation[0];
  pts[1] = this->Rotation[1];
  pts[2] = this->Rotation[2];
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::SetScale(double x, double y, double z)
{
  this->SetScaleNoEvent(x,y,z);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::SetScaleNoEvent(double x, double y, double z)
{
  if(this->Scale[0] != x || this->Scale[1] != y || this->Scale[2] != z)
    {
    this->Scale[0] = x;
    this->Scale[1] = y;
    this->Scale[2] = z;
    this->Update();
    }
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::GetScale(double pts[3])
{
  if(pts == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>
    (this->PVProcessModule->GetObjectFromID(this->Widget3DID));
  box->GetTransform(this->BoxTransform);
  this->BoxTransform->GetScale(this->Scale);
  pts[0] = this->Scale[0];
  pts[1] = this->Scale[1];
  pts[2] = this->Scale[2];
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::ResetInternal()
{
  this->Rotation[0] = this->LastAcceptedRotation[0];
  this->Rotation[1] = this->LastAcceptedRotation[1];
  this->Rotation[2] = this->LastAcceptedRotation[2];

  this->Scale[0] = this->LastAcceptedScale[0];
  this->Scale[1] = this->LastAcceptedScale[1];
  this->Scale[2] = this->LastAcceptedScale[2];

  this->Position[0] = this->LastAcceptedPosition[0];
  this->Position[1] = this->LastAcceptedPosition[1];
  this->Position[2] = this->LastAcceptedPosition[2];
  this->Update();
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::PlaceWidget(double bds[6])
{
  this->Superclass::PlaceWidget(bds);
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "GetPlanes" << this->BoxID 
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::UpdateVTKObject()
{
  if ( this->BoxID.ID )
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID << "GetPlanes" << this->BoxID 
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT_AND_SERVERS); 
    this->SetLastAcceptedPosition(this->Position);
    this->SetLastAcceptedRotation(this->Rotation);
    this->SetLastAcceptedScale(this->Scale);
    }
}
//----------------------------------------------------------------------------
void vtkRMBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BoxID: " << this->BoxID.ID << endl;
  os << indent << "Box: "   << this->Box << endl;
  os << indent << "BoxMatrixID: " << this->BoxMatrixID << endl;
  os << indent << "BoxTransform: " << this->BoxTransform << endl;
  os << indent << "BoxTransformID" << this->BoxTransformID << endl;
  os << indent << "LastAcceptedPosition: " <<this->LastAcceptedPosition[0]
        << ", " << this->LastAcceptedPosition[1] << ", " << 
        this->LastAcceptedPosition[2] << endl;
  os << indent << "LastAcceptedRotation: " <<this->LastAcceptedRotation[0]
        << ", " << this->LastAcceptedRotation[1] << ", " << 
        this->LastAcceptedRotation[2] << endl;
  os << indent << "LastAcceptedScale: " <<this->LastAcceptedScale[0]
        << ", " << this->LastAcceptedScale[1] << ", " << 
        this->LastAcceptedScale[2] << endl;
}

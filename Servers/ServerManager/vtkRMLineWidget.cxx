/*=========================================================================

  Program:   ParaView
  Module:    vtkRMLineWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRMLineWidget.h"

#include "vtkLineWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"

vtkStandardNewMacro(vtkRMLineWidget);
vtkCxxRevisionMacro(vtkRMLineWidget, "1.1");

//----------------------------------------------------------------------------
vtkRMLineWidget::vtkRMLineWidget()
{
  this->LastAcceptedPoint1[0] = -0.5;
  this->LastAcceptedPoint1[1] = this->LastAcceptedPoint1[2] = 0;
  this->LastAcceptedPoint2[0] = 0.5;
  this->LastAcceptedPoint2[1] = this->LastAcceptedPoint2[2] = 0;
  this->LastAcceptedResolution = 1;
  this->Resolution = 1;
  this->Point1[0] = -0.5;
  this->Point1[1] = this->Point1[2] = 0;
  this->Point2[0] = 0.5;
  this->Point2[1] = this->Point2[2] = 0;
}

//----------------------------------------------------------------------------
vtkRMLineWidget::~vtkRMLineWidget()
{

}
//----------------------------------------------------------------------------
void vtkRMLineWidget::SetPoint1(double x, double y, double z)
{
  this->Point1[0] = x;
  this->Point1[1] = y;
  this->Point1[2] = z;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  <<  this->Widget3DID
                  << "SetPoint1" << this->Point1[0] << this->Point1[1] 
                  <<  this->Point1[2]
                  << vtkClientServerStream::End;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  <<  this->Widget3DID
                  << "SetAlignToNone" <<  vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMLineWidget::GetPoint1(double pt[3])
{
  if (pt == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  pt[0] = this->Point1[0];
  pt[1] = this->Point1[1];
  pt[2] = this->Point1[2];
}
//----------------------------------------------------------------------------
void vtkRMLineWidget::SetPoint2(double x, double y, double z)
{
  this->Point2[0] = x;
  this->Point2[1] = y;
  this->Point2[2] = z;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  <<  this->Widget3DID
                  << "SetPoint2" << this->Point2[0] << this->Point2[1] 
                  <<  this->Point2[2]
                  << vtkClientServerStream::End;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  <<  this->Widget3DID
                  << "SetAlignToNone" <<  vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMLineWidget::GetPoint2(double pt[3])
{
  if (pt == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  pt[0] = this->Point2[0];
  pt[1] = this->Point2[1];
  pt[2] = this->Point2[2];
}
//----------------------------------------------------------------------------
void vtkRMLineWidget::SetResolution(int res)
{
  this->Resolution = res;
  if(this->Widget3DID.ID != 0)
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    <<  this->Widget3DID
                    << "SetResolution" << this->Resolution
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
    }
}

//----------------------------------------------------------------------------
void vtkRMLineWidget::Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, vtkClientServerID interactorID)
{
  this->Widget3DID = pm->NewStreamObject("vtkLineWidget");
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetAlignToNone" << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->Superclass::Create(pm,rendererID,interactorID);
}
//----------------------------------------------------------------------------
void vtkRMLineWidget::UpdateVTKObject(vtkClientServerID objectID, char *point1Variable, char *point2Variable, char *resolutionVariable)
{
  this->SetLastAcceptedPoint1(this->Point1[0],
                              this->Point1[1],
                              this->Point1[2]);
  this->SetLastAcceptedPoint2(this->Point2[0],
                              this->Point2[1],
                              this->Point2[2]);
  this->SetLastAcceptedResolution(this->Resolution);

  char acceptCmd[1024];
  if ( point1Variable && objectID.ID )    
    {
    sprintf(acceptCmd, "Set%s", point1Variable);
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << objectID
                    << acceptCmd << this->Point1[0]
                    << this->Point1[1] << this->Point1[2]
                    << vtkClientServerStream::End;
    }
  if ( point2Variable && objectID.ID )
    {
    sprintf(acceptCmd, "Set%s", point2Variable);
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << objectID
                    << acceptCmd << this->Point2[0]
                    << this->Point2[1] << this->Point2[2]
                    << vtkClientServerStream::End;
    }
  if ( resolutionVariable && objectID.ID )
    {
    sprintf(acceptCmd, "Set%s", resolutionVariable);
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << objectID
                    << acceptCmd << this->Resolution
                    << vtkClientServerStream::End;
    }
  this->PVProcessModule->SendStream(vtkProcessModule::DATA_SERVER);
}
//----------------------------------------------------------------------------
void vtkRMLineWidget::ResetInternal()
{
  this->SetPoint1(this->LastAcceptedPoint1[0], this->LastAcceptedPoint1[1],
                  this->LastAcceptedPoint1[2]);
  this->SetPoint2(this->LastAcceptedPoint2[0], this->LastAcceptedPoint2[1],
                  this->LastAcceptedPoint2[2]);
  this->SetResolution(static_cast<int>(this->LastAcceptedResolution));

}
void vtkRMLineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Point1: " << this->Point1[0] << "," <<
    this->Point1[1] << "," << this->Point1[2] << endl;
  os << indent << "Point2: " << this->Point2[0] << "," <<
    this->Point2[1] << "," << this->Point2[2] << endl;
  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "LastAcceptedPoint1: " << this->LastAcceptedPoint1[0] << ", "
    << this->LastAcceptedPoint1[1] <<  ", " << this->LastAcceptedPoint1[2] << endl;
  os << indent << "LastAcceptedPoint2: " << this->LastAcceptedPoint2[0] << ", "
    << this->LastAcceptedPoint2[1] <<  ", " << this->LastAcceptedPoint2[2] << endl;
  os << indent << "LastAcceptedResolution: " << this->LastAcceptedResolution << endl;
}

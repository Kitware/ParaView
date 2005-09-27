/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBoxWidgetProxy.h"

#include "vtkImplicitPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkTransform.h"
#include "vtkBoxWidget.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"


vtkStandardNewMacro(vtkSMBoxWidgetProxy);
vtkCxxRevisionMacro(vtkSMBoxWidgetProxy, "1.6");

//----------------------------------------------------------------------------
vtkSMBoxWidgetProxy::vtkSMBoxWidgetProxy()
{
  this->BoxTransform = 0;

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->Position[cc] = 0.0;
    this->Scale[cc] = 1.0;
    this->Rotation[cc] = 0.0;
    }
  this->SetVTKClassName("vtkBoxWidget");
}

//----------------------------------------------------------------------------
vtkSMBoxWidgetProxy::~vtkSMBoxWidgetProxy()
{
  vtkSMProxyManager *pxm = vtkSMObject::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("ProxyManger does not exist");
    }
  this->BoxTransform = 0;
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
    
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    
    stream << vtkClientServerStream::Invoke << id 
           << "SetPlaceFactor" << 1.0 
           << vtkClientServerStream::End;
    
    stream << vtkClientServerStream::Invoke << id
           << "PlaceWidget"
           << 0 << 1 << 0 << 1 << 0 << 1
           << vtkClientServerStream::End;
    pm->SendStream(this->GetServers(), stream, 1);
    }
  
  vtkSMProxy* transformProxy = this->GetSubProxy("transform");
  
  if (!transformProxy)
    {
    vtkErrorMacro("Tranform must be defined in the configuration file");
    return;
    }
  transformProxy->UpdateVTKObjects(); 
  if (!this->GetSubProxy("matrix"))
    {
    vtkErrorMacro("Matrix proxy must be defined in the configuration file");
    return;
    }
  this->BoxTransform = vtkTransform::SafeDownCast(
    pm->GetObjectFromID(transformProxy->GetID(0)));
  
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetMatrix(vtkMatrix4x4* mat)
{
  if (this->GetNumberOfIDs() == 0)
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkSMProxy* matrixProxy = this->GetSubProxy("matrix");
  vtkSMProxy* transformProxy = this->GetSubProxy("transform");
  if (!matrixProxy || ! transformProxy)
    {
    vtkErrorMacro("Matrix and Transform proxies required. Must be added to configuration file");
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    matrixProxy->GetProperty("DeepCopy"));
  if (dvp)
    {
    double *p = &mat->Element[0][0];
    for (unsigned int cc=0; cc < 16; cc ++)
      {
      dvp->SetElement(cc,p[cc]); 
      }
    }
  else
    {
    vtkErrorMacro("Could not find property DeepCopy on Matrix4x4");
    return;
    }
  matrixProxy->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    transformProxy->GetProperty("MatrixProxy"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(matrixProxy);
    }
  else
    {
    vtkErrorMacro("Could not find property Matrix on Transform");
    return;
    }
  transformProxy->UpdateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID transformid = transformProxy->GetID(0);
  vtkClientServerStream stream;
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetTransform" << transformid << vtkClientServerStream::End;
    pm->SendStream(this->GetServers(), stream, 1);
    }
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::GetMatrix(vtkMatrix4x4* mat)
{
  if (!this->BoxTransform)
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkTransform* trans = this->BoxTransform;
  trans->Identity();
  trans->Translate(this->Position);
  trans->RotateZ(this->Rotation[2]);
  trans->RotateX(this->Rotation[0]);
  trans->RotateY(this->Rotation[1]);
  trans->Scale(this->Scale);
  mat->DeepCopy(trans->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  // Generate the tranformation matrix from the pos/rot/scale
  vtkMatrix4x4* mat = vtkMatrix4x4::New();
  this->GetMatrix(mat);
  // Send the transformation to the client/servers.
  this->SetMatrix(mat);
  mat->Delete();
}


//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkBoxWidget* widget = vtkBoxWidget::SafeDownCast(wdg);
  if (!widget)
    {
    vtkErrorMacro( "This is not a box widget" );
    return;
    }
  if (!this->BoxTransform)
    {
    //not created yet...ignore these events.
    return;
    }
  // Get the values from the client object and set the ivars.
  widget->GetTransform(this->BoxTransform);
  if (event != vtkCommand::PlaceWidgetEvent || !this->IgnorePlaceWidgetChanges)
    {
    this->BoxTransform->GetPosition(this->Position);
    this->BoxTransform->GetOrientation(this->Rotation);
    this->BoxTransform->GetScale(this->Scale);
    }
  this->Superclass::ExecuteEvent(wdg, event,p);
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SaveInBatchScript(ofstream *file)
{
  this->Superclass::SaveInBatchScript(file);
  *file << endl;
  int i;
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    for (i=0; i<3; i++)
      {
      *file << "  [$pvTemp" << id.ID
        << " GetProperty Rotation] SetElement " << i << " "
        << this->Rotation[i] << endl;
      *file << "  [$pvTemp" << id.ID
        << " GetProperty RotationInfo] SetElement " << i << " "
        << this->Rotation[i] << endl;
      }
    for (i=0; i<3; i++)
      {
      *file << "  [$pvTemp" << id.ID
        << " GetProperty Scale] SetElement " << i << " "
        << this->Scale[i] << endl;
      *file << "  [$pvTemp" << id.ID
        << " GetProperty ScaleInfo] SetElement " << i << " "
        << this->Scale[i] << endl;
      }
    for (i=0; i<3; i++)
      {
      *file << "  [$pvTemp" << id.ID
        << " GetProperty Position] SetElement " << i << " "
        << this->Position[i] << endl;
      *file << "  [$pvTemp" << id.ID
        << " GetProperty PositionInfo] SetElement " << i << " "
        << this->Position[i] << endl;
      }
    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects"<<endl;
    }
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Position: " << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << endl;
  os << indent << "Rotation: " << this->Rotation[0] << ", "
    << this->Rotation[1] << ", " << this->Rotation[2] << endl;
  os << indent << "Scale: " << this->Scale[0] << ", "
    << this->Scale[1] << ", " << this->Scale[2] << endl;
  os << indent << "BoxTransform: " << this->BoxTransform << endl;
}

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
#include "vtkKWEvent.h"

#include "vtkTransform.h"
#include "vtkBoxWidget.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"


vtkStandardNewMacro(vtkSMBoxWidgetProxy);
vtkCxxRevisionMacro(vtkSMBoxWidgetProxy, "1.1");

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
  this->MatrixProxy = 0;
  this->TransformProxy = 0;
  this->MatrixProxyName = 0;
  this->TransformProxyName = 0;
  this->SetVTKClassName("vtkBoxWidget");
}

//----------------------------------------------------------------------------
vtkSMBoxWidgetProxy::~vtkSMBoxWidgetProxy()
{
  if(this->TransformProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("transforms",
      this->TransformProxyName);
    }
  this->SetTransformProxyName(0);
  if(this->TransformProxy)
    {
    this->TransformProxy->Delete();
    this->TransformProxy = 0;
    }
  
  if(this->MatrixProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("matrices",
      this->MatrixProxyName);
    }
  this->SetMatrixProxyName(0);
  if(this->MatrixProxy)
    {
    this->MatrixProxy->Delete();
    this->MatrixProxy = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  static int proxyNum = 0;
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    
    pm->GetStream() << vtkClientServerStream::Invoke << id 
                    << "SetPlaceFactor" << 1.0 
                    << vtkClientServerStream::End;
    
    pm->GetStream() << vtkClientServerStream::Invoke << id
                    << "PlaceWidget"
                    << 0 << 1 << 0 << 1 << 0 << 1
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  
  //Create Transform proxy
  this->TransformProxy = pxm->NewProxy("transforms","Transform");
  this->TransformProxy->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  ostrstream str;
  str << "vtkSMBoxWidgetProxy_Transform" << proxyNum << ends;
  this->SetTransformProxyName(str.str());
  str.rdbuf()->freeze(0);
  pxm->RegisterProxy("transforms",this->TransformProxyName, this->TransformProxy);
  this->TransformProxy->CreateVTKObjects(1);
  str.clear();

  //Create matrix proxy
  this->MatrixProxy = pxm->NewProxy("matrices","Matrix4x4");
  this->MatrixProxy->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  str << "vtkSMBoxWidgetProxy_Matrix4x4" << proxyNum << ends;
  this->SetMatrixProxyName(str.str());
  str.rdbuf()->freeze(0);
  pxm->RegisterProxy("matrices",this->MatrixProxyName, this->MatrixProxy);
  this->MatrixProxy->CreateVTKObjects(1);

  this->BoxTransform = vtkTransform::SafeDownCast(
    pm->GetObjectFromID(this->TransformProxy->GetID(0)));
  
  proxyNum++;
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetMatrix(double elements[16])
{
  vtkMatrix4x4* mat = vtkMatrix4x4::New();
  mat->Zero();
  for(int x=0;x<4;x++)
    {
    for(int y=0;y<4;y++)
      {
      mat->SetElement(x,y,elements[x*4+y]);
      }
    }
  this->SetMatrix(mat);
  mat->Delete();
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetMatrix(vtkMatrix4x4* mat)
{
  if (this->GetNumberOfIDs() == 0)
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->MatrixProxy->GetProperty("DeepCopy"));
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
  this->MatrixProxy->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->TransformProxy->GetProperty("MatrixProxy"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(this->MatrixProxy);
    }
  else
    {
    vtkErrorMacro("Could not find property Matrix on Transform");
    return;
    }
  this->TransformProxy->UpdateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID transformid = this->TransformProxy->GetID(0);
  for (unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    pm->GetStream()<< vtkClientServerStream::Invoke << this->GetID(cc)
      << "SetTransform" << transformid << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
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
double* vtkSMBoxWidgetProxy::GetMatrix()
{
  vtkMatrix4x4* mat = vtkMatrix4x4::New();
  this->GetMatrix(mat);
  for(int x=0;x<4;x++)
    {
    for(int y=0;y<4;y++)
      {
      this->Matrix[x][y] = mat->Element[x][y];
      }
    }
  return &this->Matrix[0][0];
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::Update()
{
  vtkMatrix4x4* mat = vtkMatrix4x4::New();
  this->GetMatrix(mat);
  this->SetMatrix(mat);
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetPosition(double x, double y, double z)
{
  this->SetPositionNoEvent(x,y,z);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetPositionNoEvent(double x, double y, double z)
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
void vtkSMBoxWidgetProxy::GetPositionInternal()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->GetNumberOfIDs() == 0)
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>
    (pm->GetObjectFromID(this->GetID(0)));
  box->GetTransform(this->BoxTransform);
  this->BoxTransform->GetPosition(this->Position);
}

//----------------------------------------------------------------------------
double* vtkSMBoxWidgetProxy::GetPosition()
{
  this->GetPositionInternal(); 
  return this->Position; 
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::GetPosition(double &x, double &y, double&z)
{
  this->GetPositionInternal();
  x = this->Position[0];
  y = this->Position[1];
  z = this->Position[2];
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::GetPosition(double position[3])
{
  this->GetPositionInternal();
  position[0] = this->Position[0];
  position[1] = this->Position[1];
  position[2] = this->Position[2];
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetRotation(double x, double y, double z)
{
  this->SetRotationNoEvent(x,y,z);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetRotationNoEvent(double x, double y, double z)
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
void vtkSMBoxWidgetProxy::GetRotationInternal()
{
  if (this->GetNumberOfIDs() == 0)
    {
    vtkErrorMacro("Not created yet");
    return ;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>
    (pm->GetObjectFromID(this->GetID(0)));
  box->GetTransform(this->BoxTransform);
  this->BoxTransform->GetOrientation(this->Rotation);
}
//----------------------------------------------------------------------------
double* vtkSMBoxWidgetProxy::GetRotation()
{
  this->GetRotationInternal();
  return this->Rotation;
}
void vtkSMBoxWidgetProxy::GetRotation(double &x, double &y, double &z)
{
  this->GetRotationInternal();
  x = this->Rotation[0];
  y = this->Rotation[1];
  z = this->Rotation[2];
}
void vtkSMBoxWidgetProxy::GetRotation(double rotation[3])
{
  this->GetRotationInternal();
  rotation[0] = this->Rotation[0];
  rotation[1] = this->Rotation[1];
  rotation[2] = this->Rotation[2];
}
//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetScale(double x, double y, double z)
{
  this->SetScaleNoEvent(x,y,z);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SetScaleNoEvent(double x, double y, double z)
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
void vtkSMBoxWidgetProxy::GetScaleInternal()
{
  if (this->GetNumberOfIDs() == 0)
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>
    (pm->GetObjectFromID(this->GetID(0)));
  box->GetTransform(this->BoxTransform);
  this->BoxTransform->GetScale(this->Scale);
}

//----------------------------------------------------------------------------
double *vtkSMBoxWidgetProxy::GetScale()
{
  this->GetScaleInternal();
  return this->Scale;
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::GetScale(double &x, double &y, double &z)
{
  this->GetScaleInternal();
  x = this->Scale[0];
  y = this->Scale[1];
  z = this->Scale[2];
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::GetScale(double scale[3])
{
  this->GetScaleInternal();
  scale[0] = this->Scale[0];
  scale[1] = this->Scale[1];
  scale[2] = this->Scale[2];
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
  this->Superclass::ExecuteEvent(wdg, event,p);
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::SaveInBatchScript(ofstream *file)
{
  this->Superclass::SaveInBatchScript(file);
  *file << endl;
  double *elements = this->GetMatrix();

  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    for(int i = 0; i < 16; i ++)
      {
      *file << "  [$pvTemp" << id.ID 
        << " GetProperty Matrix] SetElement " << i << " "
        << elements[i] << endl;
      }
    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects"<<endl;
    }
}

//----------------------------------------------------------------------------
void vtkSMBoxWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MatrixProxyName: " 
    << (this->MatrixProxyName? this->MatrixProxyName : "none") << endl;
  os << indent << "MatrixProxy: " << this->MatrixProxy << endl;
  os << indent << "TransformProxyName: " << 
    (this->TransformProxyName? this->TransformProxyName: "none") << endl;
  os << indent << "TransformProxy: " << this->TransformProxy << endl;
  os << indent << "BoxTransform: " << this->BoxTransform << endl;
}

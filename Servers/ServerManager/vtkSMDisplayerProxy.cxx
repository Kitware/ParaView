/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayerProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplayerProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkSMCommunicationModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"

#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMDisplayerProxy);
vtkCxxRevisionMacro(vtkSMDisplayerProxy, "1.1");

//---------------------------------------------------------------------------
vtkSMDisplayerProxy::vtkSMDisplayerProxy()
{
  this->SetVTKClassName("vtkPVGeometryFilter");

  this->MapperProxy = vtkSMProxy::New();
  this->ActorProxy = vtkSMProxy::New();
  this->PropertyProxy = vtkSMProxy::New();

  vtkSMIntVectorProperty* intVec;
  vtkSMDoubleVectorProperty* doubleVec;

  // Create the SM properties for the vtkPVGeometryFilter proxy

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetUseOutline");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 0);
  this->AddProperty("DisplayAsOutline", intVec);
  intVec->Delete();

  // Create the SM properties for the vtkProperty proxy

  double ones[3] = {1.0, 1.0, 1.0};

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetColor");
  doubleVec->SetNumberOfElements(3);
  doubleVec->SetElements(ones);
  this->AddProperty("Color", doubleVec, 0, 0);
  this->PropertyProxy->AddProperty("Color", doubleVec);
  doubleVec->Delete();

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetInterpolation");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 1);
  this->AddProperty("Interpolation", intVec, 0, 0);
  this->PropertyProxy->AddProperty("Interpolation", intVec);
  intVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetPointSize");
  doubleVec->SetNumberOfElements(1);
  doubleVec->SetElement(0, 1);
  this->AddProperty("PointSize", doubleVec, 0, 0);
  this->PropertyProxy->AddProperty("PointSize", doubleVec);
  doubleVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetLineWidth");
  doubleVec->SetNumberOfElements(1);
  doubleVec->SetElement(0, 1);
  this->AddProperty("LineWidth", doubleVec, 0, 0);
  this->PropertyProxy->AddProperty("LineWidth", doubleVec);
  doubleVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetOpacity");
  doubleVec->SetNumberOfElements(1);
  doubleVec->SetElement(0, 1);
  this->AddProperty("Opacity", doubleVec, 0, 0);
  this->PropertyProxy->AddProperty("Opacity", doubleVec);
  doubleVec->Delete();

  // Create the specialized SM properties for this

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetScalarVisibility");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 0);
  this->AddProperty("ScalarVisibility", intVec, 1, 0);
  intVec->Delete();

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetRepresentation");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 2);
  this->AddProperty("Representation", intVec, 1, 0);
  intVec->Delete();

  // Create the SM properties for the vtkMapper proxy
  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetImmediateModeRendering");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 1);
  this->AddProperty("ImmediateModeRendering", intVec, 0, 0);
  this->MapperProxy->AddProperty("ImmediateModeRendering", intVec);
  intVec->Delete();

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetScalarMode");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 0);
  this->AddProperty("ScalarMode", intVec, 0, 0);
  this->MapperProxy->AddProperty("ScalarMode", intVec);
  intVec->Delete();

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetColorMode");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 0);
  this->AddProperty("ColorMode", intVec, 1, 0);
  this->MapperProxy->AddProperty("ColorMode", intVec);
  intVec->Delete();

  vtkSMStringVectorProperty* stringVec = vtkSMStringVectorProperty::New();
  stringVec->SetCommand("SelectColorArray");
  stringVec->SetNumberOfElements(1);
  stringVec->SetElement(0, "");
  this->AddProperty("ColorArray", stringVec, 1, 0);
  this->MapperProxy->AddProperty("ColorArray", stringVec);
  stringVec->Delete();

  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::New();
  proxyProp->SetCommand("SetLookupTable");
  proxyProp->SetProxy(0);
  this->AddProperty("LookupTable", proxyProp, 1, 0);
  this->MapperProxy->AddProperty("LookupTable", proxyProp);
  proxyProp->Delete();

  // Create the SM properties for the vtkActor proxy

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetVisibility");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 1);
  this->AddProperty("Visibility", intVec, 0, 0);
  this->ActorProxy->AddProperty("Visibility", intVec);
  intVec->Delete();

  double zeros[3] = {0.0, 0.0, 0.0};

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetPosition");
  doubleVec->SetNumberOfElements(3);
  doubleVec->SetElements(zeros);
  this->AddProperty("Position", doubleVec, 0, 0);
  this->ActorProxy->AddProperty("Position", doubleVec);
  doubleVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetScale");
  doubleVec->SetNumberOfElements(3);
  doubleVec->SetElements(ones);
  this->AddProperty("Scale", doubleVec, 0, 0);
  this->ActorProxy->AddProperty("Scale", doubleVec);
  doubleVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetOrientation");
  doubleVec->SetNumberOfElements(3);
  doubleVec->SetElements(zeros);
  this->AddProperty("Orientation", doubleVec, 0, 0);
  this->ActorProxy->AddProperty("Orientation", doubleVec);
  doubleVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetOrigin");
  doubleVec->SetNumberOfElements(3);
  doubleVec->SetElements(zeros);
  this->AddProperty("Origin", doubleVec, 0, 0);
  this->ActorProxy->AddProperty("Origin", doubleVec);
  doubleVec->Delete();
}

//---------------------------------------------------------------------------
vtkSMDisplayerProxy::~vtkSMDisplayerProxy()
{
  this->MapperProxy->Delete();
  this->ActorProxy->Delete();
  this->PropertyProxy->Delete();
}

//---------------------------------------------------------------------------
// We overwrite this method to make sure that it is forwarded to the
// sub-proxies as well.
void vtkSMDisplayerProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  this->PushProperty("ScalarVisibility", this->ClientServerID, 0);
  this->SetPropertyModifiedFlag("ScalarVisibility", 0);
  
  this->PushProperty("Representation", this->ClientServerID, 0);
  this->SetPropertyModifiedFlag("Representation", 0);

  this->ActorProxy->UpdateVTKObjects();
  this->PropertyProxy->UpdateVTKObjects();
  this->MapperProxy->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::SetScalarVisibility(int vis)
{
  vtkClientServerStream stream;

  int numObjects = this->GetNumberOfIDs();
  
  for (int i=0; i<numObjects; i++)
    {
    stream << vtkClientServerStream::Invoke << this->MapperProxy->GetID(i)
           << "SetScalarVisibility" << vis << vtkClientServerStream::End;
    }

  vtkClientServerID propertyID = this->PropertyProxy->GetID(0);
  if (vis)
    {
    // Turn off the specular so it does not interfere with data.
    stream << vtkClientServerStream::Invoke 
           << propertyID << "SetSpecular" << 0.0
           << vtkClientServerStream::End;
    }
  else
    {
    stream << vtkClientServerStream::Invoke 
           << propertyID << "SetSpecular" << 0.1 
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << propertyID << "SetSpecularPower" << 100.0 
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << propertyID << "SetSpecularColor" << 1.0 << 1.0 << 1.0 
           << vtkClientServerStream::End;
    }

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  stream.Print(cout);

}

//----------------------------------------------------------------------------
void vtkSMDisplayerProxy::SetRepresentation(int repr)
{
  switch (repr)
    {
    case VTK_POINTS:
      this->DrawPoints();
      break;
    case VTK_WIREFRAME:
      this->DrawWireframe();
      break;
    case VTK_SURFACE:
      this->DrawSurface();
      break;
    default:
      vtkErrorMacro("Representation: " << repr << " is not supported");
    }
}

//----------------------------------------------------------------------------
void vtkSMDisplayerProxy::DrawWireframe()
{
  vtkClientServerStream stream;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetAmbient" << 1 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetDiffuse" << 0 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetRepresentationToWireframe" 
    << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  stream.Print(cout);
}

//----------------------------------------------------------------------------
void vtkSMDisplayerProxy::DrawPoints()
{
  vtkClientServerStream stream;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetAmbient" << 1 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetDiffuse" << 0 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetRepresentationToPoints" 
    << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
}

//----------------------------------------------------------------------------
void vtkSMDisplayerProxy::DrawSurface()
{
  vtkClientServerStream stream;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetAmbient" << 0 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetDiffuse" << 1 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << this->PropertyProxy->GetID(0) << "SetRepresentationToSurface" 
    << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  
  vtkClientServerStream str;

  this->CreateParts();

  // Create the mapper proxy connect it to the geometry filter
  this->MapperProxy->SetVTKClassName("vtkPolyDataMapper");
  this->MapperProxy->CreateVTKObjects(numObjects);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for (int i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
        << this->MapperProxy->GetID(i)
        << "SetInput"
        << this->GetPart(i)->GetVTKDataID()
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << pm->GetProcessModuleID()
        << "GetPartitionId"
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << this->MapperProxy->GetID(i)
        << "SetPiece"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << pm->GetProcessModuleID()
        << "GetNumberOfPartitions"
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << this->MapperProxy->GetID(i)
        << "SetNumberOfPieces"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
    }

  // Create the actor proxy and connect it to the mapper proxy (this)
  this->ActorProxy->SetVTKClassName("vtkActor");
  this->ActorProxy->CreateVTKObjects(numObjects);
  for (int i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
        << this->ActorProxy->GetID(i)
        << "SetMapper"
        << this->MapperProxy->GetID(i)
        << vtkClientServerStream::End;
    }

  // Create the property proxy and connect it to the actor proxy
  this->PropertyProxy->SetVTKClassName("vtkProperty");
  this->PropertyProxy->CreateVTKObjects(1);
  for (int i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
        << this->ActorProxy->GetID(i) 
        << "SetProperty" 
        << this->PropertyProxy->GetID(0)
        << vtkClientServerStream::End;
    }

  for (int i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
        << this->MapperProxy->GetID(i) 
        << "UseLookupTableScalarRangeOn" 
        << vtkClientServerStream::End;
    }

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&str, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  str.Print(cout);

}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

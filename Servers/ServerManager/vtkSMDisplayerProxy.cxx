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
vtkCxxRevisionMacro(vtkSMDisplayerProxy, "1.8");

//---------------------------------------------------------------------------
vtkSMDisplayerProxy::vtkSMDisplayerProxy()
{
  vtkSMIntVectorProperty* intVec;
  vtkSMDoubleVectorProperty* doubleVec;

  double ones[3] = {1, 1, 1};

  // This property actually invokes a method on this (as opposed
  // to the VTK object on the server). Note that an observer is
  // added but doUpdate is disabled. The update is done manually
  // in UpdateVTKObjects() using PushProperty()
  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetScalarVisibility");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 0);
  this->AddProperty("ScalarVisibility", intVec, 1, 0);
  intVec->Delete();

  doubleVec = vtkSMDoubleVectorProperty::New();
  doubleVec->SetCommand("SetColor");
  doubleVec->SetNumberOfElements(3);
  doubleVec->SetElements(ones);
  this->AddProperty("Color", doubleVec, 1, 0);
  doubleVec->Delete();

  intVec = vtkSMIntVectorProperty::New();
  intVec->SetCommand("SetRepresentation");
  intVec->SetNumberOfElements(1);
  intVec->SetElement(0, 2);
  this->AddProperty("Representation", intVec, 1, 0);
  intVec->Delete();
}

//---------------------------------------------------------------------------
vtkSMDisplayerProxy::~vtkSMDisplayerProxy()
{
}

//---------------------------------------------------------------------------
// We overwrite this method to make sure that it is forwarded to the
// sub-proxies as well.
void vtkSMDisplayerProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  // Make these property push their values on this object.
  // This is a nice way for implementing more complicated functionality
  // than properties can handle.  
  this->PushProperty("ScalarVisibility", this->SelfID, 0);
  this->SetPropertyModifiedFlag("ScalarVisibility", 0);
  
  this->PushProperty("Representation", this->SelfID, 0);
  this->SetPropertyModifiedFlag("Representation", 0);

  this->PushProperty("Color", this->SelfID, 0);
  this->SetPropertyModifiedFlag("Color", 0);
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::SetColor(double r, double g, double b)
{
  vtkClientServerStream stream;

  vtkSMProxy* propProxy = this->GetSubProxy("property");
  if (!propProxy)
    {
    vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }
  vtkClientServerID propertyID = propProxy->GetID(0);
  stream << vtkClientServerStream::Invoke 
         << propertyID << "SetColor" << r << g << b 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << propertyID << "SetSpecularColor" << 1.0 << 1.0 << 1.0 
         << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
}

//---------------------------------------------------------------------------
// Adjust scalar visibility as well as lighting.
void vtkSMDisplayerProxy::SetScalarVisibility(int vis)
{
  vtkClientServerStream stream;

  int numObjects = this->GetNumberOfIDs();
  
  vtkSMProxy* mapperProxy = this->GetSubProxy("mapper");
  if (!mapperProxy)
    {
    vtkErrorMacro("No mapper sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  for (int i=0; i<numObjects; i++)
    {
    stream << vtkClientServerStream::Invoke << mapperProxy->GetID(i)
           << "SetScalarVisibility" << vis << vtkClientServerStream::End;
    }

  vtkSMProxy* propertyProxy = this->GetSubProxy("property");
  if (!propertyProxy)
    {
    vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkClientServerID propertyID = propertyProxy->GetID(0);
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
}

//----------------------------------------------------------------------------
// Adjust representation as well as lighting.
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
// Adjust representation as well as lighting.
void vtkSMDisplayerProxy::DrawWireframe()
{
  vtkSMProxy* propertyProxy = this->GetSubProxy("property");
  if (!propertyProxy)
    {
    vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkClientServerStream stream;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetAmbient" << 1 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetDiffuse" << 0 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetRepresentationToWireframe" 
    << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
}

//----------------------------------------------------------------------------
// Adjust representation as well as lighting.
void vtkSMDisplayerProxy::DrawPoints()
{
  vtkSMProxy* propertyProxy = this->GetSubProxy("property");
  if (!propertyProxy)
    {
    vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkClientServerStream stream;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetAmbient" << 1 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetDiffuse" << 0 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetRepresentationToPoints" 
    << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
}

//----------------------------------------------------------------------------
// Adjust representation as well as lighting.
void vtkSMDisplayerProxy::DrawSurface()
{
  vtkSMProxy* propertyProxy = this->GetSubProxy("property");
  if (!propertyProxy)
    {
    vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkClientServerStream stream;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetAmbient" << 0 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetDiffuse" << 1 
    << vtkClientServerStream::End;
  stream 
    << vtkClientServerStream::Invoke 
    << propertyProxy->GetID(0) << "SetRepresentationToSurface" 
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

  int i;

  vtkSMProxy* mapperProxy = this->GetSubProxy("mapper");
  if (!mapperProxy)
    {
    vtkErrorMacro("No mapper sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << mapperProxy->GetID(i)
          << "SetInput"
          << this->GetPart(i)->GetID(0)
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << pm->GetProcessModuleID()
          << "GetPartitionId"
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << mapperProxy->GetID(i)
          << "SetPiece"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << pm->GetProcessModuleID()
          << "GetNumberOfPartitions"
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << mapperProxy->GetID(i)
          << "SetNumberOfPieces"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
      }
    }

  vtkSMProxy* actorProxy = this->GetSubProxy("actor");
  if (!actorProxy)
    {
    vtkErrorMacro("No actor sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << actorProxy->GetID(i)
          << "SetMapper"
          << mapperProxy->GetID(i)
          << vtkClientServerStream::End;

      str << vtkClientServerStream::Invoke 
          << mapperProxy->GetID(i) 
          << "UseLookupTableScalarRangeOn" 
          << vtkClientServerStream::End;
      }

    }

  vtkSMProxy* propertyProxy = this->GetSubProxy("property");
  if (!propertyProxy)
    {
    vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << actorProxy->GetID(i) 
          << "SetProperty" 
          << propertyProxy->GetID(0)
          << vtkClientServerStream::End;
      }
    }

  if (str.GetNumberOfMessages() > 0)
    {
    vtkSMCommunicationModule* cm = this->GetCommunicationModule();
    cm->SendStreamToServers(&str, 
                            this->GetNumberOfServerIDs(),
                            this->GetServerIDs());
    }
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

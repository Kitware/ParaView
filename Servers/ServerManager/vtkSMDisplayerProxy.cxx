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
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMDisplayWindowProxy.h"

vtkStandardNewMacro(vtkSMDisplayerProxy);
vtkCxxRevisionMacro(vtkSMDisplayerProxy, "1.15");

//---------------------------------------------------------------------------
vtkSMDisplayerProxy::vtkSMDisplayerProxy()
{
}

//---------------------------------------------------------------------------
vtkSMDisplayerProxy::~vtkSMDisplayerProxy()
{
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
  if ( propProxy->GetNumberOfIDs() < 1 )
    {
    vtkWarningMacro("The property sub-proxy contains no ids. The displayer "
                    "must have been connected wrong.");
    return;
    }
  vtkClientServerID propertyID = propProxy->GetID(0);
  stream << vtkClientServerStream::Invoke 
         << propertyID << "SetColor" << r << g << b 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << propertyID << "SetSpecularColor" << 1.0 << 1.0 << 1.0 
         << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, stream, 0);
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

  if ( propertyProxy->GetNumberOfIDs() < 1 )
    {
    vtkWarningMacro("The property sub-proxy contains no ids. The displayer "
                    "must have been connected wrong.");
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

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, stream, 0);
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

  if ( propertyProxy->GetNumberOfIDs() < 1 )
    {
    vtkWarningMacro("The property sub-proxy contains no ids. The displayer "
                    "must have been connected wrong.");
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

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, stream, 0);
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

  if ( propertyProxy->GetNumberOfIDs() < 1 )
    {
    vtkWarningMacro("The property sub-proxy contains no ids. The displayer "
                    "must have been connected wrong.");
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

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, stream, 0);
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

  if ( propertyProxy->GetNumberOfIDs() < 1 )
    {
    vtkWarningMacro("The property sub-proxy contains no ids. The displayer "
                    "must have been connected wrong.");
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

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, stream, 0);
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

  //this->CreateParts();

  int i;

  vtkSMProxy* mapperProxy = this->GetSubProxy("mapper");
  if (!mapperProxy)
    {
    //vtkErrorMacro("No mapper sub-proxy was defined. Please make sure that "
    //              "the configuration file defines it.");
    }
  else
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke
          << this->GetID(i)
          << "GetOutput" << 0
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << mapperProxy->GetID(i)
          << "SetInput"
          << vtkClientServerStream::LastResult
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
    //vtkErrorMacro("No actor sub-proxy was defined. Please make sure that "
    //              "the configuration file defines it.");
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
    //vtkErrorMacro("No property sub-proxy was defined. Please make sure that "
    //              "the configuration file defines it.");
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
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendStream(this->Servers, str, 0);
    }
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::AddToDisplayWindow(vtkSMDisplayWindowProxy* dw)
{
  vtkSMProxy* actorProxy = this->GetSubProxy("actor");
  if (!actorProxy)
    {
    vtkErrorMacro("No actor sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }
  
  vtkSMProxy* rendererProxy = dw->GetRendererProxy();
  if (!rendererProxy)
    {
    vtkErrorMacro("No renderer sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }
  vtkClientServerStream str;
  int numActors = actorProxy->GetNumberOfIDs();
  for (int i=0; i<numActors; i++)
    {
    str << vtkClientServerStream::Invoke 
        << rendererProxy->GetID(0) 
        << "AddActor" 
        << actorProxy->GetID(i)
        << vtkClientServerStream::End;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->Servers, str, 0);
  str.Reset();
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::AddInput(vtkSMSourceProxy *input, 
                                   const char* method, 
                                   int portIdx, 
                                   int hasMultipleInputs)
{
  if (!input)
    {
    return;
    }
  if (hasMultipleInputs)
    {
    vtkErrorMacro("Displayer should not have MultipleInputs");
    }
  input->CreateParts();
  int numInputs = input->GetNumberOfParts();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  this->CreateVTKObjects(numInputs);
  int numSources = this->GetNumberOfIDs();
  for (int sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    vtkClientServerID sourceID = this->GetID(sourceIdx);
    // This is to handle the case when there are multiple
    // inputs and the first one has multiple parts. For
    // example, in the Glyph filter, when the input has multiple
    // parts, the glyph source has to be applied to each.
    // NOTE: Make sure that you set the input which has as
    // many parts as there will be filters first. OR call
    // CreateVTKObjects() with the right number of inputs.
    int partIdx = sourceIdx % numInputs;
    vtkSMPart* part = input->GetPart(partIdx);
    stream << vtkClientServerStream::Invoke 
           << sourceID << method;
    if (portIdx >= 0)
      {
      stream << portIdx;
      stream << part->GetID(1);
      }
    else
      {
      stream << part->GetID(0);
      }
    stream << vtkClientServerStream::End;
    }
  pm->SendStream(this->Servers, stream, 0);
}

//---------------------------------------------------------------------------
void vtkSMDisplayerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

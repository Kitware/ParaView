/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarActorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMScalarBarActorProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyIterator.h"

vtkStandardNewMacro(vtkSMScalarBarActorProxy);
vtkCxxRevisionMacro(vtkSMScalarBarActorProxy, "1.1.2.6");
//-----------------------------------------------------------------------------
vtkSMScalarBarActorProxy::vtkSMScalarBarActorProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMScalarBarActorProxy::~vtkSMScalarBarActorProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(numObjects);

  vtkSMProxy* labelTextProperty = this->GetSubProxy("LabelTextProperty");
  vtkSMProxy* titleTextProperty = this->GetSubProxy("TitleTextProperty");
  
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(this->GetProperty("TitleTextProperty"));
  if (pp)
    {
    pp->AddProxy(titleTextProperty);
    }
  else
    {
    vtkErrorMacro("Failed to find property TitleTextProperty.");
    }

  pp = vtkSMProxyProperty::SafeDownCast(this->GetProperty("LabelTextProperty"));
  if (pp)
    {
    pp->AddProxy(labelTextProperty);
    }
  else
    {
    vtkErrorMacro("Failed to find property LabelTextProperty.");
    }

  // Let's set bold/shadow/italic on the text properties.

  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    labelTextProperty->GetProperty("Bold"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    labelTextProperty->GetProperty("Shadow"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    labelTextProperty->GetProperty("Italic"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    titleTextProperty->GetProperty("Bold"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    titleTextProperty->GetProperty("Shadow"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    titleTextProperty->GetProperty("Italic"));
  ivp->SetElement(0, 1);

  // We set the position only if the properties
  // are not already modified.
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  //dvp->SetElements2(0.87, 0.25);
  dvp->Modified();

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position2"));
  //dvp->SetElements2(0.13, 0.5);
  dvp->Modified();
  this->UpdateVTKObjects();
}


//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SaveTextPropertiesInBatchScript(ofstream* file)
{
  vtkSMProxy* label = this->GetSubProxy("LabelTextProperty");
  vtkSMProxy* title = this->GetSubProxy("TitleTextProperty");
  this->SaveTextPropertiesInBatchScript(file, label);
  this->SaveTextPropertiesInBatchScript(file, title);
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SaveTextPropertiesInBatchScript(ofstream* file,
  vtkSMProxy* label)
{
  *file << "set pvTemp" << label->GetID(0)
    << " [$proxyManager NewProxy " << label->GetXMLGroup() << " "
    << label->GetXMLName() << "]" << endl;
  *file << "  $proxyManager RegisterProxy " << label->GetXMLGroup()
    << " pvTemp" << label->GetID(0) << " $pvTemp" << label->GetID(0)
    << endl;
  *file << "  $pvTemp" << label->GetID(0) << " UnRegister {}" << endl;
  
  vtkSMPropertyIterator* iter = label->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* p = iter->GetProperty();

    if (!p->GetSaveable())
      {
      *file << "  # skipping not-saveable property " << p->GetXMLName() << endl;
      continue;
      }

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(p);
    vtkSMDoubleVectorProperty* dvp = 
      vtkSMDoubleVectorProperty::SafeDownCast(p);
    if (ivp)
      {
      for (unsigned int i=0; i < ivp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << label->GetID(0) << " GetProperty "
          << ivp->GetXMLName() << "] SetElement "
          << i << " " << ivp->GetElement(i) 
          << endl;
        }
      }
    else if (dvp)
      {
      for (unsigned int i=0; i < dvp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << label->GetID(0) << " GetProperty "
          << dvp->GetXMLName() << "] SetElement "
          << i << " " << dvp->GetElement(i) 
          << endl;
        }
      }
    else
      {
      *file << "  # skipping property " << p->GetXMLName() << endl;
      }
    }

  iter->Delete();
  *file << "  $pvTemp" << label->GetID(0) << " UpdateVTKObjects" << endl;
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SetPosition(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->GetNumberOfIDs(); i++)
    {
    vtkClientServerID id = this->GetID(i);
    stream << vtkClientServerStream::Invoke
      << id 
      << "GetPositionCoordinate"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetValue" << x  << y
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SetPosition2(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->GetNumberOfIDs(); i++)
    {
    vtkClientServerID id = this->GetID(i);
    stream << vtkClientServerStream::Invoke
      << id 
      << "GetPosition2Coordinate"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetValue" << x  << y
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

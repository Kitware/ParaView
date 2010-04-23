/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextSourceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTextSourceRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTextWidgetRepresentationProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkSMPropertyHelper.h"
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMTextSourceRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::vtkSMTextSourceRepresentationProxy()
{
  this->TextWidgetProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::~vtkSMTextSourceRepresentationProxy()
{
  this->TextWidgetProxy = 0;
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  this->CreateVTKObjects();
  if (!this->TextWidgetProxy->AddToView(view))
    {
    return false;
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  if (!this->TextWidgetProxy->RemoveFromView(view))
    {
    return false;
    }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->TextWidgetProxy = vtkSMTextWidgetRepresentationProxy::SafeDownCast(
    this->GetSubProxy("TextWidgetRepresentation"));
  if (!this->TextWidgetProxy)
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  // We want to deliver the text from the 1st node alone.
  this->SetReductionType(FIRST_NODE_ONLY);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::SetVisibility(int visible)
{
  this->CreateVTKObjects();
  vtkSMPropertyHelper(this->TextWidgetProxy, "Enabled").Set(visible);
  vtkSMPropertyHelper(this->TextWidgetProxy, "Visibility").Set(visible);
  this->TextWidgetProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  this->Superclass::Update(view);

  vtkTable* data = vtkTable::SafeDownCast(this->GetOutput());
  vtkstd::string text = "";
  if (data->GetNumberOfRows() > 0 && data->GetNumberOfColumns() > 0)
    {
    text = data->GetValue(0, 0).ToString();
    }

  // Now get the text from the Input and set it on the text widget display.
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->TextWidgetProxy->GetProperty("Text"));
  svp->SetElement(0, text.c_str());
  this->TextWidgetProxy->UpdateProperty("Text");

 // this->InvokeEvent(vtkSMViewProxy::ForceUpdateEvent);
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



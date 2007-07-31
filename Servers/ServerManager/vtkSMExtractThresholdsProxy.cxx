/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractThresholdsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractThresholdsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkSMExtractThresholdsProxy);
vtkCxxRevisionMacro(vtkSMExtractThresholdsProxy, "1.3");
//-----------------------------------------------------------------------------
vtkSMExtractThresholdsProxy::vtkSMExtractThresholdsProxy()
{
this->ArrayName = NULL;
}

//-----------------------------------------------------------------------------
vtkSMExtractThresholdsProxy::~vtkSMExtractThresholdsProxy()
{
if (this->ArrayName)
  {
  delete[] this->ArrayName;
  }
}

//-----------------------------------------------------------------------------
void vtkSMExtractThresholdsProxy::SetScalarArray(int, int, int, int field, const char *name)
{
  this->Field = field;
  //cerr << "field = " << field << endl;
  if (this->ArrayName)
    {
    delete[] this->ArrayName;
    }
  int len = strlen(name)+1;
  this->ArrayName = new char[len];
  strcpy(this->ArrayName, name);
  //cerr << "ArrayName = " << this->ArrayName << endl;
}

//-----------------------------------------------------------------------------
void vtkSMExtractThresholdsProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMSourceProxy* selectionSource = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("SelectionSource"));
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }
  
  this->AddInput(selectionSource, "SetSelectionConnection");
}

//-----------------------------------------------------------------------------
void vtkSMExtractThresholdsProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, vtkSelection::POINT);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  ivp->SetElement(0, vtkSelection::THRESHOLDS);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, this->Field);

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ArrayName"));
  if (this->ArrayName && strlen(this->ArrayName))
    {
    svp->SetElement(0, this->ArrayName);
    }

  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMExtractThresholdsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

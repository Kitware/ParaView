/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayRangeDomain.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMArrayRangeDomain);

//---------------------------------------------------------------------------
vtkSMArrayRangeDomain::vtkSMArrayRangeDomain()
{
}

//---------------------------------------------------------------------------
vtkSMArrayRangeDomain::~vtkSMArrayRangeDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(vtkSMProperty*)
{
  // Find the array whose range we are interested in and then find the producer
  // who is producing the data with the array of interest.

  vtkSMProperty* propForInput = this->GetRequiredProperty("Input");
  vtkSMProperty* propForArraySelection = this->GetRequiredProperty("ArraySelection");
  if (!propForInput || !propForArraySelection)
  {
    return;
  }

  vtkSMUncheckedPropertyHelper inputHelper(propForInput);
  vtkSMUncheckedPropertyHelper arraySelectionHelper(propForArraySelection);
  if (arraySelectionHelper.GetNumberOfElements() < 5)
  {
    return;
  }

  const char* arrayName = arraySelectionHelper.GetAsString(4);
  if (!arrayName || arrayName[0] == '\0')
  {
    return;
  }
  if (vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy()) != NULL)
  {
    this->Update(arrayName, arraySelectionHelper.GetAsInt(3),
      vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy()), inputHelper.GetOutputPort());
  }
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(
  const char* arrayName, int fieldAssociation, vtkSMSourceProxy* producer, int producerPort)
{
  assert(producer != NULL && arrayName != NULL);

  // Make sure the outputs are created.
  producer->CreateOutputPorts();

  vtkPVDataInformation* info = producer->GetDataInformation(producerPort);
  if (!info)
  {
    return;
  }

  vtkPVArrayInformation* arrayInfo = info->GetArrayInformation(arrayName, fieldAssociation);

  int component_number = VTK_INT_MAX;
  if (arrayInfo == NULL &&
    (fieldAssociation == vtkDataObject::POINT || fieldAssociation == vtkDataObject::CELL))
  {
    // it's possible that the array name was mangled due to "auto-conversion" of
    // field arrays.

    // first try to see if there's a component suffix added and hence we failed
    // to locate the array information.
    std::string name = vtkSMArrayListDomain::ArrayNameFromMangledName(arrayName);
    arrayInfo =
      name.length() > 0 ? info->GetArrayInformation(name.c_str(), fieldAssociation) : NULL;
    if (!arrayInfo)
    {
      // try the other field association.
      int otherField =
        fieldAssociation == vtkDataObject::POINT ? vtkDataObject::CELL : vtkDataObject::POINT;

      arrayInfo = info->GetArrayInformation(arrayName, otherField);
      arrayInfo = arrayInfo ? arrayInfo : info->GetArrayInformation(name.c_str(), otherField);
    }

    // Now, extract component information. If name == arrayName, it means we
    // don't have any component information in the array name itself.
    if (arrayInfo && (name != arrayName))
    {
      int mangledCompNo = vtkSMArrayListDomain::ComponentIndexFromMangledName(arrayInfo, arrayName);
      // ComponentIndexFromMangledName returns -1 on error and num_of_component
      // for magnitude. Which is not what vtkPVArrayInformation expects, to
      // convert that.
      if (mangledCompNo == -1)
      {
        return;
      }
      if (mangledCompNo >= arrayInfo->GetNumberOfComponents())
      {
        component_number = -1;
      }
      else
      {
        component_number = mangledCompNo;
      }
    }
  }

  if (!arrayInfo)
  {
    std::vector<vtkEntry> values;
    this->SetEntries(values);
  }
  else if (component_number < arrayInfo->GetNumberOfComponents())
  {
    // a particular component was chosen, add ranges for that.
    std::vector<vtkEntry> values(1);
    values[0].Valid[0] = values[0].Valid[1] = true;
    values[0].Value = vtkTuple<double, 2>(arrayInfo->GetComponentRange(component_number));
    this->SetEntries(values);
  }
  else
  {
    std::vector<vtkEntry> values(arrayInfo->GetNumberOfComponents());
    for (int cc = 0; cc < arrayInfo->GetNumberOfComponents(); cc++)
    {
      values[cc].Valid[0] = values[cc].Valid[1] = true;
      values[cc].Value = vtkTuple<double, 2>(arrayInfo->GetComponentRange(cc));
    }
    if (arrayInfo->GetNumberOfComponents() > 1)
    {
      // add vector magnitude.
      vtkEntry entry;
      entry.Valid[0] = entry.Valid[1] = true;
      entry.Value = vtkTuple<double, 2>(arrayInfo->GetComponentRange(-1));
      values.push_back(entry);
    }
    this->SetEntries(values);
  }
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

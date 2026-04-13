// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPrismThresholdRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkStringScanner.h"

#include <sstream>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPrismThresholdRangeDomain);

//------------------------------------------------------------------------------
vtkSMPrismThresholdRangeDomain::vtkSMPrismThresholdRangeDomain() = default;

//------------------------------------------------------------------------------
vtkSMPrismThresholdRangeDomain::~vtkSMPrismThresholdRangeDomain() = default;

//------------------------------------------------------------------------------
void vtkSMPrismThresholdRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AxisId: " << this->AxisId << endl;
}

//------------------------------------------------------------------------------
int vtkSMPrismThresholdRangeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  if (const char* axis_id = element->GetAttribute("axis_id"))
  {
    if (auto result = vtk::scan_int<int>(axis_id))
    {
      this->AxisId = result->value();
    }
    else
    {
      vtkErrorMacro("Invalid axis_id attribute: " << axis_id);
      return 0;
    }
  }
  else
  {
    vtkErrorMacro("Missing axis_id attribute.");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkSMPrismThresholdRangeDomain::Update(vtkSMProperty*)
{
  auto bounds = vtkSMDoubleVectorProperty::SafeDownCast(this->GetRequiredProperty("Bounds"));

  if (!bounds)
  {
    vtkErrorMacro("Missing required properties.");
    return;
  }

  vtkSMUncheckedPropertyHelper boundsHelper(bounds);

  std::vector<vtkEntry> values;
  switch (this->AxisId)
  {
    case 0:
      values.emplace_back(boundsHelper.GetAsDouble(0), boundsHelper.GetAsDouble(1));
      break;
    case 1:
      values.emplace_back(boundsHelper.GetAsDouble(2), boundsHelper.GetAsDouble(3));
      break;
    case 2:
      values.emplace_back(boundsHelper.GetAsDouble(4), boundsHelper.GetAsDouble(5));
      break;
    default:
      vtkErrorMacro("Invalid axis id: " << this->AxisId);
      return;
  }
  this->SetEntries(values);
}

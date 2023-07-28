// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPrismThresholdRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

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

//=============================================================================
namespace
{
template <typename T>
T lexical_cast(const std::string& s)
{
  std::stringstream ss(s);

  T result;
  if ((ss >> result).fail() || !(ss >> std::ws).eof())
  {
    throw std::bad_cast();
  }

  return result;
}
}

//------------------------------------------------------------------------------
int vtkSMPrismThresholdRangeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* axis_id = element->GetAttribute("axis_id");
  if (axis_id)
  {
    try
    {
      this->AxisId = lexical_cast<int>(axis_id);
    }
    catch (const std::bad_cast&)
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
      values.push_back(vtkEntry(boundsHelper.GetAsDouble(0), boundsHelper.GetAsDouble(1)));
      break;
    case 1:
      values.push_back(vtkEntry(boundsHelper.GetAsDouble(2), boundsHelper.GetAsDouble(3)));
      break;
    case 2:
      values.push_back(vtkEntry(boundsHelper.GetAsDouble(4), boundsHelper.GetAsDouble(5)));
      break;
    default:
      vtkErrorMacro("Invalid axis id: " << this->AxisId);
      return;
  }
  this->SetEntries(values);
}

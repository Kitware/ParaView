/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoundsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBoundsDomain.h"

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cmath>

vtkStandardNewMacro(vtkSMBoundsDomain);
//---------------------------------------------------------------------------
vtkSMBoundsDomain::vtkSMBoundsDomain()
{
  this->Mode = vtkSMBoundsDomain::NORMAL;
  this->ScaleFactor = 0.1;
  this->AxisFlags = X_Y_AND_Z_AXES;
  this->ArrayRangeDomain = NULL;
}

//---------------------------------------------------------------------------
vtkSMBoundsDomain::~vtkSMBoundsDomain()
{
  if (this->ArrayRangeDomain != NULL)
  {
    this->ArrayRangeDomain->Delete();
  }
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::Update(vtkSMProperty*)
{
  if (this->Mode == vtkSMBoundsDomain::ORIENTED_MAGNITUDE)
  {
    this->UpdateOriented();
    return;
  }

  auto* flagsInfo = this->GetAxisFlagsInformation();
  if (flagsInfo)
  {
    this->AxisFlags = flagsInfo->GetUncheckedElement(0);
  }
  else
  {
    this->AxisFlags = X_Y_AND_Z_AXES;
  }

  vtkPVDataInformation* info = this->GetInputInformation();
  if (info)
  {
    double bounds[6];
    info->GetBounds(bounds);
    this->SetDomainValues(bounds);
  }
}

//---------------------------------------------------------------------------
vtkPVDataInformation* vtkSMBoundsDomain::GetInputInformation()
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  if (!inputProperty)
  {
    vtkErrorMacro("Missing required property with function 'Input'");
    return NULL;
  }

  vtkSMUncheckedPropertyHelper helper(inputProperty);
  if (helper.GetNumberOfElements() > 0)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
    if (sp)
    {
      return sp->GetDataInformation(helper.GetOutputPort());
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
vtkSMIntVectorProperty* vtkSMBoundsDomain::GetAxisFlagsInformation()
{
  return vtkSMIntVectorProperty::SafeDownCast(this->GetRequiredProperty("AxisFlags"));
}

//---------------------------------------------------------------------------
bool vtkSMBoundsDomain::IsAxisEnabled(int axis)
{
  return (this->AxisFlags & (1 << axis)) != 0;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::UpdateOriented()
{
  vtkPVDataInformation* inputInformation = this->GetInputInformation();
  if (!inputInformation)
  {
    return;
  }

  double bounds[6];
  inputInformation->GetBounds(bounds);

  vtkSMDoubleVectorProperty* normal =
    vtkSMDoubleVectorProperty::SafeDownCast(this->GetRequiredProperty("Normal"));
  vtkSMDoubleVectorProperty* origin =
    vtkSMDoubleVectorProperty::SafeDownCast(this->GetRequiredProperty("Origin"));
  if (normal && origin)
  {
    double points[8][3];

    double xmin = bounds[0];
    double xmax = bounds[1];
    double ymin = bounds[2];
    double ymax = bounds[3];
    double zmin = bounds[4];
    double zmax = bounds[5];

    points[0][0] = xmin;
    points[0][1] = ymin;
    points[0][2] = zmin;
    points[1][0] = xmax;
    points[1][1] = ymax;
    points[1][2] = zmax;
    points[2][0] = xmin;
    points[2][1] = ymin;
    points[2][2] = zmax;
    points[3][0] = xmin;
    points[3][1] = ymax;
    points[3][2] = zmax;
    points[4][0] = xmin;
    points[4][1] = ymax;
    points[4][2] = zmin;
    points[5][0] = xmax;
    points[5][1] = ymax;
    points[5][2] = zmin;
    points[6][0] = xmax;
    points[6][1] = ymin;
    points[6][2] = zmin;
    points[7][0] = xmax;
    points[7][1] = ymin;
    points[7][2] = zmax;

    double normalv[3], originv[3];

    unsigned int i;
    if (normal->GetNumberOfUncheckedElements() > 2 && origin->GetNumberOfUncheckedElements() > 2)
    {
      for (i = 0; i < 3; i++)
      {
        normalv[i] = normal->GetUncheckedElement(i);
        originv[i] = origin->GetUncheckedElement(i);
      }
    }
    else
    {
      return;
    }

    unsigned int j;
    double dist[8];

    for (i = 0; i < 8; i++)
    {
      dist[i] = 0;
      for (j = 0; j < 3; j++)
      {
        dist[i] += (points[i][j] - originv[j]) * normalv[j];
      }
    }

    double min = dist[0], max = dist[0];
    for (i = 1; i < 8; i++)
    {
      if (dist[i] < min)
      {
        min = dist[i];
      }
      if (dist[i] > max)
      {
        max = dist[i];
      }
    }

    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(min, max));
    this->SetEntries(entries);
  }
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::SetDomainValues(double bounds[6])
{
  if (this->Mode == vtkSMBoundsDomain::NORMAL)
  {
    std::vector<vtkEntry> entries;
    for (int j = 0; j < 3; j++)
    {
      if (this->IsAxisEnabled(j))
      {
        entries.push_back(vtkEntry(bounds[2 * j], bounds[2 * j + 1]));
      }
    }
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::DATA_BOUNDS)
  {
    std::vector<vtkEntry> entries;
    for (int j = 0; j < 3; j++)
    {
      if (this->IsAxisEnabled(j))
      {
        entries.push_back(vtkEntry(bounds[2 * j], bounds[2 * j + 1]));
        entries.push_back(vtkEntry(bounds[2 * j], bounds[2 * j + 1]));
      }
    }
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::EXTENTS)
  {
    std::vector<vtkEntry> entries;
    for (int j = 0; j < 3; j++)
    {
      if (this->IsAxisEnabled(j))
      {
        entries.push_back(vtkEntry(0, bounds[2 * j + 1] - bounds[2 * j]));
      }
    }
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::MAGNITUDE)
  {
    // first check if the bounds have valid values before setting them
    if (vtkMath::AreBoundsInitialized(bounds) == 0)
    {
      return;
    }

    double magn = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
    // Never use 0 min/max.
    if (magn == 0)
    {
      magn = 1;
    }
    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(-magn / 2.0, magn / 2.0));
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::SCALED_EXTENT)
  {
    vtkBoundingBox box(bounds);
    double maxbounds = box.GetMaxLength();
    maxbounds *= this->ScaleFactor;
    // Never use 0 maxbounds.
    if (maxbounds == 0)
    {
      maxbounds = this->ScaleFactor;
    }
    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(0, maxbounds));
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::ARRAY_SCALED_EXTENT)
  {
    this->ArrayRangeDomain->Update(NULL);
    double arrayScaleFactor = 0;
    for (unsigned int i = 0; i < this->ArrayRangeDomain->GetNumberOfEntries(); i++)
    {
      arrayScaleFactor +=
        (this->ArrayRangeDomain->GetMaximum(i) - this->ArrayRangeDomain->GetMinimum(i)) / 2;
    }
    if (arrayScaleFactor == 0)
    {
      arrayScaleFactor = 1;
    }
    else
    {
      arrayScaleFactor /= this->ArrayRangeDomain->GetNumberOfEntries();
    }
    vtkBoundingBox box(bounds);
    double maxbounds = box.GetMaxLength();
    maxbounds *= (this->ScaleFactor / arrayScaleFactor);
    // Never use 0 maxbounds.
    if (maxbounds == 0)
    {
      maxbounds = this->ScaleFactor;
    }
    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(0, maxbounds));
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::APPROXIMATE_CELL_LENGTH)
  {
    double diameter = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
    std::vector<vtkEntry> entries;
    entries.push_back(vtkEntry(0, diameter));
    this->SetEntries(entries);
  }
  else if (this->Mode == vtkSMBoundsDomain::COMPONENT_MAGNITUDE)
  {
    if (vtkMath::AreBoundsInitialized(bounds) == 0)
    {
      return;
    }

    std::vector<vtkEntry> entries;
    entries.emplace_back(vtkEntry(0, bounds[1] - bounds[0]));
    entries.emplace_back(vtkEntry(0, bounds[3] - bounds[2]));
    entries.emplace_back(vtkEntry(0, bounds[5] - bounds[4]));
    this->SetEntries(entries);
  }
}

//---------------------------------------------------------------------------
int vtkSMBoundsDomain::SetDefaultValues(vtkSMProperty* property, bool use_unchecked_values)
{
  if (this->Mode == vtkSMBoundsDomain::APPROXIMATE_CELL_LENGTH)
  {
    vtkPVDataInformation* dataInfo = this->GetInputInformation();
    if (dataInfo)
    {
      double bounds[6];
      dataInfo->GetBounds(bounds);
      double unitDistance = 1.0;
      if (vtkMath::AreBoundsInitialized(bounds))
      {
        const double diameter = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
          (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
          (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

        const auto numCells = dataInfo->GetNumberOfCells();
        const double linearNumCells = std::cbrt(numCells);
        unitDistance = diameter;
        if (linearNumCells != 0.0 && !vtkMath::IsNan(linearNumCells) &&
          !vtkMath::IsInf(linearNumCells))
        {
          unitDistance = diameter / linearNumCells;
        }
      }
      vtkSMPropertyHelper helper(property);
      helper.SetUseUnchecked(use_unchecked_values);
      helper.Set(0, unitDistance);
    }
    return 1;
  }
  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//---------------------------------------------------------------------------
int vtkSMBoundsDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  bool has_default_mode = (element->GetAttribute("default_mode") != NULL);
  const char* mode = element->GetAttribute("mode");
  if (mode)
  {
    if (strcmp(mode, "normal") == 0)
    {
      this->Mode = vtkSMBoundsDomain::NORMAL;
    }
    else if (strcmp(mode, "magnitude") == 0)
    {
      this->Mode = vtkSMBoundsDomain::MAGNITUDE;
    }
    else if (strcmp(mode, "oriented_magnitude") == 0)
    {
      this->Mode = vtkSMBoundsDomain::ORIENTED_MAGNITUDE;
    }
    else if (strcmp(mode, "component_magnitude") == 0)
    {
      this->Mode = vtkSMBoundsDomain::COMPONENT_MAGNITUDE;
    }
    else if (strcmp(mode, "scaled_extent") == 0)
    {
      this->Mode = vtkSMBoundsDomain::SCALED_EXTENT;
      if (!has_default_mode)
      {
        this->DefaultDefaultMode = vtkSMDoubleRangeDomain::MAX;
      }
    }
    else if (strcmp(mode, "data_bounds") == 0)
    {
      this->Mode = vtkSMBoundsDomain::DATA_BOUNDS;
      if (!has_default_mode)
      {
        this->DefaultModeVector.resize(6);
        for (int cc = 0; cc < 3; ++cc)
        {
          this->DefaultModeVector[2 * cc] = MIN;
          this->DefaultModeVector[2 * cc + 1] = MAX;
        }
      }
    }
    else if (strcmp(mode, "extents") == 0)
    {
      this->Mode = vtkSMBoundsDomain::EXTENTS;
      if (!has_default_mode)
      {
        this->DefaultDefaultMode = vtkSMDoubleRangeDomain::MAX;
      }
    }
    else if (strcmp(mode, "approximate_cell_length") == 0)
    {
      this->Mode = vtkSMBoundsDomain::APPROXIMATE_CELL_LENGTH;
    }
    else if (strcmp(mode, "array_scaled_extent") == 0)
    {
      this->Mode = vtkSMBoundsDomain::ARRAY_SCALED_EXTENT;
      if (this->ArrayRangeDomain != NULL)
      {
        this->ArrayRangeDomain->Delete();
      }
      this->ArrayRangeDomain = vtkSMArrayRangeDomain::New();
      if (!this->ArrayRangeDomain->ReadXMLAttributes(prop, element))
      {
        return 0;
      }
    }
    else
    {
      vtkErrorMacro("Unrecognized mode: " << mode);
      return 0;
    }
  }

  const char* scalefactor = element->GetAttribute("scale_factor");
  if (scalefactor)
  {
    sscanf(scalefactor, "%lf", &this->ScaleFactor);
  }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  static const char xyz[] = "XYZ";

  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "ScaleFactor: " << this->ScaleFactor << endl;
  os << indent << "AxisFlags: ";
  for (int i = 0; i < 3; ++i)
  {
    os << xyz[i] << ":" << ((this->AxisFlags & (1 << i)) ? "On" : "Off") << " ";
  }
  os << endl;
}

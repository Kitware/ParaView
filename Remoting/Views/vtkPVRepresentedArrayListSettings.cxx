// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVRepresentedArrayListSettings.h"

#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkPVIOSettings.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSession.h"
#include "vtkStringArray.h"

#include <vtksys/RegularExpression.hxx>

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

vtkSmartPointer<vtkPVRepresentedArrayListSettings> vtkPVRepresentedArrayListSettings::Instance;

class vtkPVRepresentedArrayListSettings::vtkInternals
{
public:
  std::vector<std::string> FilterExpressions;
  std::vector<int> ArrayMagnitudeExceptions;

  std::vector<std::string> ChartsDefaultXAxis;
  std::vector<vtksys::RegularExpression> ChartsHiddenAttributes;
};

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings* vtkPVRepresentedArrayListSettings::New()
{
  vtkPVRepresentedArrayListSettings* instance = vtkPVRepresentedArrayListSettings::GetInstance();
  assert(instance);
  instance->Register(nullptr);
  return instance;
}

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings* vtkPVRepresentedArrayListSettings::GetInstance()
{
  if (!vtkPVRepresentedArrayListSettings::Instance)
  {
    vtkPVRepresentedArrayListSettings* instance = new vtkPVRepresentedArrayListSettings();
    instance->InitializeObjectBase();
    vtkPVRepresentedArrayListSettings::Instance.TakeReference(instance);
  }
  return vtkPVRepresentedArrayListSettings::Instance;
}

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings::vtkPVRepresentedArrayListSettings()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPVRepresentedArrayListSettings::~vtkPVRepresentedArrayListSettings()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetNumberOfFilterExpressions(int n)
{
  if (n != this->GetNumberOfFilterExpressions())
  {
    this->Internals->FilterExpressions.resize(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVRepresentedArrayListSettings::GetNumberOfFilterExpressions()
{
  return static_cast<int>(this->Internals->FilterExpressions.size());
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetFilterExpression(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfFilterExpressions())
  {
    if (strcmp(this->Internals->FilterExpressions[i].c_str(), expression) != 0)
    {
      this->Internals->FilterExpressions[i] = expression;
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }
}

//----------------------------------------------------------------------------
const char* vtkPVRepresentedArrayListSettings::GetFilterExpression(int i)
{
  if (i >= 0 && i < this->GetNumberOfFilterExpressions())
  {
    return this->Internals->FilterExpressions[i].c_str();
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetNumberOfArrayMagnitudeExceptions(int size)
{
  if (static_cast<std::size_t>(size) != this->Internals->ArrayMagnitudeExceptions.size())
  {
    this->Internals->ArrayMagnitudeExceptions.resize(size);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetArrayMagnitudeException(int idx, int ncomp)
{
  if (idx >= 0 && static_cast<std::size_t>(idx) < this->Internals->ArrayMagnitudeExceptions.size())
  {
    if (this->Internals->ArrayMagnitudeExceptions[idx] != ncomp)
    {
      this->Internals->ArrayMagnitudeExceptions[idx] = ncomp;
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Index out of range: " << idx);
  }
}

//----------------------------------------------------------------------------
bool vtkPVRepresentedArrayListSettings::ShouldUseMagnitudeMode(int ncomp) const
{
  const auto& exceptions = this->Internals->ArrayMagnitudeExceptions;
  const auto findComp = std::find(exceptions.cbegin(), exceptions.cend(), ncomp);

  return this->ComputeArrayMagnitude != (findComp != exceptions.end());
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetNumberOfChartsDefaultXAxis(int n)
{
  if (n != this->GetNumberOfChartsDefaultXAxis())
  {
    this->Internals->ChartsDefaultXAxis.resize(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVRepresentedArrayListSettings::GetNumberOfChartsDefaultXAxis() const
{
  return static_cast<int>(this->Internals->ChartsDefaultXAxis.size());
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetChartsDefaultXAxis(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfChartsDefaultXAxis())
  {
    if (this->Internals->ChartsDefaultXAxis[i] != expression)
    {
      this->Internals->ChartsDefaultXAxis[i] = expression;
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }
}

//----------------------------------------------------------------------------
const char* vtkPVRepresentedArrayListSettings::GetChartsDefaultXAxis(int i) const
{
  if (i >= 0 && i < this->GetNumberOfChartsDefaultXAxis())
  {
    return this->Internals->ChartsDefaultXAxis[i].c_str();
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }

  return nullptr;
}

//----------------------------------------------------------------------------
const std::vector<std::string>& vtkPVRepresentedArrayListSettings::GetAllChartsDefaultXAxis() const
{
  return this->Internals->ChartsDefaultXAxis;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetNumberOfChartsHiddenAttributes(int n)
{
  if (n != this->GetNumberOfChartsHiddenAttributes())
  {
    this->Internals->ChartsHiddenAttributes.resize(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkPVRepresentedArrayListSettings::GetNumberOfChartsHiddenAttributes() const
{
  return static_cast<int>(this->Internals->ChartsHiddenAttributes.size());
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::SetChartsHiddenAttributes(int i, const char* expression)
{
  if (i >= 0 && i < this->GetNumberOfChartsHiddenAttributes())
  {
    if (this->Internals->ChartsHiddenAttributes[i] != expression)
    {
      this->Internals->ChartsHiddenAttributes[i].compile(expression);
      this->Modified();
    }
  }
  else
  {
    vtkErrorMacro("Index out of range: " << i);
  }
}

//----------------------------------------------------------------------------
const std::vector<vtksys::RegularExpression>&
vtkPVRepresentedArrayListSettings::GetAllChartsHiddenAttributes() const
{
  return this->Internals->ChartsHiddenAttributes;
}

//----------------------------------------------------------------------------
bool vtkPVRepresentedArrayListSettings::GetSeriesVisibilityDefault(const char* name) const
{
  for (auto& regex : this->Internals->ChartsHiddenAttributes)
  {
    if (regex.is_valid() && regex.find(name))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkPVRepresentedArrayListSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

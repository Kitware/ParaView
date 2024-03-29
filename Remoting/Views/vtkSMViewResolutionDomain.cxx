// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMViewResolutionDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMViewResolutionDomain);
//----------------------------------------------------------------------------
vtkSMViewResolutionDomain::vtkSMViewResolutionDomain() = default;

//----------------------------------------------------------------------------
vtkSMViewResolutionDomain::~vtkSMViewResolutionDomain() = default;

//----------------------------------------------------------------------------
void vtkSMViewResolutionDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* useLayoutProperty = this->GetRequiredProperty("UseLayout");
  vtkSMProperty* layoutProperty = this->GetRequiredProperty("Layout");
  vtkSMProperty* viewProperty = this->GetRequiredProperty("View");

  int resolution[2] = { 0, 0 };
  if (useLayoutProperty && vtkSMUncheckedPropertyHelper(useLayoutProperty).GetAsInt() != 0)
  {
    this->GetLayoutResolution(vtkSMViewLayoutProxy::SafeDownCast(
                                vtkSMUncheckedPropertyHelper(layoutProperty).GetAsProxy(0)),
      resolution);
  }
  else if (viewProperty)
  {
    this->GetViewResolution(
      vtkSMViewProxy::SafeDownCast(vtkSMUncheckedPropertyHelper(viewProperty).GetAsProxy(0)),
      resolution);
  }

  if (resolution[0] != 0 && resolution[1] != 0)
  {
    std::vector<vtkEntry> values;
    values.push_back(vtkEntry(0, resolution[0]));
    values.push_back(vtkEntry(0, resolution[1]));
    this->SetEntries(values);
  }
}

//----------------------------------------------------------------------------
void vtkSMViewResolutionDomain::GetLayoutResolution(vtkSMViewLayoutProxy* layout, int resolution[2])
{
  if (layout)
  {
    // for layout size, prefer preview mode size, if non-empty.
    vtkSMUncheckedPropertyHelper(layout, "PreviewMode").Get(resolution, 2);
    if (resolution[0] == 0 || resolution[1] == 0)
    {
      auto size = layout->GetSize();
      resolution[0] = size[0];
      resolution[1] = size[1];
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMViewResolutionDomain::GetViewResolution(vtkSMViewProxy* view, int resolution[2])
{
  if (view)
  {
    vtkSMUncheckedPropertyHelper helper(view, "ViewSize");
    if (helper.GetNumberOfElements() == 2)
    {
      helper.Get(resolution, 2);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMViewResolutionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

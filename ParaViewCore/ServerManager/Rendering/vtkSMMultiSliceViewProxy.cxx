/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiSliceViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiSliceViewProxy.h"

#include "vtkBoundingBox.h"
#include "vtkClientServerStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVMultiSliceView.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <cassert>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkSMMultiSliceViewProxy);
//----------------------------------------------------------------------------
vtkSMMultiSliceViewProxy::vtkSMMultiSliceViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMMultiSliceViewProxy::~vtkSMMultiSliceViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMMultiSliceViewProxy::GetDataBounds(double bounds[6])
{
  vtkPVMultiSliceView* view = vtkPVMultiSliceView::SafeDownCast(this->GetClientSideObject());
  if (view)
  {
    view->GetDataBounds(bounds);
  }
}

//----------------------------------------------------------------------------
const char* vtkSMMultiSliceViewProxy::GetRepresentationType(
  vtkSMSourceProxy* producer, int outputPort)
{
  if (!producer)
  {
    return 0;
  }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype =
    pxm->GetPrototypeProxy("representations", "CompositeMultiSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(producer, outputPort);
  bool sg = (pp->IsInDomains() > 0);
  pp->RemoveAllUncheckedProxies();
  return sg ? "CompositeMultiSliceRepresentation"
            : this->Superclass::GetRepresentationType(producer, outputPort);
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMMultiSliceViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* proxy, int outputPort)
{
  vtkSMRepresentationProxy* repr = this->Superclass::CreateDefaultRepresentation(proxy, outputPort);
  if (repr && strcmp(repr->GetXMLName(), "CompositeMultiSliceRepresentation") == 0)
  {
    this->InitDefaultSlices(vtkSMSourceProxy::SafeDownCast(proxy), outputPort, repr);
  }

  return repr;
}

//-----------------------------------------------------------------------------
// HACK: method to force representation type to "Slices".
void vtkSMMultiSliceViewProxy::ForceRepresentationType(vtkSMProxy* repr, const char* type)
{
  // HACK: to set default representation type to Slices.
  vtkSMPropertyHelper reprProp(repr, "Representation");
  reprProp.Set(type);
  vtkNew<vtkPVXMLElement> nodefault;
  nodefault->SetName("NoDefault");
  if (vtkPVXMLElement* hints = repr->GetProperty("Representation")->GetHints())
  {
    hints->AddNestedElement(nodefault.GetPointer());
  }
  else
  {
    vtkNew<vtkPVXMLElement> hints2;
    hints2->SetName("Hints");
    hints2->AddNestedElement(nodefault.GetPointer());
    repr->GetProperty("Representation")->SetHints(hints2.GetPointer());
  }
}

//-----------------------------------------------------------------------------
// HACK: Get representation's input data bounds.
bool vtkSMMultiSliceViewProxy::GetDataBounds(vtkSMSourceProxy* source, int opport, double bounds[6])
{
  vtkPVDataInformation* info = source ? source->GetDataInformation(opport) : NULL;
  if (info)
  {
    if (vtkPVArrayInformation* ainfo =
          info->GetFieldDataInformation()->GetArrayInformation("BoundingBoxInModelCoordinates"))
    {
      // use "original basis" bounds,  if present.
      if (ainfo->GetNumberOfTuples() == 1 && ainfo->GetNumberOfComponents() == 6)
      {
        for (int cc = 0; cc < 6; cc++)
        {
          bounds[cc] = ainfo->GetComponentRange(cc)[0];
        }
        return true;
      }
    }
    info->GetBounds(bounds);
    return (vtkBoundingBox::IsValid(bounds) != 0);
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkSMMultiSliceViewProxy::InitDefaultSlices(
  vtkSMSourceProxy* source, int opport, vtkSMRepresentationProxy* repr)
{
  if (!source)
  {
    return;
  }
  vtkSMMultiSliceViewProxy::ForceRepresentationType(repr, "Slices");

  double bounds[6];
  if (vtkSMMultiSliceViewProxy::GetDataBounds(source, opport, bounds))
  {
    vtkBoundingBox bbox(bounds);
    double center[3];
    bbox.GetCenter(center);

    // Add orthogonal X,Y,Z slices based on center position.
    std::vector<double> xSlices = vtkSMPropertyHelper(this, "XSlicesValues").GetDoubleArray();
    std::vector<double> ySlices = vtkSMPropertyHelper(this, "YSlicesValues").GetDoubleArray();
    std::vector<double> zSlices = vtkSMPropertyHelper(this, "ZSlicesValues").GetDoubleArray();

    if (xSlices.size() == 0 && ySlices.size() == 0 && zSlices.size() == 0)
    {
      xSlices.push_back(center[0]);
      ySlices.push_back(center[1]);
      zSlices.push_back(center[2]);

      vtkSMPropertyHelper(this, "XSlicesValues")
        .Set(&xSlices[0], static_cast<unsigned int>(xSlices.size()));
      vtkSMPropertyHelper(this, "YSlicesValues")
        .Set(&ySlices[0], static_cast<unsigned int>(ySlices.size()));
      vtkSMPropertyHelper(this, "ZSlicesValues")
        .Set(&zSlices[0], static_cast<unsigned int>(ySlices.size()));
    }
    this->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
void vtkSMMultiSliceViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkSMMultiSliceViewProxy::IsSelectVisiblePointsAvailable()
{
  // The original dataset and the slice don't share the same points
  return "Multi-Slice View do not allow point selection";
}

/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOMETIFFChannelCalculatorInitializationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOMETIFFChannelCalculatorInitializationHelper.h"

#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkVector.h"

#include <sstream>
#include <vector>

namespace
{

void setupDefault(int channelNumber, vtkSMProxy* lut)
{
  vtkSMPropertyHelper(lut, "EnableOpacityMapping").Set(1);
  std::vector<double> colors;
  switch (channelNumber)
  {
    case 1:
      // BLUE.
      colors = std::vector<double>{ 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0 };
      break;
    case 2:
      // GREEN
      colors = std::vector<double>{ 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0 };
      break;
    case 3:
      // RED
      colors = std::vector<double>{ 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0 };
      break;
    case 4:
      // MAGENTA
      colors = std::vector<double>{ 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0 };
      break;
    case 5:
      // CYAN
      colors = std::vector<double>{ 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
      break;
    case 6:
    default:
      // GRAY
      colors = std::vector<double>{ 0.0, 0.5, 0.5, 0.5, 1.0, 0.5, 0.5, 0.5 };
      break;
  }
  vtkSMPropertyHelper(lut, "RGBPoints").Set(&colors[0], static_cast<unsigned int>(colors.size()));
  lut->UpdateVTKObjects();
}
}

vtkStandardNewMacro(vtkSMOMETIFFChannelCalculatorInitializationHelper);
//----------------------------------------------------------------------------
vtkSMOMETIFFChannelCalculatorInitializationHelper::
  vtkSMOMETIFFChannelCalculatorInitializationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMOMETIFFChannelCalculatorInitializationHelper::
  ~vtkSMOMETIFFChannelCalculatorInitializationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMOMETIFFChannelCalculatorInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement*, vtkMTimeType ts)
{
  auto inputProxy =
    vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(proxy, "Input").GetAsProxy());
  if (!inputProxy)
  {
    return;
  }

  const auto port = vtkSMPropertyHelper(proxy, "Input").GetOutputPort();
  auto dinfo = inputProxy->GetDataInformation(port);
  vtkNew<vtkSMParaViewPipelineController> controller;

  auto pxm = proxy->GetSessionProxyManager();
  for (int cc = 1; cc <= 10; ++cc)
  {
    std::ostringstream str;
    str << "Channel" << cc << "LUT";

    auto prop = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty(str.str().c_str()));
    if (prop == nullptr || prop->GetMTime() > ts)
    {
      continue;
    }

    std::ostringstream aname;
    aname << "Channel_" << cc;
    auto ainfo =
      dinfo->GetArrayInformation(aname.str().c_str(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
    if (!ainfo)
    {
      continue;
    }

    vtkVector2d range(ainfo->GetComponentRange(0));

    const std::string aname_range = aname.str() + "_Range";
    if (auto rinfo =
          dinfo->GetArrayInformation(aname_range.c_str(), vtkDataObject::FIELD_ASSOCIATION_NONE))
    {
      range[0] = rinfo->GetComponentRange(0)[0];
      range[1] = rinfo->GetComponentRange(1)[0];
    }

    auto sof = pxm->NewProxy("piecewise_functions", "PiecewiseFunction");
    controller->InitializeProxy(sof);
    sof->UpdateVTKObjects();

    auto lut = pxm->NewProxy("lookup_tables", "PVLookupTable");
    controller->PreInitializeProxy(lut);
    ::setupDefault(cc, lut);
    vtkSMPropertyHelper(lut, "ScalarOpacityFunction").Set(sof);
    controller->PostInitializeProxy(lut);
    lut->UpdateVTKObjects();

    vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range.GetData(), /*extend*/ true);
    vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, range.GetData(), /*extend*/ true);
    prop->SetProxy(0, lut);

    sof->Delete();
    lut->Delete();
  }
  proxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMOMETIFFChannelCalculatorInitializationHelper::RegisterProxy(
  vtkSMProxy* proxy, vtkPVXMLElement*)
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  const std::string groupname = controller->GetHelperProxyGroupName(proxy);
  auto pxm = proxy->GetSessionProxyManager();
  for (int cc = 1; cc <= 10; ++cc)
  {
    std::ostringstream str;
    str << "Channel" << cc << "LUT";

    auto lut = vtkSMPropertyHelper(proxy, str.str().c_str()).GetAsProxy();
    if (lut == nullptr)
    {
      continue;
    }

    auto sof = vtkSMPropertyHelper(lut, "ScalarOpacityFunction").GetAsProxy();
    assert(sof != nullptr);

    // register the lut/sof proxies are helpers so state save/restore works.
    pxm->RegisterProxy(groupname.c_str(), lut);
    pxm->RegisterProxy(groupname.c_str(), sof);
  }
}

//----------------------------------------------------------------------------
void vtkSMOMETIFFChannelCalculatorInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTransferFunctionProxy.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkScalarsToColors.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkStringList.h"
#include "vtkTuple.h"
#include "vtk_jsoncpp.h"

#include <algorithm>
#include <math.h>
#include <vector>

vtkStandardNewMacro(vtkSMTransferFunctionProxy);
//----------------------------------------------------------------------------
vtkSMTransferFunctionProxy::vtkSMTransferFunctionProxy()
{
}

//----------------------------------------------------------------------------
vtkSMTransferFunctionProxy::~vtkSMTransferFunctionProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::RescaleTransferFunction(
  vtkSMProxy* proxy, double rangeMin, double rangeMax, bool extend)
{
  vtkSMTransferFunctionProxy* tfp =
    vtkSMTransferFunctionProxy::SafeDownCast(proxy);
  if (tfp)
    {
    return tfp->RescaleTransferFunction(rangeMin, rangeMax, extend);
    }
  return false;
}

//----------------------------------------------------------------------------
namespace
{
  class StrictWeakOrdering
    {
  public:
    bool operator()(
      const vtkTuple<double, 4>& x, const vtkTuple<double, 4>& y) const
      {
      return (x.GetData()[0] < y.GetData()[0]);
      }
    };

  inline vtkSMProperty* GetControlPointsProperty(vtkSMProxy* self)
    {
    vtkSMProperty* controlPointsProperty = self->GetProperty("RGBPoints");
    if (!controlPointsProperty)
      {
      controlPointsProperty = self->GetProperty("Points");
      }

    if (!controlPointsProperty)
      {
      vtkGenericWarningMacro("'RGBPoints' or 'Points' property is required.");
      return NULL;
      }

    vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
    unsigned int num_elements = cntrlPoints.GetNumberOfElements();
    if (num_elements % 4 != 0)
      {
      vtkGenericWarningMacro("Property must have 4-tuples. Resizing.");
      cntrlPoints.SetNumberOfElements((num_elements/4)*4);
      }

    return controlPointsProperty;
    }
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::GetRange(double range[2])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;

  vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
  if (!controlPointsProperty)
    {
    return false;
    }

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements < 4)
    {
    return false;
    }

  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements/4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  // sort the points by x, just in case user didn't add them correctly.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());

  range[0] = points.front().GetData()[0];
  range[1] = points.back().GetData()[0];
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::RescaleTransferFunction(
  double rangeMin, double rangeMax, bool extend)
{
  vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
  if (!controlPointsProperty)
    {
    return false;
    }

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements == 0)
    {
    // nothing to do, but not an error, so return true.
    return true;
    }

  if (vtkSMProperty* sriProp = this->GetProperty("ScalarRangeInitialized"))
    {
    // mark the range as initialized.
    vtkSMPropertyHelper helper(sriProp);
    bool rangeInitialized = helper.GetAsInt() != 0;
    helper.Set(1);
    if (!rangeInitialized)
      {
      // don't extend the LUT if the current data range is invalid.
      extend = false;
      }
    }

  if (num_elements == 4)
    {
    // Only 1 control point in the property. We'll add 2 points, however.
    vtkTuple<double, 4> points[2];
    cntrlPoints.Get(points[0].GetData(), 4);
    points[1] = points[0];
    if (extend)
      {
      rangeMin = std::min(rangeMin, points[0].GetData()[0]);
      rangeMax = std::max(rangeMax, points[0].GetData()[0]);
      }
    points[0].GetData()[0] = rangeMin;
    points[1].GetData()[0] = rangeMax;
    cntrlPoints.Set(points[0].GetData(), 8);
    this->UpdateVTKObjects();

    SM_SCOPED_TRACE(CallMethod)
      .arg(this)
      .arg("RescaleTransferFunction")
      .arg(rangeMin)
      .arg(rangeMax)
      .arg("comment", "Rescale transfer function");
    return true;
    }

  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements/4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  // sort the points by x, just in case user didn't add them correctly.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());

  double old_range[2] = {points.front().GetData()[0],
                         points.back().GetData()[0]};

  if (extend)
    {
    rangeMin = std::min(rangeMin, old_range[0]);
    rangeMax = std::max(rangeMax, old_range[1]);
    }

  if (old_range[0] == rangeMin && old_range[1] == rangeMax)
    {
    // nothing to do.
    return true;
    }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("RescaleTransferFunction")
    .arg(rangeMin)
    .arg(rangeMax)
    .arg("comment", "Rescale transfer function");

  // determine if the interpolation has to happen in log-space.
  bool log_space =
    (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() != 0);
  if (log_space)
    {
    // ensure the range is valid for log space.
    double range[2] = {rangeMin, rangeMax};
    if (vtkSMCoreUtilities::AdjustRangeForLog(range))
      {
      // ranges not valid for log-space. Will convert them.
      vtkWarningMacro(
        "Ranges not valid for log-space. "
        "Changed the range to (" << range[0] <<", " << range[1] << ").");
      }
    rangeMin = range[0];
    rangeMax = range[1];
    }
  double dnew = log_space? log10(rangeMax/rangeMin) :
                           (rangeMax - rangeMin);
  // don't set empty ranges. Tweak it a bit.
  dnew = (dnew > 0)? dnew : 1.0;

  double dold = log_space?  log10(old_range[1]/old_range[0]) :
                            (old_range[1] - old_range[0]);
  dold = (dold > 0)? dold : 1.0;

  double scale = dnew / dold;
  for (size_t cc=0; cc < points.size(); cc++)
    {
    double &x = points[cc].GetData()[0];
    if (log_space)
      {
      double logx = log10(x/old_range[0]) * scale + log10(rangeMin);
      x = pow(10.0, logx);
      }
    else
      {
      x = (x - old_range[0])*scale + rangeMin;
      }
    }
  cntrlPoints.Set(points[0].GetData(), num_elements);
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ComputeDataRange(double range[2])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;
  int component = -1;
  if (vtkSMPropertyHelper(this, "VectorMode").GetAsInt() == vtkScalarsToColors::COMPONENT)
    {
    component = vtkSMPropertyHelper(this, "VectorComponent").GetAsInt();
    }

  for (unsigned int cc=0, max=this->GetNumberOfConsumers(); cc < max; ++cc)
    {
    vtkSMProxy* proxy = this->GetConsumerProxy(cc);
    // consumers could be subproxy of something; so, we locate the true-parent
    // proxy for a proxy.
    proxy = proxy? proxy->GetTrueParentProxy() : NULL;
    vtkSMPVRepresentationProxy* consumer = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    if (consumer &&
      // consumer is visible.
      vtkSMPropertyHelper(consumer, "Visibility", true).GetAsInt() == 1 &&
      // consumer is using scalar coloring.
      consumer->GetUsingScalarColoring())
      {
      vtkPVArrayInformation* arrayInfo = consumer->GetArrayInformationForColorArray();
      if (!arrayInfo || (component >= 0 && arrayInfo->GetNumberOfComponents() <= component))
        {
        // skip if no arrayInfo available of doesn't have enough components.
        continue;
        }

      double cur_range[2];
      arrayInfo->GetComponentRange(component, cur_range);
      if (cur_range[0] <= cur_range[1])
        {
        range[0] = cur_range[0] < range[0]? cur_range[0] : range[0];
        range[1] = cur_range[1] > range[1]? cur_range[1] : range[1];
        }
      }
    }
  return (range[0] <= range[1]);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::RescaleTransferFunctionToDataRange(bool extend)
{
  double range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  if (this->ComputeDataRange(range))
    {
    return this->RescaleTransferFunction(range[0], range[1], extend);
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::InvertTransferFunction(vtkSMProxy* proxy)
{
  vtkSMTransferFunctionProxy* ctf =
    vtkSMTransferFunctionProxy::SafeDownCast(proxy);
  return ctf? ctf->InvertTransferFunction() : false;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::InvertTransferFunction()
{
  vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
  if (!controlPointsProperty)
    {
    return false;
    }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("InvertTransferFunction")
    .arg("comment", "invert the transfer function");

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements == 0 || num_elements == 4)
    {
    // nothing to do, but not an error, so return true.
    return true;
    }

  // determine if the interpolation has to happen in log-space.
  bool log_space =
    (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() != 0);

  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements/4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  // sort the points by x, just in case user didn't add them correctly.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());

  double range[2] = {points.front().GetData()[0],
                     points.back().GetData()[0]};
  if (range[0] <= 0.0 || range[1] <= 0.0)
    {
    // ranges not valid for log-space. Switch to linear space.
    log_space = false;
    }

  for (size_t cc=0; cc < points.size(); cc++)
    {
    double &x = points[cc].GetData()[0];
    if (log_space)
      {
      // inverting needs to happen in log-space
      double logxprime = log10(range[0]*range[1] / x);
                       /* ^-- == log10(range[1]) - (log10(x) - log10(range[0])) */
      x = pow(10.0, logxprime);
      }
    else
      {
      x = range[1] - (x - range[0]);
      }
    }
  // sort again to ensure that the property value is set as min->max.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());
  cntrlPoints.Set(points[0].GetData(), num_elements);
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(
  bool inverse/*=false*/)
{
  vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
  if (!controlPointsProperty)
    {
    return false;
    }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg(inverse? "MapControlPointsToLinearSpace" : "MapControlPointsToLogSpace")
    .arg("comment",
      inverse? "convert from log to linear" : "convert to log space");


  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements == 0 || num_elements == 4)
    {
    // nothing to do, but not an error, so return true.
    return true;
    }

  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements/4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  // sort the points by x, just in case user didn't add them correctly.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());

  double range[2] = {points.front().GetData()[0],
                     points.back().GetData()[0]};

  if (inverse == false)
    {
    if (vtkSMCoreUtilities::AdjustRangeForLog(range))
      {
      // ranges not valid for log-space. Will convert them.
      vtkWarningMacro(
        "Ranges not valid for log-space. "
        "Changed the range to (" << range[0] <<", " << range[1] << ").");
      }
    }
  if (range[0] >= range[1])
    {
    vtkWarningMacro("Empty range! Cannot map control points.");
    return false;
    }

  for (size_t cc=0; cc < points.size(); cc++)
    {
    double &x = points[cc].GetData()[0];
    if (inverse)
      {
      // log-to-linear
      double norm = log10(x/range[0]) / log10(range[1]/range[0]);
      x = range[0] + norm * (range[1] - range[0]);
      }
    else
      {
      // linear-to-log
      double norm = (x - range[0])/(range[1] - range[0]);
      double logx = log10(range[0]) + norm * (log10(range[1]/range[0]));
      x = pow(10.0, logx);
      }
    }
  cntrlPoints.Set(points[0].GetData(), num_elements);
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ApplyColorMap(const char* text)
{
  Json::Value json = this->ConvertLegacyColorMapXMLToJSON(text);
  return this->ApplyPreset(json);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ApplyColorMap(vtkPVXMLElement* xml)
{
  Json::Value json = this->ConvertLegacyColorMapXMLToJSON(xml);
  return this->ApplyPreset(json);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ApplyPreset(const Json::Value& arg, bool rescale)
{
  if (arg.isNull())
    {
    return true;
    }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("ApplyPreset")
    .arg(arg.get("Name", "-PresetName-").asString().c_str())
    .arg(rescale)
    .arg("comment", "Apply a preset using its name. "
      "Note this may not work as expected when presets have duplicate names.");

  bool usingIndexedColors = arg.isMember("IndexedColors");

  double range[2];
  bool valid_range = (rescale && !usingIndexedColors)? this->GetRange(range) : false;

  // Fill up preset with defaults for missing values.
  Json::Value preset(arg);
  preset["IndexedLookup"] = usingIndexedColors? 1 : 0;
  if (usingIndexedColors && rescale)
    {
    // if rescaling, for indexed colors, it means we need to preserve the
    // current annotations.
    preset.removeMember("Annotations");
    }

  if (!preset.isMember("HSVWrap"))
    {
    preset["HSVWrap"] = 0;
    }
  if (vtkSMSettings::DeserializeFromJSON(this, preset))
    {
    if (valid_range)
      {
      this->RescaleTransferFunction(range[0], range[1], /*extend=*/false);
      }
    this->UpdateVTKObjects();
    return true;
    }
  vtkErrorMacro("Failed to load preset properly");
  this->UpdateVTKObjects();
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ApplyPreset(const char* presetname, bool rescale)
{
  vtkNew<vtkSMTransferFunctionPresets> presets;
  return this->ApplyPreset(presets->GetFirstPresetWithName(presetname), rescale);
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::GetStateAsPreset()
{
  vtkNew<vtkStringList> toSave;
  if (this->GetProperty("RGBPoints"))
    {
    if (vtkSMPropertyHelper(this, "IndexedLookup", /*quiet=*/true).GetAsInt() == 0)
      {
      toSave->AddString("ColorSpace");
      toSave->AddString("RGBPoints");
      if (vtkSMPropertyHelper(this, "HSVWrap", true).GetAsInt() != 0)
        {
        toSave->AddString("HSVWrap");
        }
      }
    else
      {
      toSave->AddString("IndexedColors");

      // Annotations are only saved with indexed colors.
      if (vtkSMPropertyHelper(this, "Annotations", true).GetNumberOfElements() > 0)
        {
        toSave->AddString("Annotations");
        }
      }
    }
  else
    {
    toSave->AddString("Points");
    }

  vtkNew<vtkSMNamedPropertyIterator> iter;
  iter->SetProxy(this);
  iter->SetPropertyNames(toSave.GetPointer());
  return vtkSMSettings::SerializeAsJSON(this, iter.GetPointer());
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::GetStateAsPreset(vtkSMProxy* proxy)
{
  vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
  return self? self->GetStateAsPreset() : Json::Value();
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::SaveColorMap(vtkPVXMLElement* xml)
{
  if (!xml)
    {
    vtkWarningMacro("'xml' cannot be NULL");
    return false;
    }

  xml->SetName("ColorMap");

  bool indexedLookup = vtkSMPropertyHelper(this, "IndexedLookup").GetAsInt() != 0;
  xml->AddAttribute("indexedLookup", indexedLookup ? "1" : "0");

  if (!indexedLookup)
    {
    std::string space = vtkSMPropertyHelper(this, "ColorSpace").GetAsString();
    if (space == "HSV" && vtkSMPropertyHelper(this, "HSVWrap").GetAsInt() == 1)
      {
      xml->AddAttribute("space", "Wrapped");
      }
    else
      {
      xml->AddAttribute("space", space.c_str());
      }
    }

  // Add the control points.
  vtkSMProperty* controlPointsProperty = indexedLookup?
    this->GetProperty("IndexedColors") : GetControlPointsProperty(this);
  if (!controlPointsProperty)
    {
    return false;
    }

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements  > 0)
    {
    if (indexedLookup)
      {
      // Save (r,g,b) tuples for categorical colors.
      std::vector<vtkTuple<double, 3> > points;
      points.resize(num_elements/3);
      cntrlPoints.Get(points[0].GetData(), num_elements);

      for (size_t cc=0; cc < points.size(); cc++)
        {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Point");
        child->AddAttribute("r", points[cc].GetData()[1]);
        child->AddAttribute("g", points[cc].GetData()[2]);
        child->AddAttribute("b", points[cc].GetData()[3]);
        child->AddAttribute("o", "1");
        xml->AddNestedElement(child.GetPointer());
        }
      }
    else
      {
      // Save (x, r, g, b) tuples for non-categorical colors.
      std::vector<vtkTuple<double, 4> > points;
      points.resize(num_elements/4);
      cntrlPoints.Get(points[0].GetData(), num_elements);

      // sort the points by x, just in case user didn't add them correctly.
      std::sort(points.begin(), points.end(), StrictWeakOrdering());

      for (size_t cc=0; cc < points.size(); cc++)
        {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Point");
        child->AddAttribute("x", points[cc].GetData()[0]);
        child->AddAttribute("r", points[cc].GetData()[1]);
        child->AddAttribute("g", points[cc].GetData()[2]);
        child->AddAttribute("b", points[cc].GetData()[3]);
        child->AddAttribute("o", "1");
        xml->AddNestedElement(child.GetPointer());
        }
      }
    }

  // add NanColor.
  vtkSMPropertyHelper nanProperty(this, "NanColor");
  vtkNew<vtkPVXMLElement> nan;
  nan->SetName("NaN");
  nan->AddAttribute("r", nanProperty.GetAsDouble(0));
  nan->AddAttribute("g", nanProperty.GetAsDouble(1));
  nan->AddAttribute("b", nanProperty.GetAsDouble(2));
  xml->AddNestedElement(nan.GetPointer());

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::IsScalarBarVisible(vtkSMProxy* view)
{
  if (vtkSMProxy* sbProxy = this->FindScalarBarRepresentation(view))
    {
    if (vtkSMPropertyHelper(sbProxy, "Visibility").GetAsInt() == 1)
      {
      return true;
      }
    }

  return false;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMTransferFunctionProxy::FindScalarBarRepresentation(vtkSMProxy* view)
{
  if (!view || !view->GetProperty("Representations"))
    {
    return NULL;
    }

  vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc=0; cc < reprHelper.GetNumberOfElements(); cc++)
    {
    vtkSMProxy* current = reprHelper.GetAsProxy(cc);
    if (current && current->GetXMLName() &&
      strcmp(current->GetXMLName(), "ScalarBarWidgetRepresentation") == 0 &&
      current->GetProperty("LookupTable"))
      {
      if (vtkSMPropertyHelper(current, "LookupTable").GetAsProxy() == this)
        {
        return current;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::UpdateScalarBarsComponentTitle(
  vtkPVArrayInformation* info)
{
  // find all scalar bars for this transfer function and update their titles.
  for (unsigned int cc=0, max = this->GetNumberOfConsumers(); cc < max; ++cc)
    {
    vtkSMProxy* consumer = this->GetConsumerProxy(cc);
    while (consumer && consumer->GetParentProxy())
      {
      consumer = consumer->GetParentProxy();
      }

    vtkSMScalarBarWidgetRepresentationProxy* sb =
      vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(consumer);
    if (sb)
      {
      sb->UpdateComponentTitle(info);
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionProxy::ResetPropertiesToXMLDefaults(
  bool preserve_range)
{
  try
    {
    if (!preserve_range)
      {
      throw true;
      }

    vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
    if (!controlPointsProperty)
      {
      throw true;
      }

    vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
    unsigned int num_elements = cntrlPoints.GetNumberOfElements();
    if (num_elements == 0 || num_elements == 4)
      {
      // nothing to do, but not an error, so return true.
      throw true;
      }

    std::vector<vtkTuple<double, 4> > points;
    points.resize(num_elements/4);
    cntrlPoints.Get(points[0].GetData(), num_elements);

    // sort the points by x, just in case user didn't add them correctly.
    std::sort(points.begin(), points.end(), StrictWeakOrdering());

    double range[2] = {points.front().GetData()[0],
      points.back().GetData()[0]};
    this->ResetPropertiesToXMLDefaults();
    this->RescaleTransferFunction(range[0], range[1], false);
    }
  catch (bool val)
    {
    if (val)
      {
      this->ResetPropertiesToXMLDefaults();
      }
    }
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::ConvertLegacyColorMapXMLToJSON(vtkPVXMLElement* xml)
{
  if (!xml || !xml->GetName() || strcmp(xml->GetName(), "ColorMap") != 0)
    {
    vtkGenericWarningMacro("'ColorMap' XML expected.");
    return Json::Value();
    }

  Json::Value json(Json::objectValue);

  bool indexedLookup =
    (strcmp(xml->GetAttributeOrDefault("indexedLookup", "false"), "true") == 0);
  if (!indexedLookup)
    {
    // load color-space for only for non-categorical color maps
    std::string colorSpace = xml->GetAttributeOrDefault("space", "NoChange");
    if (colorSpace == "Wrapped")
      {
      json["HSVWrap"] = 1;
      json["ColorSpace"] = "HSV";
      }
    else if (colorSpace != "NoChange")
      {
      json["ColorSpace"] = colorSpace;
      }
    }

  vtkPVXMLElement* nanElement = xml->FindNestedElementByName("NaN");
  if (nanElement && nanElement->GetAttribute("r") &&
    nanElement->GetAttribute("g") && nanElement->GetAttribute("b"))
    {
    double rgb[3];
    nanElement->GetScalarAttribute("r", &rgb[0]);
    nanElement->GetScalarAttribute("g", &rgb[1]);
    nanElement->GetScalarAttribute("b", &rgb[2]);

    Json::Value nancolor(Json::arrayValue);
    nancolor[0] = rgb[0];
    nancolor[1] = rgb[1];
    nancolor[2] = rgb[2];
    json["NanColor"] = nancolor;
    }

  // Read the control points from the XML.
  std::vector<vtkTuple<double, 4> > new_points;
  std::vector<vtkTuple<const char*, 2> > new_annotations;

  for (unsigned int cc=0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* pointElement = xml->GetNestedElement(cc);
    double xrgb[4];
    if (pointElement && pointElement->GetName() &&
      strcmp(pointElement->GetName(), "Point") == 0 &&
      pointElement->GetScalarAttribute("r", &xrgb[1]) &&
      pointElement->GetScalarAttribute("g", &xrgb[2]) &&
      pointElement->GetScalarAttribute("b", &xrgb[3]))
      {
      if (!indexedLookup &&
        pointElement->GetScalarAttribute("x", &xrgb[0]))
        {
        // "x" attribute is only needed for non-categorical color maps.
        new_points.push_back(vtkTuple<double, 4>(xrgb));
        }
      else if (indexedLookup)
        {
        // since "x" attribute is only needed for non-categorical color maps, we
        // make up one. This will be ignored when setting the "IndexedColors"
        // property.
        xrgb[0] = cc;
        new_points.push_back(vtkTuple<double, 4>(xrgb));
        }
      }
    else if (pointElement && pointElement->GetName() &&
      strcmp(pointElement->GetName(),"Annotation") &&
      pointElement->GetAttribute("v") &&
      pointElement->GetAttribute("t"))
      {
      const char* value[2] = {
          pointElement->GetAttribute("v"),
          pointElement->GetAttribute("t")};
      new_annotations.push_back(vtkTuple<const char*, 2>(value));
      }
    }

  if (new_annotations.size() > 0)
    {
    Json::Value annotations(Json::arrayValue);
    for (int cc=0, max = static_cast<int>(new_annotations.size()); cc < max; cc++)
      {
      annotations[2*cc] = new_annotations[cc][0];
      annotations[2*cc+1] = new_annotations[cc][1];
      }
    json["Annotations"] = annotations;
    }

  if (new_points.size() > 0 && indexedLookup)
    {
    Json::Value rgbColors(Json::arrayValue);
    for (int cc=0, max = static_cast<int>(new_points.size()); cc < max; cc++)
      {
      rgbColors[3*cc] = new_points[cc].GetData()[1];
      rgbColors[3*cc + 1] = new_points[cc].GetData()[2];
      rgbColors[3*cc + 2] = new_points[cc].GetData()[3];
      }
    json["IndexedColors"] = rgbColors;
    }
  else if (new_points.size() > 0 && !indexedLookup)
    {
    // sort the points by x, just in case user didn't add them correctly.
    std::sort(new_points.begin(), new_points.end(), StrictWeakOrdering());

    Json::Value rgbColors(Json::arrayValue);
    for (int cc=0, max = static_cast<int>(new_points.size()); cc < max; cc++)
      {
      rgbColors[4*cc] = new_points[cc].GetData()[0];
      rgbColors[4*cc + 1] = new_points[cc].GetData()[1];
      rgbColors[4*cc + 2] = new_points[cc].GetData()[2];
      rgbColors[4*cc + 3] = new_points[cc].GetData()[3];
      }
    json["RGBPoints"] = rgbColors;
    }

  // add name.
  json["Name"] = xml->GetAttribute("name");
  if (const char* creator= xml->GetAttribute("creator"))
    {
    json["Creator"] = creator;
    }
  return json;
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::ConvertLegacyColorMapXMLToJSON(const char* xmlcontents)
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(xmlcontents))
    {
    return Json::Value();
    }
  return vtkSMTransferFunctionProxy::ConvertLegacyColorMapXMLToJSON(parser->GetRootElement());
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(vtkPVXMLElement* xml)
{
  if (!xml || !xml->GetName() || strcmp(xml->GetName(), "ColorMaps") != 0)
    {
    vtkGenericWarningMacro("'ColorMaps' XML expected.");
    return Json::Value();
    }

  Json::Value json(Json::arrayValue);
  for (unsigned int cc=0, max=xml->GetNumberOfNestedElements(); cc < max; ++cc)
    {
    vtkPVXMLElement* elem = xml->GetNestedElement(cc);
    if (elem && elem->GetName() && strcmp(elem->GetName(), "ColorMap") == 0 && elem->GetAttribute("name"))
      {
      Json::Value cmap = vtkSMTransferFunctionProxy::ConvertLegacyColorMapXMLToJSON(elem);
      if (!cmap.empty())
        {
        json.append(cmap);
        }
      }
    }

  return json;
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(const char* xmlcontents)
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(xmlcontents))
    {
    return Json::Value();
    }
  return vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(parser->GetRootElement());
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ConvertLegacyColorMapsToJSON(
  const char* inxmlfile, const char* outjsonfile)
{
  vtkNew<vtkPVXMLParser> parser;
  parser->SetFileName(inxmlfile);
  if (!parser->Parse())
    {
    vtkGenericWarningMacro("Failed to parse XML!");
    return false;
    }

  Json::Value json = vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(parser->GetRootElement());
  if (json.empty())
    {
    return false;
    }

  ofstream file;
  file.open(outjsonfile);
  if (file)
    {
    file << json.toStyledString().c_str();
    file.close();
    return true;
    }
  return false;
}


//----------------------------------------------------------------------------
void vtkSMTransferFunctionProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

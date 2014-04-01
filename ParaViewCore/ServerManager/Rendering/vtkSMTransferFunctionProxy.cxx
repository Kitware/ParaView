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
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTuple.h"

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

  // determine if the interpolation has to happen in log-space.
  bool log_space =
    (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() != 0);
  // ensure that log_space is valid for the ranges (before and after).
  if (rangeMin <= 0.0 || rangeMax <= 0.0 ||
      old_range[0] <= 0.0 || old_range[1] <= 0.0)
    {
    log_space = false;
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
bool vtkSMTransferFunctionProxy::RescaleTransferFunctionToDataRange(bool extend)
{
  double range[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
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
  if (range[0] <= range[1])
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

  if (inverse == false && (range[0] <= 0.0 || range[1] <= 0.0))
    {
    // ranges not valid for log-space. Cannot convert.
    vtkWarningMacro("Ranges not valid for log-space. "
      "Cannot map control points to log-space.");
    return false;
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
  vtkNew<vtkPVXMLParser> parser;
  if (parser->Parse(text))
    {
    return false;
    }

  return this->ApplyColorMap(parser->GetRootElement());
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ApplyColorMap(vtkPVXMLElement* xml)
{
  if (!xml || !xml->GetName() || strcmp(xml->GetName(), "ColorMap") != 0)
    {
    vtkWarningMacro("'ColorMap' XML expected.");
    return false;
    }

  bool indexedLookup = 
    (strcmp(xml->GetAttributeOrDefault("indexedLookup", "false"), "true") == 0);
  vtkSMPropertyHelper(this, "IndexedLookup").Set(indexedLookup? 1 : 0);

  if (!indexedLookup)
    {
    // load color-space for only for non-categorical color maps
    std::string colorSpace = xml->GetAttributeOrDefault("space", "NoChange");
    if (colorSpace == "Wrapped")
      {
      vtkSMPropertyHelper(this, "HSVWrap").Set(1);
      vtkSMPropertyHelper(this, "ColorSpace").Set("HSV");
      }
    else if (colorSpace != "NoChange")
      {
      vtkSMPropertyHelper(this, "HSVWrap").Set(0);
      vtkSMPropertyHelper(this, "ColorSpace").Set(colorSpace.c_str());
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
    vtkSMPropertyHelper(this, "NanColor").Set(rgb, 3);
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
      strcmp(pointElement->GetName()," Annotation") &&
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
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      this->GetProperty("Annotations"));
    if (svp)
      {
      svp->SetElements(new_annotations[0].GetData(),
        static_cast<unsigned int>(new_annotations.size()*2));
      }
    }

  if (new_points.size() > 0 && indexedLookup)
    {
    std::vector<vtkTuple<double, 3> > rgbColors;
    rgbColors.resize(new_points.size());
    for (size_t cc=0; cc < new_points.size(); cc++)
      {
      rgbColors[cc].GetData()[0] = new_points[cc].GetData()[1];
      rgbColors[cc].GetData()[1] = new_points[cc].GetData()[2];
      rgbColors[cc].GetData()[2] = new_points[cc].GetData()[3];
      }

    vtkSMPropertyHelper indexedColors(this->GetProperty("IndexedColors"));
    indexedColors.Set(rgbColors[0].GetData(),
      static_cast<unsigned int>(rgbColors.size() * 3));
    }
  else if (new_points.size() > 0 && !indexedLookup)
    {
    vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
    if (!controlPointsProperty)
      {
      return false;
      }

    vtkSMPropertyHelper cntrlPoints(controlPointsProperty);

    // sort the points by x, just in case user didn't add them correctly.
    std::sort(new_points.begin(), new_points.end(), StrictWeakOrdering());

    if (new_points.front().GetData()[0] == 0.0 &&
      new_points.back().GetData()[0] == 1.0)
      {
      // normalized LUT, we will rescale it using the current range.
      unsigned int num_elements = cntrlPoints.GetNumberOfElements();
      if (num_elements != 0 || num_elements == 4)
        {
        std::vector<vtkTuple<double, 4> > points;
        points.resize(num_elements/4);
        cntrlPoints.Get(points[0].GetData(), num_elements);
        std::sort(points.begin(), points.end(), StrictWeakOrdering());
        double range[2] = {points.front().GetData()[0], points.back().GetData()[0]};

        // determine if the interpolation has to happen in log-space.
        bool log_space =
          (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() != 0);
        if (range[0] <= 0.0 || range[1] <= 0.0)
          {
          // ranges not valid for log-space. Switch to linear space.
          log_space = false;
          }

        for (size_t cc=0; cc < new_points.size(); cc++)
          {
          double &x = new_points[cc].GetData()[0];
          if (log_space)
            {
            double logx = log10(range[0]) + x*log10(range[1]/range[0]);
              /// ==== log10(range[0]) + (log10(range[1] - log10(range[0])) * x;
            x = pow(10.0, logx);
            }
          else
            {
            // since x is in [0, 1].
            x = range[0] + (range[1] - range[0]) * x;
            }
          }
        }
      }
    // TODO: we may need to handle normalized/non-normalized color-map more
    // elegantly.
    cntrlPoints.Set(new_points[0].GetData(),
      static_cast<unsigned int>(new_points.size() * 4));
    }
  this->UpdateVTKObjects();
  return true;
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
void vtkSMTransferFunctionProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

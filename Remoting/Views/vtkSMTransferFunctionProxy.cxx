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

#include "vtkAlgorithm.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkScalarsToColors.h"
#include "vtkStringList.h"
#include "vtkTable.h"
#include "vtkTuple.h"
#include "vtk_jsoncpp.h"

#include "vtksys/FStream.hxx"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <vector>

namespace
{
//----------------------------------------------------------------------------
class StrictWeakOrdering
{
public:
  bool operator()(const vtkTuple<double, 4>& x, const vtkTuple<double, 4>& y) const
  {
    return (x.GetData()[0] < y.GetData()[0]);
  }
};

//----------------------------------------------------------------------------
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
    return nullptr;
  }

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements % 4 != 0)
  {
    vtkGenericWarningMacro("Property must have 4-tuples. Resizing.");
    cntrlPoints.SetNumberOfElements((num_elements / 4) * 4);
  }

  return controlPointsProperty;
}

//----------------------------------------------------------------------------
// Normalize cntrlPoints so that the range goes from (0, 1). The cntrlPoints
// are assumed to be using log-space interpolation if "log_space" is true.
// The result is always in linear space irrespective of the original
// interpolation space.
// originalRange is filled with the original range of the cntrlPoints before
// rescaling.
bool vtkNormalize(std::vector<vtkTuple<double, 4> >& cntrlPoints, bool log_space,
  vtkTuple<double, 2>* originalRange = nullptr)
{
  if (cntrlPoints.size() == 0)
  {
    // nothing to do, but not an error, so return true.
    return true;
  }
  if (cntrlPoints.size() == 1)
  {
    if (originalRange)
    {
      (*originalRange)[0] = cntrlPoints[0][0];
      (*originalRange)[1] = cntrlPoints[0][0];
    }

    // Only 1 control point in the property. We'll add 2 points, however.
    cntrlPoints.resize(2);
    cntrlPoints[1] = cntrlPoints[0];
    cntrlPoints[0][0] = 0.0;
    cntrlPoints[1][0] = 1.0;
    return true;
  }

  // sort the points by x, just in case user didn't add them correctly.
  std::sort(cntrlPoints.begin(), cntrlPoints.end(), StrictWeakOrdering());

  const double old_range[2] = { cntrlPoints.front().GetData()[0], cntrlPoints.back().GetData()[0] };
  if (log_space && (old_range[0] <= 0 || old_range[1] <= 0))
  {
    vtkGenericWarningMacro("Range not valid for log. Assuming control points "
                           "are not specified in log space.");
    log_space = false;
  }
  if (originalRange)
  {
    (*originalRange)[0] = old_range[0];
    (*originalRange)[1] = old_range[1];
  }

  // if in log_space, let's convert all the control point values to
  // log.
  if (log_space)
  {
    for (size_t cc = 0; cc < cntrlPoints.size(); ++cc)
    {
      cntrlPoints[cc][0] = log10(cntrlPoints[cc][0]);
    }
  }

  // now simply normalize the cntrlPoints.
  const double range[2] = { cntrlPoints.front()[0], cntrlPoints.back()[0] };
  if (range[0] == 0.0 && range[1] == 1.0)
  {
    // nothing to do.
    return true;
  }
  const double denominator = (range[1] - range[0]);
  assert(denominator > 0);
  for (size_t cc = 0; cc < cntrlPoints.size(); ++cc)
  {
    cntrlPoints[cc][0] = (cntrlPoints[cc][0] - range[0]) / denominator;
  }
  return true;
}

//----------------------------------------------------------------------------
// Rescale normalize control points to the given range. If "log_space" is
// true, the log interpolation is used between rangeMin and rangeMax. If that
// case too, rangeMin/rangeMax should be still specified as the original data
// values (and not log of the data values).
bool vtkRescaleNormalizedControlPoints(
  std::vector<vtkTuple<double, 4> >& cntrlPoints, double rangeMin, double rangeMax, bool log_space)
{
  assert(cntrlPoints.size() >= 2);
  assert(cntrlPoints.front()[0] == 0.0 && cntrlPoints.back()[0] == 1.0);
  assert(rangeMin < rangeMax);
  if (log_space && (rangeMin <= 0.0 || rangeMax <= 0.0))
  {
    // ensure the range is valid for log space.
    double range[2] = { rangeMin, rangeMax };
    if (vtkSMCoreUtilities::AdjustRangeForLog(range))
    {
      // ranges not valid for log-space. Will convert them.
      vtkGenericWarningMacro("Ranges not valid for log-space. "
                             "Changed the range to ("
        << range[0] << ", " << range[1] << ").");
    }
    rangeMin = range[0];
    rangeMax = range[1];
  }

  double scale = (rangeMax - rangeMin);
  if (log_space)
  {
    rangeMin = log10(rangeMin);
    rangeMax = log10(rangeMax);
    scale = (rangeMax - rangeMin);
  }
  assert(scale > 0);
  for (size_t cc = 0; cc < cntrlPoints.size(); ++cc)
  {
    double& x = cntrlPoints[cc][0];
    x = x * scale + rangeMin;
    if (log_space)
    {
      x = pow(10.0, x);
    }
  }
  return true;
}
}

vtkStandardNewMacro(vtkSMTransferFunctionProxy);

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::RescaleTransferFunction(
  vtkSMProxy* proxy, double rangeMin, double rangeMax, bool extend)
{
  vtkSMTransferFunctionProxy* tfp = vtkSMTransferFunctionProxy::SafeDownCast(proxy);

  if (!tfp)
  {
    return false;
  }

  return tfp->RescaleTransferFunction(rangeMin, rangeMax, extend);
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
  points.resize(num_elements / 4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  // sort the points by x, just in case user didn't add them correctly.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());

  range[0] = points.front().GetData()[0];
  range[1] = points.back().GetData()[0];
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ExportTransferFunction(
  vtkSMTransferFunctionProxy* colorTransferFunction,
  vtkSMTransferFunctionProxy* opacityTransferFunction, const char* tfname, const char* filename)
{
  Json::Value exportCollection(Json::arrayValue);
  Json::Value transferFunction =
    vtkSMTransferFunctionProxy::GetStateAsPreset(colorTransferFunction);
  transferFunction["Name"] = tfname;

  if (opacityTransferFunction)
  {
    Json::Value opacities = vtkSMTransferFunctionProxy::GetStateAsPreset(opacityTransferFunction);
    if (opacities.isMember("Points"))
    {
      transferFunction["Points"] = opacities["Points"];
    }
  }

  exportCollection.append(transferFunction);

  vtksys::ofstream outfs;
  outfs.open(filename);
  if (!outfs.is_open())
  {
    std::cerr << "Failed to open file for writing: " << filename;
    return false;
  }
  outfs << exportCollection.toStyledString().c_str() << endl;
  outfs.close();
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

  vtkSMCoreUtilities::AdjustRange(rangeMin, rangeMax);
  if (rangeMax < rangeMin)
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

  bool log_space = (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() != 0);

  // just in case the num_elements is not a perfect multiple of 4.
  num_elements = 4 * (num_elements / 4);
  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements / 4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  vtkTuple<double, 2> preNormalizationRange;
  vtkNormalize(points, log_space, &preNormalizationRange);
  if (extend)
  {
    rangeMin = std::min(rangeMin, preNormalizationRange[0]);
    rangeMax = std::max(rangeMax, preNormalizationRange[1]);
  }
  if (preNormalizationRange[0] == rangeMin && preNormalizationRange[1] == rangeMax)
  {
    // current range is same as the new range. Nothing to do here.
    return true;
  }

  this->LastRange[0] = rangeMin;
  this->LastRange[1] = rangeMax;
  vtkRescaleNormalizedControlPoints(points, rangeMin, rangeMax, log_space);
  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("RescaleTransferFunction")
    .arg(rangeMin)
    .arg(rangeMax)
    .arg("comment", "Rescale transfer function");
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

  for (unsigned int cc = 0, max = this->GetNumberOfConsumers(); cc < max; ++cc)
  {
    vtkSMProxy* proxy = this->GetConsumerProxy(cc);
    // consumers could be subproxy of something; so, we locate the true-parent
    // proxy for a proxy.
    proxy = proxy ? proxy->GetTrueParentProxy() : nullptr;
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
      arrayInfo->GetComponentFiniteRange(component, cur_range);
      if (cur_range[0] <= cur_range[1])
      {
        range[0] = cur_range[0] < range[0] ? cur_range[0] : range[0];
        range[1] = cur_range[1] > range[1] ? cur_range[1] : range[1];
      }
    }
  }
  return (range[0] <= range[1]);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::ComputeAvailableAnnotations(bool extend)
{
  if (strcmp(this->GetXMLName(), "PiecewiseFunction") == 0)
  {
    // This is a scalar opacity function. Do not compute available annotations.
    return false;
  }

  int component = -1;
  if (vtkSMPropertyHelper(this, "VectorMode").GetAsInt() == vtkScalarsToColors::COMPONENT)
  {
    component = vtkSMPropertyHelper(this, "VectorComponent").GetAsInt();
  }

  vtkSMStringVectorProperty* allAnnotations =
    vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("Annotations"));
  vtkSMStringVectorProperty* activeAnnotatedValuesProperty =
    vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("ActiveAnnotatedValues"));
  vtkNew<vtkStringList> activeAnnotatedValues;

  if (!allAnnotations || !activeAnnotatedValuesProperty)
  {
    return false;
  }

  // If we are extending, copy the active annotated values
  if (extend)
  {
    activeAnnotatedValuesProperty->GetElements(activeAnnotatedValues);
  }

  for (unsigned int cc = 0, max = this->GetNumberOfConsumers(); cc < max; ++cc)
  {
    vtkSMProxy* proxy = this->GetConsumerProxy(cc);
    // consumers could be subproxy of something; so, we locate the true-parent
    // proxy for a proxy.
    proxy = proxy ? proxy->GetTrueParentProxy() : nullptr;
    vtkSMPVRepresentationProxy* consumer = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    if (consumer &&
      // consumer is visible.
      vtkSMPropertyHelper(consumer, "Visibility", true).GetAsInt() == 1 &&
      // consumer is using scalar coloring.
      consumer->GetUsingScalarColoring())
    {
      vtkPVProminentValuesInformation* prominentValues =
        vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(consumer);
      if (!prominentValues)
      {
        continue;
      }
      vtkSmartPointer<vtkAbstractArray> uniqueValues;
      uniqueValues.TakeReference(prominentValues->GetProminentComponentValues(component));

      if (uniqueValues)
      {
        for (int idx = 0; idx < uniqueValues->GetNumberOfTuples(); ++idx)
        {
          // Look up index of color corresponding to the annotation
          for (unsigned int j = 0; j < allAnnotations->GetNumberOfElements() / 2; ++j)
          {
            vtkVariant annotatedValue(allAnnotations->GetElement(2 * j + 0));
            if (annotatedValue == uniqueValues->GetVariantValue(idx))
            {
              activeAnnotatedValues->AddUniqueString(allAnnotations->GetElement(2 * j + 0));
              break;
            }
          }
        }
      }
    }
  }

  activeAnnotatedValuesProperty->SetElements(activeAnnotatedValues);
  this->UpdateVTKObjects();

  return true;
}

//----------------------------------------------------------------------------
vtkTable* vtkSMTransferFunctionProxy::ComputeDataHistogramTable(int numberOfBins)
{
  if (!this->HistogramTableCache)
  {
    this->HistogramTableCache = vtkSmartPointer<vtkTable>::New();
  }
  this->HistogramTableCache->Initialize();

  // Recover component property
  int component = -1;
  if (vtkSMPropertyHelper(this, "VectorMode").GetAsInt() == vtkScalarsToColors::COMPONENT)
  {
    component = vtkSMPropertyHelper(this, "VectorComponent").GetAsInt();
  }

  // Create a GroupDataSet filter
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSmartPointer<vtkSMSourceProxy> group;
  group.TakeReference(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "GroupDataSets")));

  // Group all visible consumers using the transfer function proxy
  vtkPVArrayInformation* arrayInfo = nullptr;
  std::string arrayName;
  int arrayAsso = -1;
  bool hasData = false;
  std::set<vtkSMProxy*> usedProxy;
  for (unsigned int cc = 0, max = this->GetNumberOfConsumers(); cc < max; ++cc)
  {
    vtkSMProxy* proxy = this->GetConsumerProxy(cc);
    // consumers could be subproxy of something; so, we locate the true-parent
    // proxy for a proxy.
    proxy = proxy ? proxy->GetTrueParentProxy() : nullptr;
    vtkSMPVRepresentationProxy* consumer = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
    if (consumer &&
      // consumer is visible.
      vtkSMPropertyHelper(consumer, "Visibility", true).GetAsInt() == 1 &&
      // consumer is using scalar coloring.
      consumer->GetUsingScalarColoring() &&
      // do not count proxy multiples times
      usedProxy.find(consumer) == usedProxy.end())
    {
      // Recover consumer color array
      vtkPVArrayInformation* tmpArrayInfo = consumer->GetArrayInformationForColorArray(false);
      if (!tmpArrayInfo)
      {
        continue;
      }

      // We suppose that the first consumer array info is valid
      if (!arrayInfo)
      {
        arrayInfo = tmpArrayInfo;
        if (arrayInfo->GetNumberOfComponents() == 1)
        {
          // Set the right component value for single component array
          component = 0;
        }
        if (component == -1)
        {
          // Set the right component value for magnitude component
          component = arrayInfo->GetNumberOfComponents();
        }
        if (component > arrayInfo->GetNumberOfComponents())
        {
          vtkErrorMacro("Invalid component requested by the transfer function");
          this->HistogramTableCache = nullptr;
          return this->HistogramTableCache;
        }

        vtkSMPropertyHelper colorArrayHelper(consumer, "ColorArrayName");
        arrayAsso = colorArrayHelper.GetInputArrayAssociation();
        arrayName = colorArrayHelper.GetInputArrayNameToProcess();
      }
      else
      {
        // Check other consumers array infos against the first one
        if (arrayInfo->GetNumberOfComponents() != tmpArrayInfo->GetNumberOfComponents())
        {
          vtkWarningMacro("A transfer function consumer is not providing an array with the right "
                          "number of components. Ignored.");
          continue;
        }

        vtkSMPropertyHelper colorArrayHelper(consumer, "ColorArrayName");
        if (arrayAsso != colorArrayHelper.GetInputArrayAssociation())
        {
          vtkWarningMacro("A transfer function consumer is not providing an array with the right "
                          "array association. Ignored");
          continue;
        }
        if (arrayName != std::string(colorArrayHelper.GetInputArrayNameToProcess()))
        {
          vtkWarningMacro(
            "A transfer function consumer is not providing an array with the right name. Ignored.");
          continue;
        }
      }

      // Add consumer to group filter
      vtkSMSourceProxy* input =
        vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(consumer, "Input").GetAsProxy());
      vtkSMPropertyHelper(group, "Input").Add(input);
      group->UpdateVTKObjects();
      hasData = true;
      usedProxy.insert(consumer);
    }
  }

  // No valid consumer
  if (!hasData)
  {
    this->HistogramTableCache = nullptr;
    return this->HistogramTableCache;
  }

  // Compute the histogram
  vtkSmartPointer<vtkSMSourceProxy> histo;
  histo.TakeReference(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ExtractHistogram")));
  vtkSMPropertyHelper(histo, "Input").Set(group);
  vtkSMPropertyHelper(histo, "SelectInputArray")
    .SetInputArrayToProcess(arrayAsso, arrayName.c_str());
  vtkSMPropertyHelper(histo, "Component").Set(component);
  vtkSMPropertyHelper(histo, "BinCount").Set(numberOfBins);
  vtkSMPropertyHelper(histo, "UseCustomBinRanges").Set(true);
  vtkSMPropertyHelper(histo, "CustomBinRanges").Set(this->LastRange, 2);
  histo->UpdateVTKObjects();

  // Reduce it
  vtkSmartPointer<vtkSMSourceProxy> reducer;
  reducer.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ReductionFilter")));
  vtkSMPropertyHelper(reducer, "Input").Set(histo);
  vtkSMPropertyHelper(reducer, "PostGatherHelperName").Set("vtkPVMergeTables");
  reducer->UpdateVTKObjects();

  // Move it from server to client and save it to the case
  vtkSmartPointer<vtkSMSourceProxy> mover;
  mover.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ClientServerMoveData")));
  vtkSMPropertyHelper(mover, "Input").Set(reducer);
  vtkSMPropertyHelper(mover, "OutputDataType").Set(VTK_TABLE);
  mover->UpdateVTKObjects();
  mover->UpdatePipeline();
  vtkTable* histoTable = vtkTable::SafeDownCast(
    vtkAlgorithm::SafeDownCast(mover->GetClientSideObject())->GetOutputDataObject(0));
  this->HistogramTableCache->ShallowCopy(histoTable);

  // Sanity check of the histogram table
  if (this->HistogramTableCache->GetNumberOfColumns() < 2)
  {
    vtkErrorMacro("Histogram is not producing enough data");
    this->HistogramTableCache = nullptr;
    return this->HistogramTableCache;
  }
  vtkIntArray* valueArray = vtkIntArray::SafeDownCast(this->HistogramTableCache->GetColumn(1));
  if (!valueArray)
  {
    vtkErrorMacro("Histogram is not producing integer data as expected");
    this->HistogramTableCache = nullptr;
    return this->HistogramTableCache;
  }

  // Copy histogram values, currently stored in an int array,
  // into a double array in order to be able to use shift scale in the related plots
  vtkIdType nValue = valueArray->GetNumberOfTuples();
  int* valuePointer = static_cast<int*>(valueArray->GetPointer(0));
  vtkNew<vtkDoubleArray> doubleValueArray;
  doubleValueArray->SetName(valueArray->GetName());
  doubleValueArray->SetNumberOfTuples(valueArray->GetNumberOfTuples());
  std::copy(
    valuePointer, valuePointer + nValue, static_cast<double*>(doubleValueArray->GetVoidPointer(0)));
  this->HistogramTableCache->RemoveColumn(1);
  this->HistogramTableCache->AddColumn(doubleValueArray);
  return this->HistogramTableCache;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::RescaleTransferFunctionToDataRange(bool extend)
{
  double range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  if (this->ComputeDataRange(range))
  {
    return this->RescaleTransferFunction(range[0], range[1], extend);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::InvertTransferFunction(vtkSMProxy* proxy)
{
  vtkSMTransferFunctionProxy* ctf = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
  return ctf ? ctf->InvertTransferFunction() : false;
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
  bool log_space = (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() != 0);

  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements / 4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  vtkTuple<double, 2> range;
  vtkNormalize(points, log_space, &range);
  for (size_t cc = 0; cc < points.size(); cc++)
  {
    double& x = points[cc].GetData()[0];
    x = (1.0 - x);
  }
  // sort again to ensure that the property value is set as min->max.
  std::sort(points.begin(), points.end(), StrictWeakOrdering());
  vtkRescaleNormalizedControlPoints(points, range[0], range[1], log_space);
  this->LastRange[0] = range[0];
  this->LastRange[1] = range[1];

  cntrlPoints.Set(points[0].GetData(), num_elements);
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(bool inverse /*=false*/)
{
  vtkSMProperty* controlPointsProperty = GetControlPointsProperty(this);
  if (!controlPointsProperty)
  {
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg(inverse ? "MapControlPointsToLinearSpace" : "MapControlPointsToLogSpace")
    .arg("comment", inverse ? "convert from log to linear" : "convert to log space");

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements == 0 || num_elements == 4)
  {
    // nothing to do, but not an error, so return true.
    return true;
  }

  std::vector<vtkTuple<double, 4> > points;
  points.resize(num_elements / 4);
  cntrlPoints.Get(points[0].GetData(), num_elements);

  vtkTuple<double, 2> range;
  vtkNormalize(points, inverse, &range); // if inverse==true, we're in log-space currently.
  if (range[0] >= range[1])
  {
    vtkWarningMacro("Empty range! Cannot map control points.");
    return false;
  }
  vtkRescaleNormalizedControlPoints(points, range[0], range[1], !inverse);
  this->LastRange[0] = range[0];
  this->LastRange[1] = range[1];
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
  bool valid_range = (rescale && !usingIndexedColors) ? this->GetRange(range) : false;

  // Fill up preset with defaults for missing values.
  Json::Value preset(arg);
  preset["IndexedLookup"] = usingIndexedColors ? 1 : 0;
  if (usingIndexedColors && rescale)
  {
    // if rescaling, for indexed colors, it means we need to preserve the
    // current annotations.
    preset.removeMember("Annotations");
  }

  if (rescale && valid_range)
  {
    assert(usingIndexedColors == false);

    // Since rescaling gets tricky especially when log scaling is involved
    // either in the preset or in the proxy we're loading the preset values on,
    // we will just "rescale" the range in the preset itself.
    Json::Value& pointsValue =
      this->GetProperty("RGBPoints") ? preset["RGBPoints"] : preset["Points"];
    if (pointsValue.isNull() || !pointsValue.isArray() || (pointsValue.size() % 4) != 0 ||
      pointsValue.size() == 0)
    {
      vtkErrorMacro("Preset may not be valid. Please validate the preset:\n"
        << arg.toStyledString().c_str());
      return false;
    }

    std::vector<vtkTuple<double, 4> > cntrlPoints;
    cntrlPoints.resize(pointsValue.size() / 4);
    for (Json::ArrayIndex cc = 0, max = pointsValue.size() / 4; cc < max; ++cc)
    {
      cntrlPoints[cc][0] = pointsValue[4 * cc].asDouble();
      cntrlPoints[cc][1] = pointsValue[4 * cc + 1].asDouble();
      cntrlPoints[cc][2] = pointsValue[4 * cc + 2].asDouble();
      cntrlPoints[cc][3] = pointsValue[4 * cc + 3].asDouble();
    }

    bool presetIsLog = preset.get("UseLogScale", Json::Value(false)).asBool();
    vtkNormalize(cntrlPoints, presetIsLog);
    preset.removeMember("UseLogScale");

    bool proxyIsLog = (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() == 1);
    vtkRescaleNormalizedControlPoints(cntrlPoints, range[0], range[1], proxyIsLog);
    this->LastRange[0] = range[0];
    this->LastRange[1] = range[1];

    pointsValue.resize(static_cast<Json::ArrayIndex>(cntrlPoints.size() * 4));
    for (size_t cc = 0; cc < cntrlPoints.size(); cc++)
    {
      pointsValue[static_cast<Json::ArrayIndex>(4 * cc)] = Json::Value(cntrlPoints[cc][0]);
      pointsValue[static_cast<Json::ArrayIndex>(4 * cc) + 1] = Json::Value(cntrlPoints[cc][1]);
      pointsValue[static_cast<Json::ArrayIndex>(4 * cc) + 2] = Json::Value(cntrlPoints[cc][2]);
      pointsValue[static_cast<Json::ArrayIndex>(4 * cc) + 3] = Json::Value(cntrlPoints[cc][3]);
    }
  }

  if (!preset.isMember("HSVWrap"))
  {
    preset["HSVWrap"] = 0;
  }
  if (vtkSMSettings::DeserializeFromJSON(this, preset))
  {
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
  auto presets = vtkSMTransferFunctionPresets::GetInstance();
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
  if (vtkSMPropertyHelper(this, "UseLogScale", true).GetAsInt() == 1)
  {
    // save log-space only if using log space.
    toSave->AddString("UseLogScale");
  }

  vtkNew<vtkSMNamedPropertyIterator> iter;
  iter->SetProxy(this);
  iter->SetPropertyNames(toSave);
  return vtkSMSettings::SerializeAsJSON(this, iter);
}

//----------------------------------------------------------------------------
Json::Value vtkSMTransferFunctionProxy::GetStateAsPreset(vtkSMProxy* proxy)
{
  vtkSMTransferFunctionProxy* self = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
  return self ? self->GetStateAsPreset() : Json::Value();
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::SaveColorMap(vtkPVXMLElement* xml)
{
  if (!xml)
  {
    vtkWarningMacro("'xml' cannot be nullptr");
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
  vtkSMProperty* controlPointsProperty =
    indexedLookup ? this->GetProperty("IndexedColors") : GetControlPointsProperty(this);
  if (!controlPointsProperty)
  {
    return false;
  }

  vtkSMPropertyHelper cntrlPoints(controlPointsProperty);
  unsigned int num_elements = cntrlPoints.GetNumberOfElements();
  if (num_elements > 0)
  {
    if (indexedLookup)
    {
      // Save (r,g,b) tuples for categorical colors.
      std::vector<vtkTuple<double, 3> > points;
      points.resize(num_elements / 3);
      cntrlPoints.Get(points[0].GetData(), num_elements);

      for (size_t cc = 0; cc < points.size(); cc++)
      {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Point");
        child->AddAttribute("r", points[cc].GetData()[1]);
        child->AddAttribute("g", points[cc].GetData()[2]);
        child->AddAttribute("b", points[cc].GetData()[3]);
        child->AddAttribute("o", 1.0);
        xml->AddNestedElement(child);
      }
    }
    else
    {
      // Save (x, r, g, b) tuples for non-categorical colors.
      std::vector<vtkTuple<double, 4> > points;
      points.resize(num_elements / 4);
      cntrlPoints.Get(points[0].GetData(), num_elements);

      // sort the points by x, just in case user didn't add them correctly.
      std::sort(points.begin(), points.end(), StrictWeakOrdering());

      for (size_t cc = 0; cc < points.size(); cc++)
      {
        vtkNew<vtkPVXMLElement> child;
        child->SetName("Point");
        child->AddAttribute("x", points[cc].GetData()[0]);
        child->AddAttribute("r", points[cc].GetData()[1]);
        child->AddAttribute("g", points[cc].GetData()[2]);
        child->AddAttribute("b", points[cc].GetData()[3]);
        child->AddAttribute("o", "1");
        xml->AddNestedElement(child);
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
  xml->AddNestedElement(nan);

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
    return nullptr;
  }

  vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc = 0; cc < reprHelper.GetNumberOfElements(); cc++)
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
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionProxy::UpdateScalarBarsComponentTitle(vtkPVArrayInformation* info)
{
  // find all scalar bars for this transfer function and update their titles.
  for (unsigned int cc = 0, max = this->GetNumberOfConsumers(); cc < max; ++cc)
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
void vtkSMTransferFunctionProxy::RestoreFromSiteSettingsOrXML(const char* arrayName)
{
  // First reset to XML defaults
  this->ResetPropertiesToXMLDefaults();

  // Restore to site setting if there is one. If there isn't, this does not
  // change the property setting. NOTE: user settings have priority
  // of VTK_DOUBLE_MAX, so we set the site settings priority to a
  // number just below VTK_DOUBLE_MAX.
  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  // First, check to see if there is an array-specific transfer function in
  // the settings.
  double sitePriority = nextafter(VTK_DOUBLE_MAX, 0);
  std::ostringstream prefix;
  prefix << ".array_" << this->GetXMLGroup() << "." << arrayName;
  if (settings->HasSetting(prefix.str().c_str(), sitePriority))
  {
    // Attempt to use array-specific site setting
    settings->GetProxySettings(prefix.str().c_str(), this, sitePriority);
  }
  else
  {
    // Attempt to use general site setting
    settings->GetProxySettings(this, sitePriority);
  }

  // Need to identify the active annotations in the dataset.
  this->ComputeAvailableAnnotations(this, false);
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionProxy::ResetPropertiesToDefaults(
  const char* arrayName, bool preserve_range)
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
      // nothing to do, so restore without rescaling the range
      throw true;
    }

    std::vector<vtkTuple<double, 4> > points;
    points.resize(num_elements / 4);
    cntrlPoints.Get(points[0].GetData(), num_elements);

    // sort the points by x, just in case user didn't add them correctly.
    std::sort(points.begin(), points.end(), StrictWeakOrdering());

    double range[2] = { points.front().GetData()[0], points.back().GetData()[0] };

    this->RestoreFromSiteSettingsOrXML(arrayName);

    this->RescaleTransferFunction(range[0], range[1], false);
  }
  catch (bool val)
  {
    if (val)
    {
      this->RestoreFromSiteSettingsOrXML(arrayName);
    }
  }

  // Ensure the lookup table is rebuilt.
  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionProxy::ResetRescaleModeToGlobalSetting()
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSMProxy* settingsProxy = pxm->GetProxy("settings", "GeneralSettings");
  // Guard against the settings proxies not being available.
  if (settingsProxy)
  {
    int globalResetMode =
      vtkSMPropertyHelper(settingsProxy, "TransferFunctionResetMode").GetAsInt();
    vtkSMPropertyHelper(this, "AutomaticRescaleRangeMode").Set(globalResetMode);
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

  bool indexedLookup = (strcmp(xml->GetAttributeOrDefault("indexedLookup", "false"), "true") == 0);
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
  if (nanElement && nanElement->GetAttribute("r") && nanElement->GetAttribute("g") &&
    nanElement->GetAttribute("b"))
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

  for (unsigned int cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* pointElement = xml->GetNestedElement(cc);
    double xrgb[4];
    if (pointElement && pointElement->GetName() && strcmp(pointElement->GetName(), "Point") == 0 &&
      pointElement->GetScalarAttribute("r", &xrgb[1]) &&
      pointElement->GetScalarAttribute("g", &xrgb[2]) &&
      pointElement->GetScalarAttribute("b", &xrgb[3]))
    {
      if (!indexedLookup && pointElement->GetScalarAttribute("x", &xrgb[0]))
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
      strcmp(pointElement->GetName(), "Annotation") && pointElement->GetAttribute("v") &&
      pointElement->GetAttribute("t"))
    {
      const char* value[2] = { pointElement->GetAttribute("v"), pointElement->GetAttribute("t") };
      new_annotations.push_back(vtkTuple<const char*, 2>(value));
    }
  }

  if (new_annotations.size() > 0)
  {
    Json::Value annotations(Json::arrayValue);
    for (int cc = 0, max = static_cast<int>(new_annotations.size()); cc < max; cc++)
    {
      annotations[2 * cc] = new_annotations[cc][0];
      annotations[2 * cc + 1] = new_annotations[cc][1];
    }
    json["Annotations"] = annotations;
  }

  if (new_points.size() > 0 && indexedLookup)
  {
    Json::Value rgbColors(Json::arrayValue);
    for (int cc = 0, max = static_cast<int>(new_points.size()); cc < max; cc++)
    {
      rgbColors[3 * cc] = new_points[cc].GetData()[1];
      rgbColors[3 * cc + 1] = new_points[cc].GetData()[2];
      rgbColors[3 * cc + 2] = new_points[cc].GetData()[3];
    }
    json["IndexedColors"] = rgbColors;
  }
  else if (new_points.size() > 0 && !indexedLookup)
  {
    // sort the points by x, just in case user didn't add them correctly.
    std::sort(new_points.begin(), new_points.end(), StrictWeakOrdering());

    Json::Value rgbColors(Json::arrayValue);
    for (int cc = 0, max = static_cast<int>(new_points.size()); cc < max; cc++)
    {
      rgbColors[4 * cc] = new_points[cc].GetData()[0];
      rgbColors[4 * cc + 1] = new_points[cc].GetData()[1];
      rgbColors[4 * cc + 2] = new_points[cc].GetData()[2];
      rgbColors[4 * cc + 3] = new_points[cc].GetData()[3];
    }
    json["RGBPoints"] = rgbColors;
  }

  // add name.
  json["Name"] = xml->GetAttribute("name");
  if (const char* creator = xml->GetAttribute("creator"))
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
  for (unsigned int cc = 0, max = xml->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    vtkPVXMLElement* elem = xml->GetNestedElement(cc);
    if (elem && elem->GetName() && strcmp(elem->GetName(), "ColorMap") == 0 &&
      elem->GetAttribute("name"))
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
Json::Value vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(
  const char* xmlcontents)
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(xmlcontents))
  {
    return Json::Value();
  }
  return vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(
    parser->GetRootElement());
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

  Json::Value json =
    vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(parser->GetRootElement());
  if (json.empty())
  {
    return false;
  }

  vtksys::ofstream file;
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
  os << indent << "LastRange: " << this->LastRange[0] << " " << this->LastRange[1] << endl;
  if (this->HistogramTableCache)
  {
    os << indent << "HistogramTableCache: " << endl;
    this->HistogramTableCache->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "HistogramTableCache: " << this->HistogramTableCache << endl;
  }
}

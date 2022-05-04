/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransferFunction2DProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// ParaView includes
#include "vtkSMTransferFunction2DProxy.h"

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTransferFunction2DBox.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"

// VTK includes
#include <vtkAlgorithm.h>
#include <vtkDoubleArray.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkScalarsToColors.h>
#include <vtkStringArray.h>
#include <vtkTuple.h>

// STL includes
#include <algorithm>
#include <vector>

namespace
{

//----------------------------------------------------------------------------
inline vtkSMProperty* GetTransferFunction2DBoxesProperty(vtkSMProxy* self)
{
  vtkSMProperty* controlBoxesProperty = self->GetProperty("Boxes");
  if (!controlBoxesProperty)
  {
    vtkGenericWarningMacro("'Boxes' property is required.");
    return nullptr;
  }
  vtkSMPropertyHelper cntrlBoxes(controlBoxesProperty);
  unsigned int num_elements = cntrlBoxes.GetNumberOfElements();
  if (num_elements % vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE != 0)
  {
    vtkGenericWarningMacro("'Boxes' property must have "
      << vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE << "-tuples. Resizing.");
    cntrlBoxes.SetNumberOfElements(
      (num_elements / vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE) *
      vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE);
  }

  return controlBoxesProperty;
}

//----------------------------------------------------------------------------
inline vtkSMProperty* GetTransferFunction2DRangeProperty(vtkSMProxy* self)
{
  vtkSMProperty* tf2DRangeProperty = self->GetProperty("Range");
  if (!tf2DRangeProperty)
  {
    vtkGenericWarningMacro("'Range' property is required.");
    return nullptr;
  }
  vtkSMPropertyHelper tf2DRange(tf2DRangeProperty);
  unsigned int num_elements = tf2DRange.GetNumberOfElements();
  if (num_elements != 4)
  {
    vtkGenericWarningMacro("'Range' property must have 4-tuples. Resizing.");
    tf2DRange.SetNumberOfElements(4);
  }
  return tf2DRangeProperty;
}

//----------------------------------------------------------------------------
inline vtkSMProperty* GetTransferFunction2DOutputDimensionsProperty(vtkSMProxy* self)
{
  vtkSMProperty* outDimsProperty = self->GetProperty("OutputDimensions");
  if (!outDimsProperty)
  {
    vtkGenericWarningMacro("'OutputDimensions' property is required.");
    return nullptr;
  }
  vtkSMPropertyHelper outDims(outDimsProperty);
  unsigned int num_elements = outDims.GetNumberOfElements();
  if (num_elements != 2)
  {
    vtkGenericWarningMacro("'OutputDimensions' property must have 2-tuples. Resizing.");
    outDims.SetNumberOfElements(2);
  }
  return outDimsProperty;
}

//----------------------------------------------------------------------------
// Normalize cntrlBoxes so that the two ranges go from (0, 1).
// The cntrlBoxes are assumed to be using log-space interpolation
// if "log_space" is true. The result is always in linear space
// irrespective of the original interpolation space.
// originalRange is the range of the cntrlBoxes before rescaling.
bool vtkNormalize(
  std::vector<vtkTuple<double, vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE>>& cntrlBoxes,
  bool log_space, vtkTuple<double, 4>* originalRange)
{
  if (cntrlBoxes.empty() || !originalRange)
  {
    // nothing to do, but not an error, so return true.
    return true;
  }

  // if in log_space, let's convert all the box values to log.
  if (log_space)
  {
    for (auto box : cntrlBoxes)
    {
      for (auto i = 0; i < 4; ++i)
      {
        box[i] = log10(box[i]);
      }
    }
  }

  // now simply normalize the box points
  if ((*originalRange)[0] == 0.0 && (*originalRange)[1] == 1.0 && (*originalRange)[2] == 0.0 &&
    (*originalRange)[3] == 1.0)
  {
    // nothing to do.
    return true;
  }

  const double xDenominator = (*originalRange)[1] - (*originalRange)[0];
  assert(xDenominator > 0);
  const double yDenominator = (*originalRange)[3] - (*originalRange)[2];
  assert(yDenominator > 0);

  for (auto box : cntrlBoxes)
  {
    box[0] = (box[0] - (*originalRange)[0]) / xDenominator;
    box[1] = (box[1] - (*originalRange)[1]) / xDenominator;
    box[2] = (box[2] - (*originalRange)[2]) / yDenominator;
    box[3] = (box[3] - (*originalRange)[3]) / yDenominator;
  }
  return true;
}

//----------------------------------------------------------------------------
// Rescale normalized control boxes to the given range. If "log_space" is true, the log
// interpolation is used between rangeMin and rangeMax.
bool vtkRescaleNormalizedControlBoxes(
  std::vector<vtkTuple<double, vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE>>& cntrlBoxes,
  vtkTuple<double, 4>* range, bool log_space)
{
  assert((*range)[0] < (*range)[1]);
  assert((*range)[2] < (*range)[3]);
  double xrange[2] = { (*range)[0], (*range)[1] };
  double yrange[2] = { (*range)[2], (*range)[3] };
  if (log_space)
  {
    if (xrange[0] <= 0.0 || xrange[1] <= 0.0)
    {
      // ensure the range is valid for log space.
      if (vtkSMCoreUtilities::AdjustRangeForLog(xrange))
      {
        vtkGenericWarningMacro("X-Range not valid for log-space. "
                               "Changed the range to ("
          << xrange[0] << ", " << xrange[1] << ").");
      }
    }
    if (yrange[0] <= 0.0 || yrange[1] <= 0.0)
    {
      // ensure the range is valid for log space.
      if (vtkSMCoreUtilities::AdjustRangeForLog(yrange))
      {
        vtkGenericWarningMacro("Y-Range not valid for log-space. "
                               "Changed the range to ("
          << yrange[0] << ", " << yrange[1] << ").");
      }
    }
  }

  if (log_space)
  {
    xrange[0] = log10(xrange[0]);
    xrange[1] = log10(xrange[1]);
    yrange[0] = log10(yrange[0]);
    yrange[1] = log10(yrange[1]);
  }
  double scale[2];
  scale[0] = xrange[1] - xrange[0];
  assert(scale[0] > 0);
  scale[1] = yrange[1] - yrange[0];
  assert(scale[1] > 0);
  for (auto box : cntrlBoxes)
  {
    for (int j = 0; j < 2; ++j)
    {
      double& x = box[j];
      x = x * scale[0] + xrange[0];
      if (log_space)
      {
        x = pow(10.0, x);
      }
    }
    for (int j = 2; j < 3; ++j)
    {
      double& y = box[j];
      y = y * scale[1] + yrange[0];
      if (log_space)
      {
        y = pow(10.0, y);
      }
    }
  }
  return true;
}

}

vtkStandardNewMacro(vtkSMTransferFunction2DProxy);

//------------------------------------------------------------------------------------------------
void vtkSMTransferFunction2DProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Histogram2DCache)
  {
    os << indent << "Histogram2DCache: " << endl;
    this->Histogram2DCache->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Histogram2DCache: " << this->Histogram2DCache << endl;
  }
}

//------------------------------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::RescaleTransferFunction(vtkSMProxy* proxy, double rangeXMin,
  double rangeXMax, double rangeYMin, double rangeYMax, bool extend)
{
  vtkSMTransferFunction2DProxy* tfp = vtkSMTransferFunction2DProxy::SafeDownCast(proxy);

  if (!tfp)
  {
    return false;
  }

  return tfp->RescaleTransferFunction(rangeXMin, rangeXMax, rangeYMin, rangeYMax, extend);
}

//------------------------------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::GetRange(double range[4])
{
  range[0] = range[2] = VTK_DOUBLE_MAX;
  range[1] = range[3] = VTK_DOUBLE_MIN;

  vtkSMProperty* tf2dRangeProperty = GetTransferFunction2DRangeProperty(this);
  if (!tf2dRangeProperty)
  {
    return range;
  }

  vtkSMPropertyHelper rangeHelper(tf2dRangeProperty);
  unsigned int num_elements = rangeHelper.GetNumberOfElements();
  if (num_elements != 4)
  {
    return range;
  }

  rangeHelper.Get(range, 4);
  return range;

#if 0
  vtkSMProperty* controlBoxesProperty = GetTransferFunction2DBoxesProperty(this);
  if (!controlBoxesProperty)
  {
    return false;
  }

  vtkSMPropertyHelper cntrlBoxes(controlBoxesProperty);
  unsigned int num_elements = cntrlBoxes.GetNumberOfElements();
  if (num_elements < 4)
  {
    return false;
  }

  std::vector<vtkTuple<double, BOX_PROPERTY_SIZE>> boxes;
  boxes.resize(num_elements / BOX_PROPERTY_SIZE);
  cntrlBoxes.Get(boxes[0].GetData(), num_elements);

  auto boxXMin = std::min_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2)
    { return box1[0] < box2[0]; });
  range[0] = (*boxXMin)[0];

  // For the max value, check box (X + Width). Note that the property values are stored as
  // [x, y, width, height]
  auto boxXMax = std::max_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2)
    { return box1[0] + box1[2] < box2[0] + box2[2]; });
  range[1] = (*boxXMax)[0] + (*boxXMax)[2];

  auto boxYMin = std::min_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2)
    { return box1[1] < box2[1]; });
  range[2] = (*boxYMin)[1];

  // For the max value, check box (Y + Height). Note that the property values are stored as
  // [x, y, width, height]
  auto boxYMax = std::max_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2)
    { return box1[1] + box1[3] < box2[1] + box2[3]; });
  range[3] = (*boxYMax)[1] + (*boxYMax)[3];

  return true;
#endif
}

//------------------------------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::RescaleTransferFunction(
  double rangeXMin, double rangeXMax, double rangeYMin, double rangeYMax, bool extend)
{
  // Adjust the range
  vtkSMCoreUtilities::AdjustRange(rangeXMin, rangeXMax);
  vtkSMCoreUtilities::AdjustRange(rangeYMin, rangeYMax);
  if (rangeXMax < rangeXMin || rangeYMax < rangeYMin)
  {
    return false;
  }

  vtkSMProperty* rangeProperty = GetTransferFunction2DRangeProperty(this);
  if (!rangeProperty)
  {
    return false;
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

  vtkSMPropertyHelper rangeHelper(rangeProperty);
  vtkTuple<double, 4> currentRange;
  rangeHelper.Get(currentRange.GetData(), 4);

  if (extend)
  {
    rangeXMin = std::min(rangeXMin, currentRange[0]);
    rangeXMax = std::max(rangeXMax, currentRange[1]);
    rangeYMin = std::min(rangeYMin, currentRange[2]);
    rangeYMax = std::max(rangeYMax, currentRange[3]);
  }
  if (rangeXMin == currentRange[0] && rangeXMax == currentRange[1] &&
    rangeYMin == currentRange[2] && rangeYMax == currentRange[3])
  {
    // The range is the same.
    // Nothing to do here.
    return true;
  }

  vtkTuple<double, 4> newRange;
  newRange[0] = rangeXMin;
  newRange[1] = rangeXMax;
  newRange[2] = rangeYMin;
  newRange[3] = rangeYMax;

  // Rescale the control boxes
  vtkSMProperty* controlBoxesProperty = GetTransferFunction2DBoxesProperty(this);
  unsigned int num_elements = 0;
  std::vector<vtkTuple<double, BOX_PROPERTY_SIZE>> boxes;
  if (controlBoxesProperty)
  {
    vtkSMPropertyHelper cntrlBoxes(controlBoxesProperty);
    // vtkSMPropertyHelper cntrlBoxes(controlBoxesProperty);
    num_elements = cntrlBoxes.GetNumberOfElements();
    if (num_elements != 0)
    {
      // just in case the num_elements is not a perfect multiple of BOX_PROPERTY_SIZE
      num_elements = BOX_PROPERTY_SIZE * (num_elements / BOX_PROPERTY_SIZE);
      boxes.resize(num_elements / BOX_PROPERTY_SIZE);
      cntrlBoxes.Get(boxes[0].GetData(), num_elements);

      // Normalize the range
      vtkNormalize(boxes, false, &currentRange);

      vtkRescaleNormalizedControlBoxes(boxes, &newRange, false);
    }
  }
  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("RescaleTransferFunction")
    .arg(rangeXMin)
    .arg(rangeXMax)
    .arg(rangeYMin)
    .arg(rangeYMax)
    .arg("comment", "Rescale 2D transfer function");

  rangeHelper.Set(newRange.GetData(), 4);
  if (controlBoxesProperty && num_elements != 0)
  {
    vtkSMPropertyHelper(controlBoxesProperty).Set(boxes[0].GetData(), num_elements);
  }
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::ComputeDataRange(double range[4])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;
  range[2] = VTK_DOUBLE_MAX;
  range[3] = VTK_DOUBLE_MIN;

  return (range[0] <= range[1]);
}

//------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMTransferFunction2DProxy::ComputeDataHistogram2D(int numberOfBins)
{
  // Recover component property
  int component = -1;

  // Find the visible consumer using the transfer function proxy
  vtkPVArrayInformation* arrayInfo = nullptr;
  std::string arrayName;
  int arrayAsso = -1;
  bool hasData = false;
  std::set<vtkSMProxy*> usedProxy;
  vtkSMSourceProxy* input = nullptr;
  bool useGradientAsY = true;
  std::string array2Name;
  int array2Asso = -1;
  int array2Component = 0;
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
      // do not count proxy multiple times
      usedProxy.find(consumer) == usedProxy.end())
    {
      // Recover consumer color array
      vtkPVArrayInformation* tmpArrayInfo = consumer->GetArrayInformationForColorArray(false);
      if (!tmpArrayInfo)
      {
        continue;
      }

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
          this->Histogram2DCache = nullptr;
          return this->Histogram2DCache;
        }

        vtkSMPropertyHelper colorArrayHelper(consumer, "ColorArrayName");
        arrayAsso = colorArrayHelper.GetInputArrayAssociation();
        arrayName = colorArrayHelper.GetInputArrayNameToProcess();

        useGradientAsY =
          (vtkSMPropertyHelper(consumer, "UseGradientForTransfer2D", true).GetAsInt() == 1);
        if (!useGradientAsY)
        {
          vtkSMPropertyHelper colorArray2Helper(consumer, "ColorArray2Name");
          array2Asso = colorArray2Helper.GetInputArrayAssociation();
          array2Name = colorArray2Helper.GetInputArrayNameToProcess();
          vtkSMPropertyHelper inputHelper(consumer, "Input");
          vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
          unsigned int port = inputHelper.GetOutputPort();
          vtkPVArrayInformation* arrayInfoFromData =
            inputProxy->GetDataInformation(port)->GetArrayInformation(
              colorArray2Helper.GetInputArrayNameToProcess(),
              colorArray2Helper.GetInputArrayAssociation());
          if (arrayInfoFromData && arrayInfoFromData->GetNumberOfComponents() > 1)
          {
            array2Component =
              vtkSMPropertyHelper(consumer, "SelectColorArray2Component", true).GetAsInt();
          }
        }
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

      usedProxy.insert(consumer);
      // Found the consumer.
      input = vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(consumer, "Input").GetAsProxy());
      hasData = true;
      break;
    }
  }

  // No valid consumer
  if (!hasData)
  {
    this->Histogram2DCache = nullptr;
    return this->Histogram2DCache;
  }

  double range[4];
  this->GetRange(range);

  // Compute the histogram
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSmartPointer<vtkSMSourceProxy> histo;
  histo.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ExtractHistogram2D")));
  vtkSMPropertyHelper(histo, "Input").Set(input);
  vtkSMPropertyHelper(histo, "SelectInputArray1")
    .SetInputArrayToProcess(arrayAsso, arrayName.c_str());
  vtkSMPropertyHelper(histo, "Component0").Set(component);
  vtkSMPropertyHelper(histo, "SelectInputArray2")
    .SetInputArrayToProcess(array2Asso, array2Name.c_str());
  vtkSMPropertyHelper(histo, "Component1").Set(array2Component);
  int numBins[2];
  numBins[0] = numberOfBins;
  numBins[1] = numberOfBins;
  vtkSMPropertyHelper(histo, "NumberOfBins").Set(numBins, 2);
  vtkSMPropertyHelper(histo, "UseCustomBinRangesX").Set(true);
  vtkSMPropertyHelper(histo, "CustomBinRangesX").Set(range, 2);
  vtkSMPropertyHelper(histo, "UseGradientForYAxis").Set(useGradientAsY);
  vtkSMPropertyHelper(histo, "UseCustomBinRangesY").Set(false);
  // Use the input ranges for the histogram bounds so that the editor shows true ranges
  vtkSMPropertyHelper(histo, "UseInputRangesForOutputBounds").Set(true);
  //  vtkSMPropertyHelper(histo, "CustomBinRangesY").Set(range + 2, 2);
  histo->UpdateVTKObjects();

  // Reduce it
  // Use vtkImageMathematics to add up all the image histograms (per pixel) from different server
  // processes / mpi ranks
  vtkSmartPointer<vtkSMSourceProxy> reducer;
  reducer.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ReductionFilter")));
  vtkSMPropertyHelper(reducer, "Input").Set(histo);
  vtkSMPropertyHelper(reducer, "PostGatherHelperName").Set("vtkImageMathematics");
  reducer->UpdateVTKObjects();

  // Move it from server to client and save it to the case
  vtkSmartPointer<vtkSMSourceProxy> mover;
  mover.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ClientServerMoveData")));
  vtkSMPropertyHelper(mover, "Input").Set(reducer);
  vtkSMPropertyHelper(mover, "OutputDataType").Set(VTK_IMAGE_DATA);
  mover->UpdateVTKObjects();
  mover->UpdatePipeline();
  vtkSmartPointer<vtkImageData> hist2D = vtkImageData::SafeDownCast(
    vtkAlgorithm::SafeDownCast(mover->GetClientSideObject())->GetOutputDataObject(0));
  if (!this->Histogram2DCache)
  {
    this->Histogram2DCache = vtkSmartPointer<vtkImageData>::New();
  }
  this->Histogram2DCache->DeepCopy(hist2D);

  // Sanity check
  if (this->Histogram2DCache->GetNumberOfPoints() <
    static_cast<long long>(numberOfBins) * numberOfBins)
  {
    vtkErrorMacro("Histogram2D did not produce enough data");
    this->Histogram2DCache = nullptr;
    return this->Histogram2DCache;
  }
  vtkSmartPointer<vtkDoubleArray> valueArray =
    vtkDoubleArray::SafeDownCast(this->Histogram2DCache->GetPointData()->GetScalars());
  if (!valueArray)
  {
    vtkErrorMacro("Histogram2D is not producing the values as expected");
    this->Histogram2DCache = nullptr;
    return this->Histogram2DCache;
  }

  vtkNew<vtkStringArray> s;
  s->SetName("ArrayNames");
  s->SetNumberOfTuples(2);
  s->SetValue(0, arrayName.c_str());
  s->SetValue(1, (useGradientAsY ? "Gradient Magnitude" : array2Name.c_str()));
  this->Histogram2DCache->GetFieldData()->AddArray(s);

  // Set the Y axis range on the 2D transfer function.
  // This is ideally only required when the Y axis is the gradient function.
  // Applying to all cases right now as ParaView doesn't support custom ranges
  // for Y axis of the transfer function yet.
  // if (useGradientAsY)
  // {
  vtkSMProperty* rangeProperty = GetTransferFunction2DRangeProperty(this);

  vtkSMPropertyHelper rangeHelper(rangeProperty);

  // Check if the range needs to be updated.
  vtkTuple<double, 4> currentRange;
  rangeHelper.Get(currentRange.GetData(), 4);
  double bounds[6];
  this->Histogram2DCache->GetBounds(bounds);
  currentRange[2] = bounds[2];
  currentRange[3] = bounds[3];
  rangeHelper.Set(currentRange.GetData(), 4);
  this->UpdateVTKObjects();
  // }

  // Update the output dimensions property once the image is updated
  vtkSMProperty* outDimsProperty = GetTransferFunction2DOutputDimensionsProperty(this);
  vtkSMPropertyHelper outDimsHelper(outDimsProperty);
  int outDims[2];
  outDims[0] = numberOfBins;
  outDims[1] = numberOfBins;
  outDimsHelper.Set(outDims, 2);

  return this->Histogram2DCache;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::RescaleTransferFunctionToDataRange(bool extend)
{
  double range[4] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  if (this->ComputeDataRange(range))
  {
    return this->RescaleTransferFunction(range[0], range[1], range[2], range[3], extend);
  }
  return false;
}

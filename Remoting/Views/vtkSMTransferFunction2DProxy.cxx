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

// VTK includes
#include <vtkAlgorithm.h>
#include <vtkDoubleArray.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkScalarsToColors.h>
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
    vtkGenericWarningMacro("Property must have " << vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE
                                                 << "-tuples. Resizing.");
    cntrlBoxes.SetNumberOfElements(
      (num_elements / vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE) *
      vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE);
  }

  return controlBoxesProperty;
}

//----------------------------------------------------------------------------
// Normalize cntrlBoxes so that the two ranges go from (0, 1).
// The cntrlBoxes are assumed to be using log-space interpolation
// if "log_space" is true. The result is always in linear space
// irrespective of the original interpolation space.
// originalRange is filled with the original range of the cntrlBoxes before rescaling.
bool vtkNormalize(
  std::vector<vtkTuple<double, vtkSMTransferFunction2DProxy::BOX_PROPERTY_SIZE>>& cntrlBoxes,
  bool log_space, vtkTuple<double, 4>* originalRange = nullptr)
{
  if (cntrlBoxes.empty())
  {
    // nothing to do, but not an error, so return true.
    return true;
  }

  if (cntrlBoxes.size() == 1)
  {
    if (originalRange)
    {
      (*originalRange)[0] = cntrlBoxes[0][0];
      (*originalRange)[1] = cntrlBoxes[0][0] + cntrlBoxes[0][2];
      (*originalRange)[2] = cntrlBoxes[0][1];
      (*originalRange)[3] = cntrlBoxes[0][1] + cntrlBoxes[0][3];
    }
    return true;
  }
  return true;
}
}

vtkStandardNewMacro(vtkSMTransferFunction2DProxy);

//------------------------------------------------------------------------------------------------
void vtkSMTransferFunction2DProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastRange: " << this->LastRange[0] << " " << this->LastRange[1] << " "
     << this->LastRange[2] << " " << this->LastRange[3] << endl;
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
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2) {
      return box1[0] < box2[0];
    });
  range[0] = (*boxXMin)[0];

  // For the max value, check box (X + Width). Note that the property values are stored as
  // [x, y, width, height]
  auto boxXMax = std::max_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2) {
      return box1[0] + box1[2] < box2[0] + box2[2];
    });
  range[1] = (*boxXMax)[0] + (*boxXMax)[2];

  auto boxYMin = std::min_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2) {
      return box1[1] < box2[1];
    });
  range[2] = (*boxYMin)[1];

  // For the max value, check box (Y + Height). Note that the property values are stored as
  // [x, y, width, height]
  auto boxYMax = std::max_element(boxes.begin(), boxes.end(),
    [](vtkTuple<double, BOX_PROPERTY_SIZE> box1, vtkTuple<double, BOX_PROPERTY_SIZE> box2) {
      return box1[1] + box1[3] < box2[1] + box2[3];
    });
  range[3] = (*boxYMax)[1] + (*boxYMax)[3];

  return true;
}

//------------------------------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::RescaleTransferFunction(
  double rangeXMin, double rangeXMax, double rangeYMin, double rangeYMax, bool extend)
{
  vtkSMProperty* controlBoxesProperty = GetTransferFunction2DBoxesProperty(this);
  if (!controlBoxesProperty)
  {
    return false;
  }

  vtkSMCoreUtilities::AdjustRange(rangeXMin, rangeXMax);
  vtkSMCoreUtilities::AdjustRange(rangeYMin, rangeYMax);
  if (rangeXMax < rangeXMin || rangeYMax < rangeYMin)
  {
    return false;
  }

  vtkSMPropertyHelper cntrlBoxes(controlBoxesProperty);
  unsigned int num_elements = cntrlBoxes.GetNumberOfElements();
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

  // just in case the num_elements is not a perfect multiple of BOX_PROPERTY_SIZE
  num_elements = BOX_PROPERTY_SIZE * (num_elements / BOX_PROPERTY_SIZE);
  std::vector<vtkTuple<double, BOX_PROPERTY_SIZE>> points;
  points.resize(num_elements / BOX_PROPERTY_SIZE);
  cntrlBoxes.Get(points[0].GetData(), num_elements);

  if (extend)
  {
    vtkSMProperty* rngProp = this->GetProperty("CustomRange");
    if (!rngProp)
    {
      vtkGenericWarningMacro("'CustomRange' property is required.");
      return false;
    }
    vtkSMPropertyHelper rng(rngProp);
    unsigned int rng_num_elements = rng.GetNumberOfElements();
    if (rng_num_elements != 4)
    {
      vtkGenericWarningMacro(
        "'CustomRange' property must have 4-tuples. Found " << rng_num_elements);
      return false;
    }
  }

  //  // Setting the "LastRange" here because, it should match the current range of the control
  //  boxes.
  //  // Aside from that, it will also be used for computing the 2D histogram.
  //  this->LastRange[0] = rangeXMin;
  //  this->LastRange[1] = rangeXMax;
  //  this->LastRange[2] = rangeYMin;
  //  this->LastRange[3] = rangeYMax;
  //

  // TODO: Normalize and rescale
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunction2DProxy::ComputeDataRange(double range[4])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;
  range[2] = VTK_DOUBLE_MAX;
  range[3] = VTK_DOUBLE_MIN;

  //  int component = -1;
  //  if (vtkSMPropertyHelper(this, "VectorMode").GetAsInt() == vtkScalarsToColors::COMPONENT)
  //  {
  //    component = vtkSMPropertyHelper(this, "VectorComponent").GetAsInt();
  //  }
  //
  //  // Find the visible consumer using the transfer function proxy
  //  vtkPVArrayInformation* arrayInfo = nullptr;
  //  std::string arrayName;
  //  int arrayAsso = -1;
  //  bool hasData = false;
  //  std::set<vtkSMProxy*> usedProxy;
  //  vtkSMSourceProxy* input = nullptr;
  //  bool useGradientAsY = true;
  //  std::string array2Name;
  //  int array2Asso = -1;
  //  int array2Component = 0;
  //
  //  for (unsigned int cc = 0, max = this->GetNumberOfConsumers(); cc < max; ++cc)
  //  {
  //    vtkSMProxy* proxy = this->GetConsumerProxy(cc);
  //    // consumers could be subproxy of something; so, we locate the true-parent
  //    // proxy for a proxy.
  //    proxy = proxy ? proxy->GetTrueParentProxy() : nullptr;
  //    vtkSMPVRepresentationProxy* consumer = vtkSMPVRepresentationProxy::SafeDownCast(proxy);
  //    if (consumer &&
  //      // consumer is visible.
  //      vtkSMPropertyHelper(consumer, "Visibility", true).GetAsInt() == 1 &&
  //      // consumer is using scalar coloring.
  //      consumer->GetUsingScalarColoring())
  //    {
  //    }
  //      // Recover consumer color array
  //      vtkPVArrayInformation* tmpArrayInfo = consumer->GetArrayInformationForColorArray(false);
  //      if (!tmpArrayInfo)
  //      {
  //        continue;
  //      }
  //
  //      if (!arrayInfo)
  //      {
  //        arrayInfo = tmpArrayInfo;
  //        if (arrayInfo->GetNumberOfComponents() == 1)
  //        {
  //          // Set the right component value for single component array
  //          component = 0;
  //        }
  //        if (component == -1)
  //        {
  //          // Set the right component value for magnitude component
  //          component = arrayInfo->GetNumberOfComponents();
  //        }
  //        if (component > arrayInfo->GetNumberOfComponents())
  //        {
  //          vtkErrorMacro("Invalid component requested by the transfer function");
  //          this->HistogramTableCache = nullptr;
  //          return this->Histogram2DCache;
  //        }
  //
  //        vtkSMPropertyHelper colorArrayHelper(consumer, "ColorArrayName");
  //        arrayAsso = colorArrayHelper.GetInputArrayAssociation();
  //        arrayName = colorArrayHelper.GetInputArrayNameToProcess();
  return (range[0] <= range[1]);
}

//------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMTransferFunction2DProxy::ComputeDataHistogram2D(int numberOfBins)
{
  // Recover component property
  int component = -1;
  //  if (vtkSMPropertyHelper(this, "VectorMode").GetAsInt() == vtkScalarsToColors::COMPONENT)
  //  {
  //    component = vtkSMPropertyHelper(this, "VectorComponent").GetAsInt();
  //  }

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
      // consumer is of volume representation type
      // strcmp(vtkSMPropertyHelper(consumer, "RepresentationType", true).GetAsString(), "Volume")
      // ==
      // 0 &&
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
      // Add consumer to group filter
      // vtkSMSourceProxy* input =
      //   vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(consumer, "Input").GetAsProxy());
      // vtkSMPropertyHelper(group, "Input").Add(input);
      // group->UpdateVTKObjects();
    }
  }

  // No valid consumer
  if (!hasData)
  {
    this->Histogram2DCache = nullptr;
    return this->Histogram2DCache;
  }

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
  vtkSMPropertyHelper(histo, "CustomBinRangesX").Set(this->LastRange, 2);
  vtkSMPropertyHelper(histo, "UseGradientForYAxis").Set(useGradientAsY);
  vtkSMPropertyHelper(histo, "UseCustomBinRangesY").Set(false);
  //  vtkSMPropertyHelper(histo, "CustomBinRangesY").Set(this->LastYRange, 2);
  histo->UpdateVTKObjects();

  // Reduce it
  vtkSmartPointer<vtkSMSourceProxy> reducer;
  reducer.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ReductionFilter")));
  vtkSMPropertyHelper(reducer, "Input").Set(histo);
  vtkSMPropertyHelper(reducer, "PostGatherHelperName").Set("vtkImageAppend");
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
  if (this->Histogram2DCache->GetNumberOfPoints() < numberOfBins * numberOfBins)
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

//----------------------------------------------------------------------------

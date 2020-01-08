/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHistogramChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVHistogramChartRepresentation.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContextView.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPExtractHistogram.h"
#include "vtkPVContextView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"

#include <cmath>

static const char* BIN_EXTENTS = "bin_extents";
static const char* BIN_VALUES = "bin_values";

namespace
{
template <class T>
T vtkRound(double value)
{
  return static_cast<T>(std::floor(value + 0.5));
}
}

vtkStandardNewMacro(vtkPVHistogramChartRepresentation);
//----------------------------------------------------------------------------
vtkPVHistogramChartRepresentation::vtkPVHistogramChartRepresentation()
{
  this->ExtractHistogram = vtkPExtractHistogram::New();
  this->SetChartTypeToBar();
  this->SetUseIndexForXAxis(false);
  this->SetXAxisSeriesName(BIN_EXTENTS);
  this->SetSeriesVisibility(BIN_VALUES, true);
  this->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_ROWS);
  this->SetHistogramColor(0, 0, 255);
  this->AttributeType = vtkDataObject::POINT;
}

//----------------------------------------------------------------------------
vtkPVHistogramChartRepresentation::~vtkPVHistogramChartRepresentation()
{
  if (this->ExtractHistogram)
  {
    this->ExtractHistogram->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetBinCount(int bins)
{
  if (this->ExtractHistogram->GetBinCount() != bins)
  {
    this->ExtractHistogram->SetBinCount(bins);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
int vtkPVHistogramChartRepresentation::GetBinCount()
{
  return this->ExtractHistogram->GetBinCount();
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetCenterBinsAroundMinAndMax(bool center)
{
  if (this->ExtractHistogram->GetCenterBinsAroundMinAndMax() != center)
  {
    this->ExtractHistogram->SetCenterBinsAroundMinAndMax(center);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
bool vtkPVHistogramChartRepresentation::GetCenterBinsAroundMinAndMax()
{
  return this->ExtractHistogram->GetCenterBinsAroundMinAndMax();
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetUseCustomBinRanges(bool b)
{
  if (this->ExtractHistogram->GetUseCustomBinRanges() != b)
  {
    this->ExtractHistogram->SetUseCustomBinRanges(b);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
bool vtkPVHistogramChartRepresentation::GetUseCustomBinRanges()
{
  return this->ExtractHistogram->GetUseCustomBinRanges();
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetCustomBinRanges(double min, double max)
{
  double curRanges[2];
  this->ExtractHistogram->GetCustomBinRanges(curRanges);
  if (curRanges[0] != min || curRanges[1] != max)
  {
    this->ExtractHistogram->SetCustomBinRanges(min, max);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
double* vtkPVHistogramChartRepresentation::GetCustomBinRanges()
{
  return this->ExtractHistogram->GetCustomBinRanges();
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetComponent(int c)
{
  if (this->ExtractHistogram->GetComponent() != c)
  {
    this->ExtractHistogram->SetComponent(c);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
int vtkPVHistogramChartRepresentation::GetComponent()
{
  return this->ExtractHistogram->GetComponent();
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetHistogramColor(double r, double g, double b)
{
  this->SetColor(BIN_VALUES, r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetUseColorMapping(bool colorMapping)
{
  this->vtkXYChartRepresentation::SetUseColorMapping(BIN_VALUES, colorMapping);
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetLookupTable(vtkScalarsToColors* lut)
{
  this->vtkXYChartRepresentation::SetLookupTable(BIN_VALUES, lut);
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetHistogramLineStyle(int val)
{
  this->SetLineStyle(BIN_VALUES, val);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkPVHistogramChartRepresentation::TransformInputData(
  vtkDataObject* data)
{
  // NOTE: This method gets called in RequestData() and only on
  // the server-side, so avoid doing anything that modifies the MTime.
  if (this->ArrayName.empty())
  {
    return NULL;
  }

  this->ExtractHistogram->SetInputArrayToProcess(
    0, 0, 0, this->AttributeType, this->ArrayName.c_str());
  this->ExtractHistogram->CalculateAveragesOff();
  this->ExtractHistogram->SetInputData(data);
  this->ExtractHistogram->Update();

  return this->Superclass::TransformInputData(this->ExtractHistogram->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();
  vtkChartXY* chart = this->GetChart();
  chart->SetBarWidthFraction(1.0);
  vtkAxis* axis = chart->GetAxis(vtkAxis::LEFT);
  axis->SetMinimum(0);
  axis->SetMinimumLimit(0);
}

//----------------------------------------------------------------------------
bool vtkPVHistogramChartRepresentation::MapSelectionToInput(vtkSelection* sel)
{
  assert(sel != NULL);

  // Now we do the magic: convert chart row selection to threshold selection
  vtkNew<vtkDoubleArray> selRanges;
  selRanges->SetName(this->ArrayName.c_str());
  selRanges->SetNumberOfComponents(1);

  vtkTable* table = this->GetLocalOutput();
  vtkDoubleArray* binExtents = vtkDoubleArray::SafeDownCast(table->GetColumnByName(BIN_EXTENTS));
  double delta = 1.;
  if (binExtents->GetNumberOfTuples() >= 2)
  {
    delta = (binExtents->GetValue(1) - binExtents->GetValue(0)) * 0.5;
  }
  vtkIdType nbNodes = sel->GetNumberOfNodes();
  for (vtkIdType i = 0; i < nbNodes; i++)
  {
    vtkSelectionNode* node = sel->GetNode(i);
    vtkDataArray* selRows = vtkDataArray::SafeDownCast(node->GetSelectionList());
    if (selRows)
    {
      vtkIdType nbRows = selRows->GetNumberOfTuples();
      for (vtkIdType j = 0; j < nbRows; j++)
      {
        // FIXME: make this smart to collapse adjacent bins and specify a
        // single range instead of multiple ones. That'll be faster to execute.
        vtkIdType row = (vtkIdType)selRows->GetTuple1(j);
        double binExtent = binExtents->GetValue(row);
        selRanges->InsertNextTuple1(binExtent - delta);
        selRanges->InsertNextTuple1(binExtent + delta);
      }
    }
  }

  // Construct threshold selection on input
  vtkNew<vtkSelection> newSel;
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::THRESHOLDS);
  selNode->SetFieldType(
    vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->AttributeType));
  newSel->AddNode(selNode.Get());
  selNode->SetSelectionList(selRanges.Get());
  sel->ShallowCopy(newSel.GetPointer());
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVHistogramChartRepresentation::MapSelectionToView(vtkSelection* sel)
{
  assert(sel != NULL);

  // For vtkPVHistogramChartRepresentation (in the ServerManagerConfiguration
  // xml), we set the SelectionRepresentation up so that the original input
  // selection is passed to the vtkChartSelectionRepresentation rather than the
  // id-based selection. Here, we see if the sel is of type
  // vtkSelection::THRESHOLDS and if so, are the thresholds applicable to the
  // bins we are showing.

  // build a list of vtkSelectionNode instances that are potentially relevant.
  int fieldType = vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->AttributeType);
  std::vector<vtkSmartPointer<vtkSelectionNode> > nodes;
  for (unsigned int cc = 0, max = sel->GetNumberOfNodes(); cc < max; ++cc)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node && node->GetFieldType() == fieldType &&
      node->GetContentType() == vtkSelectionNode::THRESHOLDS && node->GetSelectionList() != NULL &&
      node->GetSelectionList()->GetName() != NULL &&
      this->ArrayName == node->GetSelectionList()->GetName() &&
      node->GetSelectionList()->GetNumberOfTuples() > 0)
    {
      // potentially applicable selection node.
      nodes.push_back(node);
    }
  }
  sel->RemoveAllNodes();

  if (nodes.size() == 0)
  {
    return true;
  }

  // now, check if the thresholds are applicable to the histogram being shown.
  // since this method is called in `RequestData` we need to use the
  // `pre_delivery` data.
  vtkTable* table = this->GetLocalOutput(/*pre_delivery = */ true);
  vtkDoubleArray* binExtents =
    table ? vtkDoubleArray::SafeDownCast(table->GetColumnByName(BIN_EXTENTS)) : NULL;
  if (binExtents == NULL)
  {
    // Seems like the vtkPVHistogramChartRepresentation hasn't updated yet and
    // the selection is being updated before it. Shouldn't happen, but let's
    // handle it.
    return false;
  }

  double delta = 1.;
  if (binExtents->GetNumberOfTuples() >= 2)
  {
    delta = (binExtents->GetValue(1) - binExtents->GetValue(0));
  }
  const double halfDelta = delta / 2.0;
  double dataRange[2];
  binExtents->GetRange(dataRange); // this is range of bin-midpoints.

  double episilon = delta * 1e-5; // we make this a factor of the delta.
  std::set<vtkIdType> selectedBins;
  for (size_t cc = 0; cc < nodes.size(); cc++)
  {
    vtkSelectionNode* node = nodes[cc];
    vtkDoubleArray* selectionList = vtkDoubleArray::SafeDownCast(node->GetSelectionList());
    for (vtkIdType j = 0; j < selectionList->GetNumberOfTuples(); ++j)
    {
      double range[2] = { selectionList->GetTypedComponent(j, 0),
        selectionList->GetTypedComponent(j, 1) };
      // since range is bin-range, convert that to bin-mid-points based range.
      range[0] += halfDelta;
      range[1] -= halfDelta;

      // now get the bin index for this range.
      vtkIdType index[2];
      index[0] = vtkRound<vtkIdType>((range[0] - dataRange[0]) / delta);
      index[1] = vtkRound<vtkIdType>((range[1] - dataRange[0]) / delta);

      // now only accept the range if it nearly matches the bins
      if (index[0] >= 0 && index[0] < binExtents->GetNumberOfTuples() &&
        std::abs(range[0] - binExtents->GetValue(index[0])) < episilon && index[1] >= 0 &&
        index[1] < binExtents->GetNumberOfTuples() &&
        std::abs(range[1] - binExtents->GetValue(index[1])) < episilon)
      {
        for (vtkIdType i = index[0]; i <= index[1]; ++i)
        {
          selectedBins.insert(i);
        }
      }
    }
  }

  vtkNew<vtkSelectionNode> node;
  node->SetContentType(vtkSelectionNode::INDICES);
  node->SetFieldType(vtkSelectionNode::POINT);
  vtkNew<vtkIdTypeArray> convertedSelectionList;
  convertedSelectionList->SetNumberOfTuples(static_cast<vtkIdType>(selectedBins.size()));
  if (selectedBins.size())
  {
    std::copy(selectedBins.begin(), selectedBins.end(), convertedSelectionList->GetPointer(0));
  }
  node->SetSelectionList(convertedSelectionList.GetPointer());
  sel->AddNode(node.GetPointer());
  return true;
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  if (this->AttributeType != fieldAssociation)
  {
    this->AttributeType = fieldAssociation;
    this->MarkModified();
  }

  std::string arrayName(name ? name : "");
  if (this->ArrayName != name)
  {
    this->ArrayName = name;
    this->SetLabel(BIN_VALUES, this->ArrayName.c_str());
    this->MarkModified();
  }
}

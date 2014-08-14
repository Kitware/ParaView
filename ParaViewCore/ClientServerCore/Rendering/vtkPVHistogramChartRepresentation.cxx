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
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkPExtractHistogram.h"
#include "vtkPlotBar.h"
#include "vtkPVContextView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"

#include <string>

static const char* BIN_EXTENTS = "bin_extents";
static const char* BIN_VALUES = "bin_values";


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
  this->SetUseCache(false);
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
vtkDataObject* vtkPVHistogramChartRepresentation::TransformInputData(
  vtkInformationVector** vtkNotUsed(inputVector), vtkDataObject* data)
{
  // NOTE: This method gets called in RequestData() and only on
  // the server-side, so avoid doing anything that modifies the MTime.
  if (this->ArrayName.empty())
    {
    return NULL;
    }

  this->ExtractHistogram->SetInputArrayToProcess(0, 0, 0,
    this->AttributeType, this->ArrayName.c_str());
  this->ExtractHistogram->CalculateAveragesOff();
  this->ExtractHistogram->SetInputData(data);
  this->ExtractHistogram->Update();

  return this->ExtractHistogram->GetOutputDataObject(0);
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
vtkSelection* vtkPVHistogramChartRepresentation::GetSelection()
{
  vtkSelection* sel = NULL;
  if (vtkChart *chart = vtkChart::SafeDownCast(this->GetChart()))
    {
    sel = chart->GetAnnotationLink()->GetCurrentSelection();
    }
  if (!sel)
    {
    return NULL;
    }

  if (this->CachedSelection && this->CachedSelection->GetMTime() > sel->GetMTime())
    {
    return this->CachedSelection;
    }

  // Now we do the magic: convert chart row selection to threshold selection
  vtkNew<vtkDoubleArray> selRanges;
  selRanges->SetName(this->ArrayName.c_str());
  selRanges->SetNumberOfComponents(1);

  vtkTable* table = this->GetLocalOutput();
  vtkDoubleArray* binExtents =
    vtkDoubleArray::SafeDownCast(table->GetColumnByName(BIN_EXTENTS));
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
        vtkIdType row = (vtkIdType)selRows->GetTuple1(j);
        double binExtent = binExtents->GetValue(row);
        selRanges->InsertNextTuple1(binExtent - delta);
        selRanges->InsertNextTuple1(binExtent + delta);
        }
      }
    }

  // Construct threshold selection on input
  vtkSelection* newSel = vtkSelection::New();
  vtkNew<vtkSelectionNode> selNode;
  selNode->SetContentType(vtkSelectionNode::THRESHOLDS);
  int selType = 0;
  switch (this->AttributeType)
    {
    case vtkDataObject::POINT:
      selType = vtkSelectionNode::POINT;
      break;
    case vtkDataObject::CELL:
      selType = vtkSelectionNode::CELL;
      break;
    case vtkDataObject::ROW:
      selType = vtkSelectionNode::ROW;
      break;
    default:
      break;
    }
  selNode->SetFieldType(selType);
  newSel->AddNode(selNode.Get());
  selNode->SetSelectionList(selRanges.Get());

  this->CachedSelection.TakeReference(newSel);
  this->CachedSelection->Modified();
  return newSel;
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::ResetSelection()
{
  if (this->GetChart())
    {
    vtkNew<vtkSelection> emptySel;
    vtkNew<vtkSelectionNode> selNode;
    selNode->SetContentType(vtkSelectionNode::INDICES);
    selNode->SetFieldType(vtkSelectionNode::ROW);
    emptySel->AddNode(selNode.GetPointer());
    this->GetChart()->GetAnnotationLink()->SetCurrentSelection(emptySel.GetPointer());
    }
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::SetInputArrayToProcess(int idx,
                                          int port, int connection,
                                          int fieldAssociation,
                                          const char *name)
{
  this->Superclass::SetInputArrayToProcess(
    idx, port, connection, fieldAssociation, name);

  if (this->AttributeType != fieldAssociation)
    {
    this->AttributeType = fieldAssociation;
    this->MarkModified();
    }

  std::string arrayName(name? name : "");
  if (this->ArrayName != name)
    {
    this->ArrayName = name;
    this->SetLabel(BIN_VALUES, this->ArrayName.c_str());
    this->MarkModified();
    }
}

//----------------------------------------------------------------------------
void vtkPVHistogramChartRepresentation::MarkModified()
{
  this->ResetSelection();
  this->Superclass::MarkModified();
}
